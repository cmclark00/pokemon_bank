/**
 * @file pokemon_trade.c
 * @brief Pokemon Gen I trading state machine implementation
 * 
 * Based on the protocol documented in the Flipper Zero Pokemon Trading project.
 * 
 * Protocol overview:
 * 1. Link negotiation: Master sends 0x01, we respond 0x02 (slave)
 * 2. Connection confirmation: Both exchange 0x60 (CONNECTED)
 * 3. Menu selection: Game Boy selects Trade Centre (0xD4)
 * 4. Trade block exchange: 10x preamble, 10x random, 9x preamble, 415 bytes data
 * 5. Patch list exchange: 6x preamble, 7x blank, 189 bytes patch data
 * 6. Pokemon selection: Exchange which Pokemon to trade
 * 7. Confirmation: Both send 0x62 to accept or 0x61 to reject
 * 8. Completion: Re-exchange trade blocks with updated party data
 */

#include "pokemon_trade.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------
// Internal constants
//--------------------------------------------------------------------

#define PREAMBLE_COUNT       10
#define RANDOM_BYTES         10
#define TRADE_PREAMBLE       9
#define PATCH_PREAMBLE_COUNT 6
#define PATCH_DATA_SIZE      196

//--------------------------------------------------------------------
// Debug macros
//--------------------------------------------------------------------

// Always enable debug for now to trace handshake issues
#define DEBUG_TRADE 1

#ifdef DEBUG_TRADE
#define TRADE_LOG(fmt, ...) printf("[TRADE] " fmt "\n", ##__VA_ARGS__)
#else
#define TRADE_LOG(fmt, ...) ((void)0)
#endif

//--------------------------------------------------------------------
// State name lookup
//--------------------------------------------------------------------

const char* pokemon_trade_state_name(trade_state_t state) {
    switch (state) {
        case TRADE_STATE_NOT_CONNECTED: return "NOT_CONNECTED";
        case TRADE_STATE_CONNECTED:     return "CONNECTED";
        case TRADE_STATE_READY:         return "READY";
        case TRADE_STATE_WAITING:       return "WAITING";
        case TRADE_STATE_PENDING:       return "PENDING";
        case TRADE_STATE_TRADING:       return "TRADING";
        case TRADE_STATE_DONE:          return "DONE";
        case TRADE_STATE_ERROR:         return "ERROR";
        default:                        return "UNKNOWN";
    }
}

//--------------------------------------------------------------------
// Initialization
//--------------------------------------------------------------------

void pokemon_trade_init(pokemon_trade_ctx *ctx, const char *trainer_name, uint16_t trainer_id) {
    memset(ctx, 0, sizeof(pokemon_trade_ctx));
    
    ctx->state = TRADE_STATE_NOT_CONNECTED;
    ctx->tc_state = TC_RESET;
    
    // Create our tradeable Pokemon (Bulbasaur level 5)
    pokemon_create_bulbasaur(&ctx->our_data, trainer_name, trainer_id);
    
    // Build initial patch list
    pokemon_build_patch_list(&ctx->our_patches, &ctx->our_data);
    
    printf("[Trade] Initialized with trainer '%s' (ID: %d)\n", trainer_name, trainer_id);
    pokemon_print_info(&ctx->our_data.party[0], NULL);
}

void pokemon_trade_reset(pokemon_trade_ctx *ctx) {
    ctx->state = TRADE_STATE_NOT_CONNECTED;
    ctx->tc_state = TC_RESET;
    ctx->counter = 0;
    ctx->patch_part2 = false;
    ctx->selected_index = 0;
}

//--------------------------------------------------------------------
// Connection state handler
//--------------------------------------------------------------------

/**
 * IMPORTANT: Due to PIO autopull, the byte we return here gets sent on the
 * NEXT exchange, not the current one. The first byte (0x02 SLAVE) is preloaded
 * in gb_link_init(), so when we receive MASTER (0x01), we've ALREADY sent
 * SLAVE. Therefore we return BLANK for the next exchange.
 * 
 * Timeline:
 *   Exchange 1: GB sends 0x01, we send 0x02 (preloaded), return 0x00 for next
 *   Exchange 2: GB sends 0x00, we send 0x00, return 0x00
 *   ...continues with BLANKs...
 *   Exchange N: GB sends 0x60, we send 0x00, return 0x60 for next & set CONNECTED
 *   Exchange N+1: GB sends 0x60, we send 0x60, continue echoing 0x60
 */
static uint8_t handle_connection(pokemon_trade_ctx *ctx, uint8_t in) {
    uint8_t out = in;  // Default: echo
    
    // Log every byte during connection for debugging
    printf("[CONN] RX:0x%02X ", in);
    
    switch (in) {
        case PKMN_MASTER:
            // Game Boy declares itself master
            // We've ALREADY sent SLAVE (preloaded), so return BLANK for next exchange
            out = PKMN_BLANK;
            printf("-> MASTER (already sent SLAVE), next TX:0x%02X\n", out);
            TRADE_LOG("Master detected, SLAVE was preloaded");
            break;
            
        case PKMN_CONNECTED:
            // Connection confirmed! Echo CONNECTED and transition state
            ctx->state = TRADE_STATE_CONNECTED;
            out = PKMN_CONNECTED;
            printf("-> CONNECTED! next TX:0x%02X\n", out);
            TRADE_LOG("Connected!");
            break;
            
        case PKMN_BLANK:
            // Normal padding - echo it
            out = PKMN_BLANK;
            printf("-> BLANK, next TX:0x%02X\n", out);
            break;
            
        default:
            // Unknown byte - echo it back
            printf("-> 0x%02X (echo)\n", in);
            break;
    }
    
    return out;
}

//--------------------------------------------------------------------
// Menu state handler
//--------------------------------------------------------------------

static uint8_t handle_menu(pokemon_trade_ctx *ctx, uint8_t in) {
    uint8_t out = PKMN_BLANK;
    
    switch (in) {
        case PKMN_CONNECTED:
            out = PKMN_CONNECTED;
            break;
            
        case ITEM_1_HIGHLIGHTED:
        case ITEM_2_HIGHLIGHTED:
        case ITEM_3_HIGHLIGHTED:
            // Just echo back menu navigation
            out = in;
            break;
            
        case ITEM_1_SELECTED:  // Trade Centre
            ctx->state = TRADE_STATE_READY;
            ctx->tc_state = TC_RESET;
            TRADE_LOG("Trade Centre selected, entering trade room");
            break;
            
        case ITEM_2_SELECTED:  // Colosseum (battle) - just echo
            TRADE_LOG("Colosseum selected (loopback mode)");
            out = in;
            break;
            
        case ITEM_3_SELECTED:  // Cancel
            ctx->state = TRADE_STATE_NOT_CONNECTED;
            out = ITEM_3_SELECTED;
            TRADE_LOG("Link cancelled");
            break;
            
        case PKMN_MASTER:
            // Connection lost, restart
            ctx->state = TRADE_STATE_NOT_CONNECTED;
            out = PKMN_SLAVE;
            break;
            
        default:
            out = in;  // Echo unknown bytes
            break;
    }
    
    return out;
}

//--------------------------------------------------------------------
// Trade center state machine
//--------------------------------------------------------------------

static uint8_t handle_trade_center(pokemon_trade_ctx *ctx, uint8_t in) {
    uint8_t *our_block = (uint8_t *)&ctx->our_data;
    uint8_t *their_block = (uint8_t *)&ctx->their_data;
    uint8_t *their_party = (uint8_t *)ctx->their_data.party;
    uint8_t out = in;  // Default: echo
    
    switch (ctx->tc_state) {
        case TC_RESET:
            // Reset counters
            ctx->counter = 0;
            ctx->patch_part2 = false;
            ctx->tc_state = TC_INIT;
            // Fall through
            
        case TC_INIT:
            // Count preamble bytes (10x 0xFD)
            if (in == SERIAL_PREAMBLE_BYTE) {
                ctx->counter++;
                if (ctx->counter == PREAMBLE_COUNT) {
                    ctx->tc_state = TC_RANDOM;
                    ctx->counter = 0;
                    ctx->state = TRADE_STATE_WAITING;
                    TRADE_LOG("Preamble complete, entering random exchange");
                }
            }
            break;
            
        case TC_RANDOM:
            // 10x random bytes + 9x preamble
            ctx->counter++;
            if (ctx->counter == (RANDOM_BYTES + TRADE_PREAMBLE)) {
                ctx->tc_state = TC_DATA;
                ctx->counter = 0;
                TRADE_LOG("Random/preamble complete, starting data exchange");
            }
            break;
            
        case TC_DATA:
            // Exchange trade block data (415 bytes)
            their_block[ctx->counter] = in;
            out = our_block[ctx->counter];
            ctx->counter++;
            
            if (ctx->counter == PKMN_TRADE_BLOCK_SIZE) {
                ctx->tc_state = TC_PATCH_HEADER;
                ctx->counter = 0;
                TRADE_LOG("Trade block exchange complete");
            }
            break;
            
        case TC_PATCH_HEADER:
            // 3 byte ending sequence, then 6x preamble bytes
            if (in == SERIAL_PREAMBLE_BYTE) {
                ctx->counter++;
            }
            
            if (ctx->counter == 6) {
                ctx->tc_state = TC_PATCH_DATA;
                ctx->counter = 0;
                ctx->patch_part2 = false;
                TRADE_LOG("Patch header complete");
            }
            break;
            
        case TC_PATCH_DATA:
            // Exchange patch list data
            ctx->counter++;
            
            // After header (7 blank bytes), start sending our patches
            if (ctx->counter > 8) {
                out = ctx->our_patches.data[ctx->counter - 9];
            }
            
            // Apply received patches to their party data
            switch (in) {
                case PKMN_BLANK:
                    // Padding, ignore
                    break;
                case 0xFF:
                    // Part terminator
                    ctx->patch_part2 = true;
                    break;
                default:
                    // Patch index - restore 0xFE at position
                    if (!ctx->patch_part2) {
                        their_party[in - 1] = SERIAL_NO_DATA_BYTE;
                    } else {
                        their_party[0xFB + in] = SERIAL_NO_DATA_BYTE;
                    }
                    break;
            }
            
            if (ctx->counter == PATCH_DATA_SIZE) {
                ctx->tc_state = TC_SELECT;
                ctx->counter = 0;
                TRADE_LOG("Patch data complete, ready to select Pokemon");
                
                // Print received Pokemon info
                char nickname[12];
                pokemon_string_decode(nickname, ctx->their_data.nicknames[0].str, 10);
                pokemon_print_info(&ctx->their_data.party[0], nickname);
            }
            break;
            
        case TC_SELECT:
            // Reset selection state
            ctx->selected_index = 0;
            if (in == PKMN_BLANK) {
                ctx->tc_state = TC_PENDING;
            }
            break;
            
        case TC_PENDING:
            // Handle Pokemon selection or table exit
            if (in == PKMN_TABLE_LEAVE) {
                // Player left the trade table
                ctx->tc_state = TC_RESET;
                ctx->state = TRADE_STATE_READY;
                out = PKMN_TABLE_LEAVE;
                TRADE_LOG("Player left trade table");
            }
            else if ((in & PKMN_SEL_NUM_MASK) == PKMN_SEL_NUM_MASK) {
                // Player selected a Pokemon (index in low nibble)
                ctx->selected_index = in & 0x0F;
                out = PKMN_SEL_NUM_ONE;  // We always trade first Pokemon
                ctx->state = TRADE_STATE_PENDING;
                TRADE_LOG("Player selected Pokemon %d, we offer slot 0", ctx->selected_index);
            }
            else if (in == PKMN_BLANK) {
                if (ctx->selected_index != 0) {
                    // Selection confirmed, move to confirmation
                    ctx->tc_state = TC_CONFIRMATION;
                    out = PKMN_BLANK;
                }
            }
            break;
            
        case TC_CONFIRMATION:
            // Handle trade accept/reject
            if (in == PKMN_TRADE_REJECT) {
                // Trade rejected, go back to selection
                ctx->tc_state = TC_SELECT;
                ctx->state = TRADE_STATE_WAITING;
                out = PKMN_TRADE_REJECT;
                TRADE_LOG("Trade rejected");
            }
            else if (in == PKMN_TRADE_ACCEPT) {
                // Trade accepted!
                ctx->tc_state = TC_DONE;
                out = PKMN_TRADE_ACCEPT;
                TRADE_LOG("Trade accepted!");
            }
            break;
            
        case TC_DONE:
            // Trade complete - wait for blank byte then restart
            if (in == PKMN_BLANK) {
                ctx->tc_state = TC_RESET;
                ctx->state = TRADE_STATE_TRADING;
                ctx->trades_completed++;
                
                // Copy received Pokemon to our slot
                // (In a full implementation, this would go to storage)
                memcpy(&ctx->our_data.party[0], 
                       &ctx->their_data.party[ctx->selected_index],
                       sizeof(PokemonParty));
                ctx->our_data.party_species[0] = 
                    ctx->their_data.party_species[ctx->selected_index];
                memcpy(&ctx->our_data.nicknames[0],
                       &ctx->their_data.nicknames[ctx->selected_index],
                       sizeof(PokemonName));
                memcpy(&ctx->our_data.ot_names[0],
                       &ctx->their_data.ot_names[ctx->selected_index],
                       sizeof(PokemonName));
                
                // Rebuild patch list with new data
                pokemon_build_patch_list(&ctx->our_patches, &ctx->our_data);
                
                printf("\n*** TRADE COMPLETE! ***\n");
                printf("Trades completed: %lu\n", ctx->trades_completed);
                
                char nickname[12];
                pokemon_string_decode(nickname, ctx->our_data.nicknames[0].str, 10);
                pokemon_print_info(&ctx->our_data.party[0], nickname);
                
                // Callback if set
                if (ctx->on_trade_complete) {
                    ctx->on_trade_complete(ctx);
                }
            }
            break;
            
        case TC_CANCEL:
            // We want to cancel - keep sending table leave until they acknowledge
            if (in == PKMN_TABLE_LEAVE) {
                ctx->tc_state = TC_RESET;
                ctx->state = TRADE_STATE_READY;
            }
            out = PKMN_TABLE_LEAVE;
            break;
    }
    
    ctx->bytes_exchanged++;
    return out;
}

//--------------------------------------------------------------------
// Main process function
//--------------------------------------------------------------------

uint8_t pokemon_trade_process(pokemon_trade_ctx *ctx, uint8_t received) {
    uint8_t response = received;  // Default: echo
    
    switch (ctx->state) {
        case TRADE_STATE_NOT_CONNECTED:
            response = handle_connection(ctx, received);
            break;
            
        case TRADE_STATE_CONNECTED:
            response = handle_menu(ctx, received);
            break;
            
        case TRADE_STATE_READY:
        case TRADE_STATE_WAITING:
        case TRADE_STATE_PENDING:
        case TRADE_STATE_TRADING:
            response = handle_trade_center(ctx, received);
            break;
            
        case TRADE_STATE_DONE:
        case TRADE_STATE_ERROR:
            // Stay in terminal state until reset
            break;
    }
    
    return response;
}

//--------------------------------------------------------------------
// Query functions
//--------------------------------------------------------------------

trade_state_t pokemon_trade_get_state(const pokemon_trade_ctx *ctx) {
    return ctx->state;
}

bool pokemon_trade_is_connected(const pokemon_trade_ctx *ctx) {
    return ctx->state >= TRADE_STATE_CONNECTED;
}

void pokemon_trade_set_callback(pokemon_trade_ctx *ctx,
                                pokemon_trade_callback_t callback,
                                void *user_context) {
    ctx->on_trade_complete = callback;
    ctx->user_context = user_context;
}

const PokemonParty* pokemon_trade_get_received(const pokemon_trade_ctx *ctx) {
    if (ctx->trades_completed > 0) {
        return &ctx->our_data.party[0];
    }
    return NULL;
}

void pokemon_trade_get_received_nickname(const pokemon_trade_ctx *ctx, char *dest, size_t len) {
    if (ctx->trades_completed > 0) {
        pokemon_string_decode(dest, ctx->our_data.nicknames[0].str, len - 1);
    } else {
        dest[0] = '\0';
    }
}
