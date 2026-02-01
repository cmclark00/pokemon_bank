/**
 * @file pokemon_storage.h
 * @brief Pokemon Box Storage System - Flash Persistence
 * 
 * Implements persistent storage for traded Pokemon using RP2040 flash.
 * Uses the last sectors of flash to avoid overwriting the program.
 * 
 * Storage Layout (4KB sector):
 * ├── Header (16 bytes): magic, version, count, checksum
 * ├── Pokemon[0..19] (44 bytes each = 880 bytes)
 * ├── Nicknames[0..19] (11 bytes each = 220 bytes)
 * ├── OT Names[0..19] (11 bytes each = 220 bytes)
 * └── Padding to 4KB
 */

#ifndef POKEMON_STORAGE_H
#define POKEMON_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "pokemon_data.h"

//--------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------

#define STORAGE_MAGIC           0x504B4D42  // "PKMB" in little-endian
#define STORAGE_VERSION         1
#define STORAGE_BOX_SIZE        20          // Pokemon per box
#define STORAGE_SECTOR_SIZE     4096        // Flash sector size
#define STORAGE_FLASH_OFFSET    (2 * 1024 * 1024 - STORAGE_SECTOR_SIZE)  // Last 4KB of 2MB flash

//--------------------------------------------------------------------
// Storage Structures
//--------------------------------------------------------------------

/**
 * Storage header (16 bytes)
 */
typedef struct __attribute__((__packed__)) {
    uint32_t magic;         // Should be STORAGE_MAGIC
    uint16_t version;       // Storage format version
    uint8_t  count;         // Number of Pokemon stored (0-20)
    uint8_t  reserved;      // Reserved for future use
    uint32_t checksum;      // CRC32 of data following header
    uint32_t write_count;   // Number of writes (for wear tracking)
} StorageHeader;

/**
 * Pokemon slot data (66 bytes per Pokemon)
 * Contains party data + nickname + OT name
 */
typedef struct __attribute__((__packed__)) {
    PokemonParty party;                     // 44 bytes
    uint8_t nickname[PKMN_NAME_LENGTH];     // 11 bytes
    uint8_t ot_name[PKMN_NAME_LENGTH];      // 11 bytes
} PokemonSlot;

/**
 * Complete storage structure (fits in 4KB sector)
 * Header (16) + Slots (66*20=1320) = 1336 bytes
 */
typedef struct __attribute__((__packed__)) {
    StorageHeader header;
    PokemonSlot slots[STORAGE_BOX_SIZE];
} StorageData;

// Verify structure fits in one sector
_Static_assert(sizeof(StorageData) <= STORAGE_SECTOR_SIZE, 
               "StorageData must fit in one flash sector");

//--------------------------------------------------------------------
// Status codes
//--------------------------------------------------------------------

typedef enum {
    STORAGE_OK = 0,
    STORAGE_ERR_FULL,           // Box is full (20 Pokemon)
    STORAGE_ERR_EMPTY,          // No Pokemon stored
    STORAGE_ERR_INVALID_SLOT,   // Slot index out of range
    STORAGE_ERR_FLASH_WRITE,    // Flash write failed
    STORAGE_ERR_CORRUPT,        // Checksum mismatch
    STORAGE_ERR_NOT_INIT        // Storage not initialized
} storage_status_t;

//--------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------

/**
 * Initialize storage system - loads from flash or creates empty storage
 * Must be called before any other storage functions
 * @return STORAGE_OK on success, error code otherwise
 */
storage_status_t pokemon_storage_init(void);

/**
 * Get number of Pokemon currently stored
 * @return Count (0-20)
 */
uint8_t pokemon_storage_count(void);

/**
 * Check if storage is full
 * @return true if 20 Pokemon stored
 */
bool pokemon_storage_is_full(void);

/**
 * Store a Pokemon in the next available slot
 * @param party Pokemon party data (44 bytes)
 * @param nickname Pokemon nickname (encoded, 11 bytes)
 * @param ot_name Original trainer name (encoded, 11 bytes)
 * @return STORAGE_OK on success, STORAGE_ERR_FULL if box full
 */
storage_status_t pokemon_storage_add(const PokemonParty *party,
                                     const uint8_t *nickname,
                                     const uint8_t *ot_name);

/**
 * Get Pokemon from a specific slot
 * @param slot Slot index (0-19)
 * @param party Output: Pokemon party data
 * @param nickname Output: Nickname (11 bytes)
 * @param ot_name Output: OT name (11 bytes)
 * @return STORAGE_OK on success, error code otherwise
 */
storage_status_t pokemon_storage_get(uint8_t slot,
                                     PokemonParty *party,
                                     uint8_t *nickname,
                                     uint8_t *ot_name);

/**
 * Remove Pokemon from a slot (shifts remaining Pokemon down)
 * @param slot Slot index to remove
 * @return STORAGE_OK on success
 */
storage_status_t pokemon_storage_remove(uint8_t slot);

/**
 * Update Pokemon in a specific slot (in-place update)
 * @param slot Slot index to update
 * @param party Pokemon party data (or NULL to keep existing)
 * @param nickname Nickname (encoded, or NULL to keep existing)
 * @param ot_name OT name (encoded, or NULL to keep existing)
 * @return STORAGE_OK on success
 */
storage_status_t pokemon_storage_update(uint8_t slot,
                                        const PokemonParty *party,
                                        const uint8_t *nickname,
                                        const uint8_t *ot_name);

/**
 * Clear all stored Pokemon
 * @return STORAGE_OK on success
 */
storage_status_t pokemon_storage_clear(void);

/**
 * Force save to flash (normally automatic)
 * @return STORAGE_OK on success
 */
storage_status_t pokemon_storage_save(void);

/**
 * Get storage statistics
 * @param write_count Output: total flash writes
 * @param checksum_valid Output: current data integrity
 */
void pokemon_storage_stats(uint32_t *write_count, bool *checksum_valid);

/**
 * Print storage contents to stdout (debug)
 */
void pokemon_storage_print(void);

/**
 * Get pointer to slot data (for direct access)
 * @param slot Slot index
 * @return Pointer to slot or NULL if invalid
 */
const PokemonSlot* pokemon_storage_get_slot(uint8_t slot);

/**
 * Get status string
 */
const char* pokemon_storage_status_name(storage_status_t status);

#endif // POKEMON_STORAGE_H
