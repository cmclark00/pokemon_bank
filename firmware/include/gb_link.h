/**
 * @file gb_link.h
 * @brief Game Boy Link Cable communication for RP2040
 * 
 * This module handles the low-level serial communication with a Game Boy
 * via the link cable. Uses PIO for reliable timing with external clock.
 */

#ifndef GB_LINK_H
#define GB_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/pio.h"

// Pin definitions for Game Boy Zero Link Board
#define GB_PIN_SC   0   // Serial Clock (from Game Boy)
#define GB_PIN_SI   1   // Serial In (TO Game Boy)
#define GB_PIN_SO   2   // Serial Out (FROM Game Boy)
#define GB_PIN_SD   3   // SD line (directly directly directly

// Connection states
typedef enum {
    GB_LINK_DISCONNECTED,
    GB_LINK_CONNECTED,
    GB_LINK_TRADING,
    GB_LINK_ERROR
} gb_link_state_t;

// Transfer callback type - called when a byte is exchanged
// Parameters: byte received from Game Boy, context pointer
// Returns: byte to send on next transfer (or use gb_link_set_next_byte)
typedef uint8_t (*gb_link_callback_t)(uint8_t received, void *context);

// Link handle structure
typedef struct {
    PIO pio;
    uint sm;
    uint offset;
    gb_link_state_t state;
    gb_link_callback_t callback;
    void *callback_context;
    uint8_t next_byte;      // Byte to send on next transfer
    uint32_t bytes_transferred;
    uint32_t last_transfer_time;
} gb_link_t;

/**
 * Initialize the Game Boy link interface
 * 
 * @param link Pointer to link structure to initialize
 * @param pio PIO instance to use (pio0 or pio1)
 * @return true if initialization successful
 */
bool gb_link_init(gb_link_t *link, PIO pio);

/**
 * Start the link interface (enable state machine)
 */
void gb_link_start(gb_link_t *link);

/**
 * Stop the link interface (disable state machine)
 */
void gb_link_stop(gb_link_t *link);

/**
 * Reset the link interface (clear FIFOs, restart)
 */
void gb_link_reset(gb_link_t *link);

/**
 * Set callback for received bytes
 * 
 * @param link Link handle
 * @param callback Function to call when byte received
 * @param context User context passed to callback
 */
void gb_link_set_callback(gb_link_t *link, gb_link_callback_t callback, void *context);

/**
 * Set the byte to send on next transfer
 * Use this to preload the response byte before the Game Boy initiates transfer
 * 
 * @param link Link handle
 * @param byte Byte to send
 */
void gb_link_set_next_byte(gb_link_t *link, uint8_t byte);

/**
 * Process link activity (call this regularly from main loop)
 * Checks for received bytes and calls callback if set
 * 
 * @param link Link handle
 * @return true if a byte was processed
 */
bool gb_link_process(gb_link_t *link);

/**
 * Exchange a single byte (blocking)
 * Waits for the Game Boy to initiate transfer
 * 
 * @param link Link handle
 * @param tx_byte Byte to send to Game Boy
 * @param timeout_ms Timeout in milliseconds (0 = forever)
 * @return Byte received from Game Boy, or -1 on timeout
 */
int gb_link_exchange_byte(gb_link_t *link, uint8_t tx_byte, uint32_t timeout_ms);

/**
 * Check if connected (based on recent activity)
 * 
 * @param link Link handle
 * @return true if Game Boy appears connected
 */
bool gb_link_is_connected(gb_link_t *link);

/**
 * Get statistics
 */
uint32_t gb_link_get_bytes_transferred(gb_link_t *link);

#endif // GB_LINK_H
