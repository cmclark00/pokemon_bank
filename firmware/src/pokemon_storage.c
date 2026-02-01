/**
 * @file pokemon_storage.c
 * @brief Pokemon Box Storage System - Flash Persistence
 * 
 * Uses RP2040 flash for persistent Pokemon storage.
 * Flash operations require interrupts disabled.
 */

#include "pokemon_storage.h"
#include <stdio.h>
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

//--------------------------------------------------------------------
// Flash memory mapping
//--------------------------------------------------------------------

// Flash is memory-mapped at XIP_BASE (0x10000000)
// We use the last sector to avoid program collision
#define FLASH_TARGET_OFFSET  STORAGE_FLASH_OFFSET
#define FLASH_TARGET_ADDR    (XIP_BASE + FLASH_TARGET_OFFSET)

//--------------------------------------------------------------------
// Module state
//--------------------------------------------------------------------

// RAM copy of storage (we modify this, then write to flash)
static StorageData storage_ram;
static bool storage_initialized = false;
static bool storage_dirty = false;  // RAM differs from flash

//--------------------------------------------------------------------
// CRC32 implementation (simple, no lookup table)
//--------------------------------------------------------------------

static uint32_t crc32_byte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xEDB88320;
        } else {
            crc >>= 1;
        }
    }
    return crc;
}

static uint32_t crc32_buffer(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_byte(crc, data[i]);
    }
    return crc ^ 0xFFFFFFFF;
}

//--------------------------------------------------------------------
// Internal helpers
//--------------------------------------------------------------------

/**
 * Calculate checksum of storage data (excludes header)
 */
static uint32_t calculate_checksum(const StorageData *data) {
    // Checksum covers slots only
    return crc32_buffer((const uint8_t *)data->slots, sizeof(data->slots));
}

/**
 * Verify storage data integrity
 */
static bool verify_storage(const StorageData *data) {
    // Check magic
    if (data->header.magic != STORAGE_MAGIC) {
        return false;
    }
    
    // Check version
    if (data->header.version != STORAGE_VERSION) {
        return false;
    }
    
    // Check count is sane
    if (data->header.count > STORAGE_BOX_SIZE) {
        return false;
    }
    
    // Verify checksum
    uint32_t calc_checksum = calculate_checksum(data);
    if (data->header.checksum != calc_checksum) {
        return false;
    }
    
    return true;
}

/**
 * Initialize empty storage
 */
static void init_empty_storage(void) {
    memset(&storage_ram, 0, sizeof(storage_ram));
    storage_ram.header.magic = STORAGE_MAGIC;
    storage_ram.header.version = STORAGE_VERSION;
    storage_ram.header.count = 0;
    storage_ram.header.write_count = 0;
    storage_ram.header.checksum = calculate_checksum(&storage_ram);
}

/**
 * Write storage to flash
 * Must be called with interrupts disabled or from main thread only
 */
static storage_status_t write_to_flash(void) {
    // Update checksum before writing
    storage_ram.header.checksum = calculate_checksum(&storage_ram);
    storage_ram.header.write_count++;
    
    printf("[Storage] Writing to flash (write #%lu)...\n", 
           storage_ram.header.write_count);
    
    // Disable interrupts - flash operations are sensitive
    uint32_t ints = save_and_disable_interrupts();
    
    // Erase the sector (required before writing)
    flash_range_erase(FLASH_TARGET_OFFSET, STORAGE_SECTOR_SIZE);
    
    // Write our data (must be multiple of FLASH_PAGE_SIZE = 256 bytes)
    // Round up to page boundary
    size_t write_size = (sizeof(StorageData) + FLASH_PAGE_SIZE - 1) 
                        & ~(FLASH_PAGE_SIZE - 1);
    flash_range_program(FLASH_TARGET_OFFSET, 
                        (const uint8_t *)&storage_ram, 
                        write_size);
    
    restore_interrupts(ints);
    
    // Verify the write by reading back
    const StorageData *flash_data = (const StorageData *)FLASH_TARGET_ADDR;
    if (memcmp(&storage_ram, flash_data, sizeof(StorageData)) != 0) {
        printf("[Storage] ERROR: Flash verify failed!\n");
        return STORAGE_ERR_FLASH_WRITE;
    }
    
    storage_dirty = false;
    printf("[Storage] Write complete, %d Pokemon stored\n", 
           storage_ram.header.count);
    
    return STORAGE_OK;
}

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------

storage_status_t pokemon_storage_init(void) {
    printf("[Storage] Initializing from flash at 0x%08X...\n", 
           FLASH_TARGET_ADDR);
    
    // Read from flash (memory-mapped)
    const StorageData *flash_data = (const StorageData *)FLASH_TARGET_ADDR;
    
    // Check if flash contains valid data
    if (verify_storage(flash_data)) {
        // Copy to RAM
        memcpy(&storage_ram, flash_data, sizeof(StorageData));
        printf("[Storage] Loaded %d Pokemon from flash (writes: %lu)\n", 
               storage_ram.header.count, storage_ram.header.write_count);
    } else {
        // Flash is empty/corrupt - initialize fresh
        printf("[Storage] No valid data in flash, initializing empty storage\n");
        init_empty_storage();
        
        // Write initial empty storage to flash
        storage_status_t status = write_to_flash();
        if (status != STORAGE_OK) {
            return status;
        }
    }
    
    storage_initialized = true;
    storage_dirty = false;
    
    return STORAGE_OK;
}

uint8_t pokemon_storage_count(void) {
    if (!storage_initialized) return 0;
    return storage_ram.header.count;
}

bool pokemon_storage_is_full(void) {
    return storage_ram.header.count >= STORAGE_BOX_SIZE;
}

storage_status_t pokemon_storage_add(const PokemonParty *party,
                                     const uint8_t *nickname,
                                     const uint8_t *ot_name) {
    if (!storage_initialized) {
        return STORAGE_ERR_NOT_INIT;
    }
    
    if (storage_ram.header.count >= STORAGE_BOX_SIZE) {
        printf("[Storage] ERROR: Box full!\n");
        return STORAGE_ERR_FULL;
    }
    
    // Find next slot
    uint8_t slot = storage_ram.header.count;
    
    // Copy Pokemon data
    memcpy(&storage_ram.slots[slot].party, party, sizeof(PokemonParty));
    memcpy(storage_ram.slots[slot].nickname, nickname, PKMN_NAME_LENGTH);
    memcpy(storage_ram.slots[slot].ot_name, ot_name, PKMN_NAME_LENGTH);
    
    storage_ram.header.count++;
    storage_dirty = true;
    
    // Decode nickname for logging
    char nick_str[12];
    pokemon_string_decode(nick_str, nickname, 10);
    printf("[Storage] Added %s to slot %d\n", nick_str, slot);
    
    // Save immediately to flash
    return write_to_flash();
}

storage_status_t pokemon_storage_get(uint8_t slot,
                                     PokemonParty *party,
                                     uint8_t *nickname,
                                     uint8_t *ot_name) {
    if (!storage_initialized) {
        return STORAGE_ERR_NOT_INIT;
    }
    
    if (slot >= storage_ram.header.count) {
        return STORAGE_ERR_INVALID_SLOT;
    }
    
    if (party) {
        memcpy(party, &storage_ram.slots[slot].party, sizeof(PokemonParty));
    }
    if (nickname) {
        memcpy(nickname, storage_ram.slots[slot].nickname, PKMN_NAME_LENGTH);
    }
    if (ot_name) {
        memcpy(ot_name, storage_ram.slots[slot].ot_name, PKMN_NAME_LENGTH);
    }
    
    return STORAGE_OK;
}

storage_status_t pokemon_storage_remove(uint8_t slot) {
    if (!storage_initialized) {
        return STORAGE_ERR_NOT_INIT;
    }
    
    if (slot >= storage_ram.header.count) {
        return STORAGE_ERR_INVALID_SLOT;
    }
    
    // Shift remaining Pokemon down
    for (uint8_t i = slot; i < storage_ram.header.count - 1; i++) {
        memcpy(&storage_ram.slots[i], &storage_ram.slots[i + 1], 
               sizeof(PokemonSlot));
    }
    
    // Clear last slot
    memset(&storage_ram.slots[storage_ram.header.count - 1], 0, 
           sizeof(PokemonSlot));
    
    storage_ram.header.count--;
    storage_dirty = true;
    
    printf("[Storage] Removed slot %d, %d Pokemon remaining\n", 
           slot, storage_ram.header.count);
    
    return write_to_flash();
}

storage_status_t pokemon_storage_update(uint8_t slot,
                                        const PokemonParty *party,
                                        const uint8_t *nickname,
                                        const uint8_t *ot_name) {
    if (!storage_initialized) {
        return STORAGE_ERR_NOT_INIT;
    }
    
    if (slot >= storage_ram.header.count) {
        return STORAGE_ERR_INVALID_SLOT;
    }
    
    // Update only provided fields
    if (party) {
        memcpy(&storage_ram.slots[slot].party, party, sizeof(PokemonParty));
    }
    if (nickname) {
        memcpy(storage_ram.slots[slot].nickname, nickname, PKMN_NAME_LENGTH);
    }
    if (ot_name) {
        memcpy(storage_ram.slots[slot].ot_name, ot_name, PKMN_NAME_LENGTH);
    }
    
    storage_dirty = true;
    
    // Decode nickname for logging
    char nick_str[12];
    pokemon_string_decode(nick_str, storage_ram.slots[slot].nickname, 10);
    printf("[Storage] Updated slot %d (%s)\n", slot, nick_str);
    
    return write_to_flash();
}

storage_status_t pokemon_storage_clear(void) {
    if (!storage_initialized) {
        return STORAGE_ERR_NOT_INIT;
    }
    
    printf("[Storage] Clearing all Pokemon...\n");
    
    // Keep write count
    uint32_t write_count = storage_ram.header.write_count;
    
    // Reset to empty
    init_empty_storage();
    storage_ram.header.write_count = write_count;
    
    return write_to_flash();
}

storage_status_t pokemon_storage_save(void) {
    if (!storage_initialized) {
        return STORAGE_ERR_NOT_INIT;
    }
    
    if (!storage_dirty) {
        printf("[Storage] No changes to save\n");
        return STORAGE_OK;
    }
    
    return write_to_flash();
}

void pokemon_storage_stats(uint32_t *write_count, bool *checksum_valid) {
    if (write_count) {
        *write_count = storage_initialized ? 
                       storage_ram.header.write_count : 0;
    }
    
    if (checksum_valid) {
        if (storage_initialized) {
            uint32_t calc = calculate_checksum(&storage_ram);
            *checksum_valid = (calc == storage_ram.header.checksum);
        } else {
            *checksum_valid = false;
        }
    }
}

void pokemon_storage_print(void) {
    if (!storage_initialized) {
        printf("[Storage] Not initialized!\n");
        return;
    }
    
    printf("\n=== Pokemon Storage ===\n");
    printf("Pokemon stored: %d / %d\n", 
           storage_ram.header.count, STORAGE_BOX_SIZE);
    printf("Flash writes: %lu\n", storage_ram.header.write_count);
    
    uint32_t calc_checksum = calculate_checksum(&storage_ram);
    printf("Checksum: 0x%08lX (%s)\n", 
           storage_ram.header.checksum,
           calc_checksum == storage_ram.header.checksum ? "OK" : "MISMATCH!");
    
    if (storage_ram.header.count == 0) {
        printf("(No Pokemon stored)\n");
    } else {
        printf("\nStored Pokemon:\n");
        for (uint8_t i = 0; i < storage_ram.header.count; i++) {
            const PokemonSlot *slot = &storage_ram.slots[i];
            char nickname[12], ot_name[12];
            
            pokemon_string_decode(nickname, slot->nickname, 10);
            pokemon_string_decode(ot_name, slot->ot_name, 10);
            
            const char *species = pokemon_get_species_name(slot->party.index);
            
            printf("  [%2d] %s (%s) Lv.%d - OT: %s\n",
                   i,
                   nickname,
                   species,
                   slot->party.level,
                   ot_name);
        }
    }
    printf("\n");
}

const PokemonSlot* pokemon_storage_get_slot(uint8_t slot) {
    if (!storage_initialized || slot >= storage_ram.header.count) {
        return NULL;
    }
    return &storage_ram.slots[slot];
}

const char* pokemon_storage_status_name(storage_status_t status) {
    switch (status) {
        case STORAGE_OK:             return "OK";
        case STORAGE_ERR_FULL:       return "Box Full";
        case STORAGE_ERR_EMPTY:      return "Empty";
        case STORAGE_ERR_INVALID_SLOT: return "Invalid Slot";
        case STORAGE_ERR_FLASH_WRITE: return "Flash Write Error";
        case STORAGE_ERR_CORRUPT:    return "Corrupt Data";
        case STORAGE_ERR_NOT_INIT:   return "Not Initialized";
        default:                     return "Unknown";
    }
}
