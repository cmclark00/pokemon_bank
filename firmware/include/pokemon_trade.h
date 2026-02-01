/**
 * @file pokemon_trade.h
 * @brief Pokemon Gen I trading state machine
 * 
 * Implements the Pokemon Red/Blue/Yellow trading protocol.
 * This acts as the "follower/slave" device, letting the Game Boy
 * drive the clock and control the trade flow.
 */

#ifndef POKEMON_TRADE_H
#define POKEMON_TRADE_H

#include <stdint.h>
#include <stdbool.h>
#include "pokemon_data.h"

//--------------------------------------------------------------------
// Protocol constants
//--------------------------------------------------------------------

// Link negotiation
#define PKMN_MASTER             0x01
#define PKMN_SLAVE              0x02
#define PKMN_BLANK              0x00
#define PKMN_CONNECTED          0x60

// Menu items
#define ITEM_1_HIGHLIGHTED      0xD0
#define ITEM_2_HIGHLIGHTED      0xD1
#define ITEM_3_HIGHLIGHTED      0xD2
#define ITEM_1_SELECTED         0xD4  // Trade Centre
#define ITEM_2_SELECTED         0xD5  // Colosseum
#define ITEM_3_SELECTED         0xD6  // Cancel

// Serial protocol
#define SERIAL_PREAMBLE_BYTE    0xFD
#define SERIAL_NO_DATA_BYTE     0xFE

// Trade actions (Gen I)
#define PKMN_TRADE_ACCEPT       0x62
#define PKMN_TRADE_REJECT       0x61
#define PKMN_TABLE_LEAVE        0x6F
#define PKMN_SEL_NUM_MASK       0x60
#define PKMN_SEL_NUM_ONE        0x60

//--------------------------------------------------------------------
// State enums
//--------------------------------------------------------------------

/**
 * Overall connection state
 */
typedef enum {
    TRADE_STATE_NOT_CONNECTED,  // Waiting for Game Boy
    TRADE_STATE_CONNECTED,      // Link established
    TRADE_STATE_READY,          // In trade room, ready to trade
    TRADE_STATE_WAITING,        // Exchanged trade blocks, selecting Pokemon
    TRADE_STATE_PENDING,        // Deal offered, waiting for confirmation
    TRADE_STATE_TRADING,        // Trade in progress
    TRADE_STATE_DONE,           // Trade complete
    TRADE_STATE_ERROR           // Error occurred
} trade_state_t;

/**
 * Internal trade center sub-states
 */
typedef enum {
    TC_RESET,           // Initial/reset state
    TC_INIT,            // Counting preamble bytes
    TC_RANDOM,          // Random seed exchange
    TC_DATA,            // Trade block data exchange
    TC_PATCH_HEADER,    // Patch list header
    TC_PATCH_DATA,      // Patch list data
    TC_SELECT,          // Pokemon selection
    TC_PENDING,         // Waiting for trade confirmation
    TC_CONFIRMATION,    // Accept/reject decision
    TC_DONE,            // Trade complete, restart loop
    TC_CANCEL           // Cancel requested
} trade_center_state_t;

//--------------------------------------------------------------------
// Trade context structure
//--------------------------------------------------------------------

// Forward declaration for callback typedef
struct pokemon_trade_ctx_s;
typedef struct pokemon_trade_ctx_s pokemon_trade_ctx;

// Callback type
typedef void (*pokemon_trade_callback_t)(pokemon_trade_ctx *ctx);

struct pokemon_trade_ctx_s {
    // Current states
    trade_state_t state;
    trade_center_state_t tc_state;
    
    // Our trade data (what we're offering)
    TradeBlock our_data;
    PatchList our_patches;
    
    // Received trade data (from Game Boy)
    TradeBlock their_data;
    
    // State machine counters
    uint32_t counter;
    bool patch_part2;
    uint8_t selected_index;     // Which of their Pokemon they selected
    
    // Statistics
    uint32_t bytes_exchanged;
    uint32_t trades_completed;
    
    // Callback for trade completion
    pokemon_trade_callback_t on_trade_complete;
    void *user_context;
};

//--------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------

/**
 * Initialize the trading context with a Pokemon to offer
 * @param ctx Trade context to initialize
 * @param trainer_name Trainer name (max 7 chars)
 * @param trainer_id Trainer ID number
 */
void pokemon_trade_init(pokemon_trade_ctx *ctx, const char *trainer_name, uint16_t trainer_id);

/**
 * Reset the trade state machine
 */
void pokemon_trade_reset(pokemon_trade_ctx *ctx);

/**
 * Process a received byte and return the response byte
 * This is the main state machine - call this for every byte exchange
 * @param ctx Trade context
 * @param received Byte received from Game Boy
 * @return Byte to send back to Game Boy
 */
uint8_t pokemon_trade_process(pokemon_trade_ctx *ctx, uint8_t received);

/**
 * Get current trade state
 */
trade_state_t pokemon_trade_get_state(const pokemon_trade_ctx *ctx);

/**
 * Get human-readable state name
 */
const char* pokemon_trade_state_name(trade_state_t state);

/**
 * Check if connected to Game Boy
 */
bool pokemon_trade_is_connected(const pokemon_trade_ctx *ctx);

/**
 * Set callback for trade completion
 */
void pokemon_trade_set_callback(pokemon_trade_ctx *ctx, 
                                pokemon_trade_callback_t callback,
                                void *user_context);

/**
 * Get the Pokemon we received in the last trade
 */
const PokemonParty* pokemon_trade_get_received(const pokemon_trade_ctx *ctx);

/**
 * Get the received Pokemon's nickname
 */
void pokemon_trade_get_received_nickname(const pokemon_trade_ctx *ctx, char *dest, size_t len);

#endif // POKEMON_TRADE_H
