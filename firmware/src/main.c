/**
 * @file main.c
 * @brief Pokemon Bank - Main Application
 * 
 * A Pokemon storage device that acts as a "second Game Boy" for trading.
 * Phase 3: Flash storage for Pokemon persistence
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "hardware/sync.h"
#include "bsp/board.h"
#include "tusb.h"

#include "gb_link.h"
#include "pokemon_data.h"
#include "pokemon_trade.h"
#include "pokemon_storage.h"
#include "webui_protocol.h"

// RGB LED on RP2040-Zero
#define LED_PIN 16

// Default trainer configuration
#define DEFAULT_TRAINER_NAME "PKMBANK"
#define DEFAULT_TRAINER_ID   12345

// Link cable handler
static gb_link_t gb_link;

// Trade context
static pokemon_trade_ctx trade_ctx;

// Statistics
static uint32_t last_print_time = 0;
static trade_state_t last_reported_state = TRADE_STATE_NOT_CONNECTED;

//--------------------------------------------------------------------
// LED control (WS2812 on RP2040-Zero)
//--------------------------------------------------------------------

// Simple WS2812 bit-bang (good enough for single LED status)
static inline void ws2812_write_byte(uint8_t b) {
    for (int i = 7; i >= 0; i--) {
        if (b & (1 << i)) {
            // 1 bit: ~800ns high, ~450ns low
            gpio_put(LED_PIN, 1);
            __asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
            __asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
            gpio_put(LED_PIN, 0);
            __asm volatile("nop; nop; nop; nop;");
        } else {
            // 0 bit: ~400ns high, ~850ns low
            gpio_put(LED_PIN, 1);
            __asm volatile("nop; nop; nop; nop;");
            gpio_put(LED_PIN, 0);
            __asm volatile("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
        }
    }
}

static void set_led(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t save = save_and_disable_interrupts();
    // WS2812 is GRB order
    ws2812_write_byte(g);
    ws2812_write_byte(r);
    ws2812_write_byte(b);
    restore_interrupts(save);
    sleep_us(60);  // Reset pulse
}

static void led_off(void) {
    set_led(0, 0, 0);
}

static void led_idle(void) {
    set_led(0, 0, 32);  // Dim blue - waiting
}

static void led_connected(void) {
    set_led(0, 64, 0);  // Green - connected
}

static void led_waiting(void) {
    set_led(0, 32, 32);  // Cyan - in trade room
}

static void led_pending(void) {
    set_led(64, 32, 0);  // Orange - pending trade
}

static void led_trading(void) {
    set_led(64, 0, 64);  // Purple - actively trading
}

static void led_activity(void) {
    set_led(64, 64, 0);  // Yellow - data activity
}

static void led_error(void) {
    set_led(64, 0, 0);  // Red - error
}

static void led_success(void) {
    set_led(0, 64, 32);  // Bright green - trade complete
}

//--------------------------------------------------------------------
// Update LED based on trade state
//--------------------------------------------------------------------

static void update_led_for_state(trade_state_t state) {
    switch (state) {
        case TRADE_STATE_NOT_CONNECTED:
            led_idle();
            break;
        case TRADE_STATE_CONNECTED:
            led_connected();
            break;
        case TRADE_STATE_READY:
            led_waiting();
            break;
        case TRADE_STATE_WAITING:
            led_waiting();
            break;
        case TRADE_STATE_PENDING:
            led_pending();
            break;
        case TRADE_STATE_TRADING:
            led_trading();
            break;
        case TRADE_STATE_DONE:
            led_success();
            break;
        case TRADE_STATE_ERROR:
            led_error();
            break;
        default:
            led_idle();
            break;
    }
}

//--------------------------------------------------------------------
// Link callback - handles each byte received from Game Boy
//--------------------------------------------------------------------

static uint8_t link_callback(uint8_t received, void *context) {
    (void)context;
    
    // Process through trading state machine
    uint8_t response = pokemon_trade_process(&trade_ctx, received);
    
    // Check for state changes
    trade_state_t current_state = pokemon_trade_get_state(&trade_ctx);
    if (current_state != last_reported_state) {
        printf("[Trade] State: %s -> %s\n", 
               pokemon_trade_state_name(last_reported_state),
               pokemon_trade_state_name(current_state));
        last_reported_state = current_state;
        
        // Update LED for new state
        update_led_for_state(current_state);
    }
    
    return response;
}

//--------------------------------------------------------------------
// Trade completion callback
//--------------------------------------------------------------------

static void on_trade_complete(pokemon_trade_ctx *ctx) {
    printf("\n=== TRADE COMPLETED ===\n");
    printf("Total trades: %lu\n", ctx->trades_completed);
    printf("Bytes exchanged: %lu\n", ctx->bytes_exchanged);
    
    // Store the received Pokemon to flash
    if (pokemon_storage_is_full()) {
        printf("WARNING: Storage is full! Pokemon not saved.\n");
        // Flash red to indicate storage full
        for (int i = 0; i < 3; i++) {
            led_error();
            sleep_ms(200);
            led_off();
            sleep_ms(200);
        }
    } else {
        // Get the received Pokemon data
        const PokemonParty *received = &ctx->our_data.party[0];
        const uint8_t *nickname = ctx->our_data.nicknames[0].str;
        const uint8_t *ot_name = ctx->our_data.ot_names[0].str;
        
        storage_status_t status = pokemon_storage_add(received, nickname, ot_name);
        
        if (status == STORAGE_OK) {
            printf("Pokemon saved to storage! (%d/%d slots used)\n",
                   pokemon_storage_count(), STORAGE_BOX_SIZE);
            
            // Flash LED to indicate success
            for (int i = 0; i < 3; i++) {
                led_success();
                sleep_ms(200);
                led_off();
                sleep_ms(200);
            }
        } else {
            printf("ERROR: Failed to save Pokemon: %s\n",
                   pokemon_storage_status_name(status));
            led_error();
            sleep_ms(500);
        }
    }
}

//--------------------------------------------------------------------
// USB CDC handling
//--------------------------------------------------------------------

// JSON command buffer
static char json_buffer[JSON_BUFFER_SIZE];
static size_t json_buffer_len = 0;
static int json_brace_depth = 0;
static bool json_in_string = false;

/**
 * Process single-character legacy commands
 */
static void process_legacy_command(char cmd) {
    switch (cmd) {
        case 'r':  // Reset
        case 'R':
            printf("Resetting trade state...\n");
            pokemon_trade_reset(&trade_ctx);
            gb_link_reset(&gb_link);
            last_reported_state = TRADE_STATE_NOT_CONNECTED;
            break;
            
        case 's':  // Status
        case 'S':
            printf("\n=== Pokemon Bank Status ===\n");
            printf("Trade state: %s\n", 
                   pokemon_trade_state_name(pokemon_trade_get_state(&trade_ctx)));
            printf("Link state: %s\n", 
                gb_link.state == GB_LINK_DISCONNECTED ? "Disconnected" :
                gb_link.state == GB_LINK_CONNECTED ? "Connected" :
                gb_link.state == GB_LINK_TRADING ? "Trading" : "Error");
            printf("Link bytes transferred: %lu\n", gb_link.bytes_transferred);
            printf("Trade bytes exchanged: %lu\n", trade_ctx.bytes_exchanged);
            printf("Trades completed: %lu\n", trade_ctx.trades_completed);
            printf("SC pin (clock): %d\n", gpio_get(GB_PIN_SC));
            printf("SO pin (from GB): %d\n", gpio_get(GB_PIN_SO));
            printf("Storage: %d/%d Pokemon\n", 
                   pokemon_storage_count(), STORAGE_BOX_SIZE);
            break;
            
        case 'p':  // Print current Pokemon
        case 'P':
            {
                printf("\n=== Current Pokemon to Trade ===\n");
                char nickname[12];
                pokemon_string_decode(nickname, trade_ctx.our_data.nicknames[0].str, 10);
                pokemon_print_info(&trade_ctx.our_data.party[0], nickname);
            }
            break;
            
        case 'd':  // Debug - dump trade block (first 64 bytes)
        case 'D':
            printf("\n=== Trade Block (first 64 bytes) ===\n");
            for (int j = 0; j < 64; j++) {
                printf("%02X ", ((uint8_t*)&trade_ctx.our_data)[j]);
                if ((j + 1) % 16 == 0) printf("\n");
            }
            printf("\n");
            break;
            
        case 'l':  // List stored Pokemon
        case 'L':
            pokemon_storage_print();
            break;
            
        case 'c':  // Clear storage (requires confirmation)
        case 'C':
            printf("\n*** WARNING: This will delete all stored Pokemon! ***\n");
            printf("Press 'y' to confirm, any other key to cancel.\n");
            // Wait for next character
            while (!tud_cdc_available()) {
                tud_task();
            }
            {
                char confirm;
                tud_cdc_read(&confirm, 1);
                if (confirm == 'y' || confirm == 'Y') {
                    storage_status_t status = pokemon_storage_clear();
                    if (status == STORAGE_OK) {
                        printf("Storage cleared!\n");
                    } else {
                        printf("Error clearing storage: %s\n",
                               pokemon_storage_status_name(status));
                    }
                } else {
                    printf("Clear cancelled.\n");
                }
            }
            break;
            
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // View specific Pokemon slot
            {
                uint8_t slot = cmd - '0';
                if (slot < pokemon_storage_count()) {
                    const PokemonSlot *pslot = pokemon_storage_get_slot(slot);
                    if (pslot) {
                        printf("\n=== Slot %d ===\n", slot);
                        char nickname[12];
                        pokemon_string_decode(nickname, pslot->nickname, 10);
                        pokemon_print_info(&pslot->party, nickname);
                    }
                } else {
                    printf("Slot %d is empty (have %d Pokemon)\n",
                           slot, pokemon_storage_count());
                }
            }
            break;
            
        case 'x':  // Remove Pokemon from slot (asks for slot number)
        case 'X':
            printf("Enter slot number to remove (0-9): ");
            while (!tud_cdc_available()) {
                tud_task();
            }
            {
                char slot_char;
                tud_cdc_read(&slot_char, 1);
                printf("%c\n", slot_char);
                
                if (slot_char >= '0' && slot_char <= '9') {
                    uint8_t slot = slot_char - '0';
                    if (slot < pokemon_storage_count()) {
                        storage_status_t status = pokemon_storage_remove(slot);
                        if (status == STORAGE_OK) {
                            printf("Removed Pokemon from slot %d\n", slot);
                        } else {
                            printf("Error: %s\n", 
                                   pokemon_storage_status_name(status));
                        }
                    } else {
                        printf("Invalid slot %d (have %d Pokemon)\n",
                               slot, pokemon_storage_count());
                    }
                } else {
                    printf("Invalid input, cancelled.\n");
                }
            }
            break;
            
        case 'b':  // Bootsel mode (for firmware update)
        case 'B':
            printf("Entering BOOTSEL mode...\n");
            sleep_ms(100);
            reset_usb_boot(0, 0);
            break;
            
        case 'g':  // GPIO diagnostic
        case 'G':
            printf("\n=== GPIO Diagnostic ===\n");
            printf("GP0 (SC/Clock): %d (expect HIGH when idle)\n", gpio_get(GB_PIN_SC));
            printf("GP1 (SI/to GB): %d (our output)\n", gpio_get(GB_PIN_SI));
            printf("GP2 (SO/from GB): %d (expect HIGH when idle)\n", gpio_get(GB_PIN_SO));
            printf("\nIf SC and SO are LOW when GB disconnected, check:\n");
            printf("- Pull-up resistors on the board\n");
            printf("- Level shifter connections\n");
            printf("- Cable connection\n\n");
            break;
            
        case 'h':  // Help
        case 'H':
        case '?':
            printf("\n=== Pokemon Bank Commands ===\n");
            printf("s - Status\n");
            printf("r - Reset link and trade state\n");
            printf("p - Print current Pokemon to trade\n");
            printf("l - List stored Pokemon\n");
            printf("0-9 - View Pokemon in slot\n");
            printf("x - Remove Pokemon from slot\n");
            printf("c - Clear all stored Pokemon\n");
            printf("d - Debug: dump trade block\n");
            printf("g - GPIO pin diagnostic\n");
            printf("w - Watch pins (continuous, any key to stop)\n");
            printf("b - Enter BOOTSEL mode\n");
            printf("h - This help\n");
            printf("\nJSON commands also supported for WebUI.\n\n");
            break;
            
        case 'w':  // Watch pins continuously
        case 'W':
            printf("Watching pins (press any key to stop)...\n");
            printf("SC=Clock, SO=DataFromGB, SI=DataToGB\n");
            {
                int prev_sc = -1, prev_so = -1;
                uint32_t count = 0;
                while (!tud_cdc_available()) {
                    tud_task();
                    int sc = gpio_get(GB_PIN_SC);
                    int so = gpio_get(GB_PIN_SO);
                    if (sc != prev_sc || so != prev_so) {
                        printf("[%lu] SC=%d SO=%d\n", count++, sc, so);
                        prev_sc = sc;
                        prev_so = so;
                    }
                    sleep_us(10);  // Sample fast
                }
                // Consume the key that stopped us
                char c; tud_cdc_read(&c, 1);
                printf("Stopped.\n");
            }
            break;
            
        case '\r':
        case '\n':
            // Ignore newlines
            break;
            
        default:
            // Silently ignore unknown single chars when they could be part of JSON
            if (json_buffer_len == 0) {
                printf("Unknown command: '%c' (0x%02X). Press 'h' for help.\n", 
                       cmd, cmd);
            }
            break;
    }
}

/**
 * Process JSON command and send response
 */
static void process_json_command(void) {
    static char response_buffer[JSON_BUFFER_SIZE];
    
    json_buffer[json_buffer_len] = '\0';  // Null terminate
    
    if (webui_process_command(json_buffer, response_buffer, sizeof(response_buffer))) {
        // Send response with newline
        printf("%s\n", response_buffer);
    }
    
    // Reset JSON buffer
    json_buffer_len = 0;
    json_brace_depth = 0;
    json_in_string = false;
}

static void cdc_task(void) {
    if (!tud_cdc_connected()) return;
    
    // Check for incoming data
    if (tud_cdc_available()) {
        char buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        
        for (uint32_t i = 0; i < count; i++) {
            char c = buf[i];
            
            // Check if we're collecting JSON
            if (json_buffer_len > 0 || c == '{') {
                // Add to JSON buffer
                if (json_buffer_len < JSON_BUFFER_SIZE - 1) {
                    json_buffer[json_buffer_len++] = c;
                    
                    // Track brace depth (simplified - doesn't handle escaped quotes)
                    if (c == '"' && (json_buffer_len < 2 || json_buffer[json_buffer_len - 2] != '\\')) {
                        json_in_string = !json_in_string;
                    } else if (!json_in_string) {
                        if (c == '{') json_brace_depth++;
                        else if (c == '}') json_brace_depth--;
                    }
                    
                    // Check if JSON is complete
                    if (json_brace_depth == 0 && json_buffer_len > 0) {
                        process_json_command();
                    }
                } else {
                    // Buffer overflow - reset
                    printf("{\"error\":\"JSON too long\"}\n");
                    json_buffer_len = 0;
                    json_brace_depth = 0;
                    json_in_string = false;
                }
            } else {
                // Legacy single-character command
                process_legacy_command(c);
            }
        }
    }
}

//--------------------------------------------------------------------
// Main
//--------------------------------------------------------------------

int main(void) {
    // Initialize stdio (for printf over USB)
    stdio_init_all();
    
    // Initialize board (TinyUSB needs this)
    board_init();
    
    // Initialize TinyUSB
    tusb_init();
    
    // Initialize LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    led_idle();
    
    // Wait a moment for USB to enumerate
    sleep_ms(1000);
    
    printf("\n\n");
    printf("======================================\n");
    printf("   Pokemon Bank v0.3\n");
    printf("   Phase 3: Flash Storage\n");
    printf("======================================\n\n");
    
    // Initialize storage system (loads Pokemon from flash)
    storage_status_t storage_status = pokemon_storage_init();
    if (storage_status != STORAGE_OK) {
        printf("WARNING: Storage init failed: %s\n",
               pokemon_storage_status_name(storage_status));
    } else {
        printf("Storage: %d/%d Pokemon loaded from flash\n",
               pokemon_storage_count(), STORAGE_BOX_SIZE);
    }
    
    // Initialize trading state machine with default trainer
    pokemon_trade_init(&trade_ctx, DEFAULT_TRAINER_NAME, DEFAULT_TRAINER_ID);
    pokemon_trade_set_callback(&trade_ctx, on_trade_complete, NULL);
    
    printf("Trainer: %s (ID: %d)\n", DEFAULT_TRAINER_NAME, DEFAULT_TRAINER_ID);
    printf("Offering: Bulbasaur Lv.5\n\n");
    
    // Initialize Game Boy link
    if (!gb_link_init(&gb_link, pio0)) {
        printf("ERROR: Failed to initialize GB Link!\n");
        led_error();
        while (1) {
            tight_loop_contents();
        }
    }
    
    // Set up callback
    gb_link_set_callback(&gb_link, link_callback, NULL);
    
    // Start the link (begin listening for Game Boy)
    gb_link_start(&gb_link);
    
    printf("Press 'h' for command help.\n");
    printf("Connect your Game Boy and enter the trade room!\n\n");
    printf("LED colors:\n");
    printf("  Blue   = Waiting for connection\n");
    printf("  Green  = Connected to Game Boy\n");
    printf("  Cyan   = In trade room\n");
    printf("  Orange = Trade pending\n");
    printf("  Purple = Trading in progress\n");
    printf("  Bright Green = Trade complete!\n");
    printf("  Red    = Error\n\n");
    printf("Pin states at boot: SC=%d SI=%d SO=%d\n", 
           gpio_get(GB_PIN_SC), gpio_get(GB_PIN_SI), gpio_get(GB_PIN_SO));
    
    // Main loop
    uint32_t last_led_update = 0;
    bool activity_flash = false;
    
    while (1) {
        // Process USB
        tud_task();
        cdc_task();
        
        // Process Game Boy link
        bool activity = gb_link_process(&gb_link);
        
        // Track activity for LED flashing
        if (activity) {
            activity_flash = true;
            last_led_update = to_ms_since_boot(get_absolute_time());
        }
        
        // Update LED based on state (every 100ms)
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_led_update > 100) {
            last_led_update = now;
            
            if (activity_flash) {
                // Brief yellow flash for activity
                led_activity();
                activity_flash = false;
            } else {
                // Show state color
                update_led_for_state(pokemon_trade_get_state(&trade_ctx));
            }
        }
        
        // Periodic status (every 10 seconds if connected)
        static uint32_t last_status_time = 0;
        if (pokemon_trade_is_connected(&trade_ctx) && (now - last_status_time > 10000)) {
            last_status_time = now;
            printf("[Status] %s, %lu bytes, %lu trades\n", 
                   pokemon_trade_state_name(pokemon_trade_get_state(&trade_ctx)),
                   trade_ctx.bytes_exchanged,
                   trade_ctx.trades_completed);
        }
    }
    
    return 0;
}
