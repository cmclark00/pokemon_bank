/**
 * @file gb_link.c
 * @brief Game Boy Link Cable communication implementation
 */

#include "gb_link.h"
#include "gb_link.pio.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

// Timeout for considering connection lost (ms)
#define CONNECTION_TIMEOUT_MS 2000

bool gb_link_init(gb_link_t *link, PIO pio) {
    if (!link || !pio) {
        return false;
    }
    
    // Try to claim a state machine
    int sm = pio_claim_unused_sm(pio, false);
    if (sm < 0) {
        printf("[GB Link] Failed to claim state machine\n");
        return false;
    }
    
    // Add the PIO program
    if (!pio_can_add_program(pio, &gb_link_slave_program)) {
        printf("[GB Link] Failed to add PIO program\n");
        pio_sm_unclaim(pio, sm);
        return false;
    }
    
    uint offset = pio_add_program(pio, &gb_link_slave_program);
    
    // Initialize structure
    link->pio = pio;
    link->sm = sm;
    link->offset = offset;
    link->state = GB_LINK_DISCONNECTED;
    link->callback = NULL;
    link->callback_context = NULL;
    // Start with 0x00 for initial echo/sync detection.
    // The callback will return 0x02 when it sees 0x01 (MASTER).
    link->next_byte = 0x00;
    link->bytes_transferred = 0;
    link->last_transfer_time = 0;
    
    // Initialize PIO
    gb_link_slave_program_init(pio, sm, offset);
    
    printf("[GB Link] Initialized on PIO%d SM%d\n", pio == pio0 ? 0 : 1, sm);
    printf("[GB Link] Pins: SC=GP%d, SI=GP%d, SO=GP%d\n", GB_PIN_SC, GB_PIN_SI, GB_PIN_SO);
    
    return true;
}

void gb_link_start(gb_link_t *link) {
    if (!link) return;
    
    // Preload a byte into TX FIFO so we have something to send
    pio_sm_put(link->pio, link->sm, link->next_byte);
    
    // Enable the state machine
    gb_link_enable(link->pio, link->sm);
    
    link->state = GB_LINK_DISCONNECTED;  // Will become CONNECTED on first byte
    printf("[GB Link] Started, waiting for Game Boy...\n");
}

void gb_link_stop(gb_link_t *link) {
    if (!link) return;
    
    gb_link_disable(link->pio, link->sm);
    link->state = GB_LINK_DISCONNECTED;
    printf("[GB Link] Stopped\n");
}

void gb_link_reset(gb_link_t *link) {
    if (!link) return;
    
    // Reset the state machine
    pio_sm_set_enabled(link->pio, link->sm, false);
    pio_sm_clear_fifos(link->pio, link->sm);
    pio_sm_restart(link->pio, link->sm);
    pio_sm_exec(link->pio, link->sm, 
                pio_encode_jmp(link->offset + gb_link_slave_offset_entry_point));
    pio_sm_set_enabled(link->pio, link->sm, true);
    
    // Reset to echo mode for fresh connection detection
    link->next_byte = 0x00;
    pio_sm_put(link->pio, link->sm, link->next_byte);
    
    link->state = GB_LINK_DISCONNECTED;
    printf("[GB Link] Reset\n");
}

void gb_link_set_callback(gb_link_t *link, gb_link_callback_t callback, void *context) {
    if (!link) return;
    
    link->callback = callback;
    link->callback_context = context;
}

void gb_link_set_next_byte(gb_link_t *link, uint8_t byte) {
    if (!link) return;
    
    link->next_byte = byte;
    
    // If TX FIFO is empty, put the byte in
    if (pio_sm_is_tx_fifo_empty(link->pio, link->sm)) {
        pio_sm_put(link->pio, link->sm, byte);
    }
}

bool gb_link_process(gb_link_t *link) {
    if (!link) return false;
    
    // Check if we have received data
    if (!gb_link_rx_available(link->pio, link->sm)) {
        // Check for connection timeout
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (link->state != GB_LINK_DISCONNECTED && 
            link->last_transfer_time > 0 &&
            (now - link->last_transfer_time) > CONNECTION_TIMEOUT_MS) {
            link->state = GB_LINK_DISCONNECTED;
            printf("[GB Link] Connection timeout\n");
        }
        return false;
    }
    
    // Get received byte
    uint8_t received = gb_link_get_byte(link->pio, link->sm);
    link->bytes_transferred++;
    link->last_transfer_time = to_ms_since_boot(get_absolute_time());
    
    // Update connection state
    if (link->state == GB_LINK_DISCONNECTED) {
        link->state = GB_LINK_CONNECTED;
        printf("[GB Link] Connected!\n");
    }
    
    // Call callback if set
    uint8_t response = link->next_byte;
    if (link->callback) {
        response = link->callback(received, link->callback_context);
    }
    
    // Queue response for next transfer
    link->next_byte = response;
    pio_sm_put(link->pio, link->sm, response);
    
    return true;
}

int gb_link_exchange_byte(gb_link_t *link, uint8_t tx_byte, uint32_t timeout_ms) {
    if (!link) return -1;
    
    // Set byte to send
    gb_link_set_next_byte(link, tx_byte);
    
    // Wait for response
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (1) {
        if (gb_link_rx_available(link->pio, link->sm)) {
            uint8_t received = gb_link_get_byte(link->pio, link->sm);
            link->bytes_transferred++;
            link->last_transfer_time = to_ms_since_boot(get_absolute_time());
            return received;
        }
        
        if (timeout_ms > 0) {
            uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - start;
            if (elapsed >= timeout_ms) {
                return -1;  // Timeout
            }
        }
        
        // Small delay to avoid hammering the CPU
        tight_loop_contents();
    }
}

bool gb_link_is_connected(gb_link_t *link) {
    if (!link) return false;
    
    return link->state == GB_LINK_CONNECTED || link->state == GB_LINK_TRADING;
}

uint32_t gb_link_get_bytes_transferred(gb_link_t *link) {
    if (!link) return 0;
    return link->bytes_transferred;
}
