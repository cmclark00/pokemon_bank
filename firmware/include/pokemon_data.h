/**
 * @file pokemon_data.h
 * @brief Pokemon Gen I data structures and utilities
 * 
 * Defines the data structures for Pokemon party members and trade blocks
 * as used in Pokemon Red/Blue/Yellow. All multi-byte values are big-endian
 * (Game Boy byte order).
 */

#ifndef POKEMON_DATA_H
#define POKEMON_DATA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//--------------------------------------------------------------------
// Pokemon character encoding (Gen I English)
//--------------------------------------------------------------------

#define PKMN_CHAR_TERM    0x50  // String terminator
#define PKMN_CHAR_SPACE   0x7F
#define PKMN_CHAR_A       0x80
#define PKMN_CHAR_B       0x81
#define PKMN_CHAR_C       0x82
#define PKMN_CHAR_D       0x83
#define PKMN_CHAR_E       0x84
#define PKMN_CHAR_F       0x85
#define PKMN_CHAR_G       0x86
#define PKMN_CHAR_H       0x87
#define PKMN_CHAR_I       0x88
#define PKMN_CHAR_J       0x89
#define PKMN_CHAR_K       0x8A
#define PKMN_CHAR_L       0x8B
#define PKMN_CHAR_M       0x8C
#define PKMN_CHAR_N       0x8D
#define PKMN_CHAR_O       0x8E
#define PKMN_CHAR_P       0x8F
#define PKMN_CHAR_Q       0x90
#define PKMN_CHAR_R       0x91
#define PKMN_CHAR_S       0x92
#define PKMN_CHAR_T       0x93
#define PKMN_CHAR_U       0x94
#define PKMN_CHAR_V       0x95
#define PKMN_CHAR_W       0x96
#define PKMN_CHAR_X       0x97
#define PKMN_CHAR_Y       0x98
#define PKMN_CHAR_Z       0x99
#define PKMN_CHAR_a       0xA0
#define PKMN_CHAR_b       0xA1
#define PKMN_CHAR_c       0xA2
#define PKMN_CHAR_d       0xA3
#define PKMN_CHAR_e       0xA4
#define PKMN_CHAR_f       0xA5
#define PKMN_CHAR_g       0xA6
#define PKMN_CHAR_h       0xA7
#define PKMN_CHAR_i       0xA8
#define PKMN_CHAR_j       0xA9
#define PKMN_CHAR_k       0xAA
#define PKMN_CHAR_l       0xAB
#define PKMN_CHAR_m       0xAC
#define PKMN_CHAR_n       0xAD
#define PKMN_CHAR_o       0xAE
#define PKMN_CHAR_p       0xAF
#define PKMN_CHAR_q       0xB0
#define PKMN_CHAR_r       0xB1
#define PKMN_CHAR_s       0xB2
#define PKMN_CHAR_t       0xB3
#define PKMN_CHAR_u       0xB4
#define PKMN_CHAR_v       0xB5
#define PKMN_CHAR_w       0xB6
#define PKMN_CHAR_x       0xB7
#define PKMN_CHAR_y       0xB8
#define PKMN_CHAR_z       0xB9
#define PKMN_CHAR_0       0xF6
#define PKMN_CHAR_1       0xF7
#define PKMN_CHAR_2       0xF8
#define PKMN_CHAR_3       0xF9
#define PKMN_CHAR_4       0xFA
#define PKMN_CHAR_5       0xFB
#define PKMN_CHAR_6       0xFC
#define PKMN_CHAR_7       0xFD
#define PKMN_CHAR_8       0xFE
#define PKMN_CHAR_9       0xFF

//--------------------------------------------------------------------
// Size constants
//--------------------------------------------------------------------

#define PKMN_NAME_LENGTH      11   // Max 10 chars + terminator
#define PKMN_PARTY_SIZE       6    // 6 Pokemon in party
#define PKMN_PARTY_DATA_SIZE  44   // Size of party Pokemon struct
#define PKMN_TRADE_BLOCK_SIZE 415  // Total trade block size for Gen I

//--------------------------------------------------------------------
// Pokemon species indices (Gen I internal index, NOT Pokedex number)
//--------------------------------------------------------------------

// Common starters and trade candidates
#define SPECIES_BULBASAUR     0x99
#define SPECIES_IVYSAUR       0x09
#define SPECIES_VENUSAUR      0x9A
#define SPECIES_CHARMANDER    0xB0
#define SPECIES_CHARMELEON    0xB2
#define SPECIES_CHARIZARD     0xB4
#define SPECIES_SQUIRTLE      0xB1
#define SPECIES_WARTORTLE     0xB3
#define SPECIES_BLASTOISE     0x1C
#define SPECIES_PIKACHU       0x54
#define SPECIES_RAICHU        0x55
#define SPECIES_MEW           0x15
#define SPECIES_MEWTWO        0x83

//--------------------------------------------------------------------
// Type indices
//--------------------------------------------------------------------

#define TYPE_NORMAL   0x00
#define TYPE_FIGHTING 0x01
#define TYPE_FLYING   0x02
#define TYPE_POISON   0x03
#define TYPE_GROUND   0x04
#define TYPE_ROCK     0x05
#define TYPE_BUG      0x07
#define TYPE_GHOST    0x08
#define TYPE_FIRE     0x14
#define TYPE_WATER    0x15
#define TYPE_GRASS    0x16
#define TYPE_ELECTRIC 0x17
#define TYPE_PSYCHIC  0x18
#define TYPE_ICE      0x19
#define TYPE_DRAGON   0x1A

//--------------------------------------------------------------------
// Move indices
//--------------------------------------------------------------------

#define MOVE_NONE      0x00
#define MOVE_TACKLE    0x21
#define MOVE_GROWL     0x2D
#define MOVE_SCRATCH   0x0A
#define MOVE_TAIL_WHIP 0x27
#define MOVE_LEECH_SEED 0x49
#define MOVE_VINE_WHIP 0x16

//--------------------------------------------------------------------
// Data Structures
//--------------------------------------------------------------------

/**
 * Pokemon party data structure (44 bytes)
 * This is exactly how Pokemon data is stored in memory/transmitted.
 * All multi-byte values are big-endian (Game Boy native byte order).
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  index;           // Pokemon species index (NOT Pokedex number!)
    uint16_t hp;              // Current HP (big-endian)
    uint8_t  level;           // Current level (also in level_again)
    uint8_t  status;          // Status condition (0 = healthy)
    uint8_t  type[2];         // Type 1, Type 2 (single type = both same)
    uint8_t  catch_rate;      // Catch rate / held item slot
    uint8_t  moves[4];        // Move indices
    uint16_t ot_id;           // Original trainer ID (big-endian)
    uint8_t  exp[3];          // Experience points (3 bytes, big-endian)
    uint16_t hp_ev;           // HP EV (big-endian)
    uint16_t atk_ev;          // Attack EV (big-endian)
    uint16_t def_ev;          // Defense EV (big-endian)
    uint16_t spd_ev;          // Speed EV (big-endian)
    uint16_t spc_ev;          // Special EV (big-endian)
    uint16_t iv;              // DVs/IVs packed (atk:4, def:4, spd:4, spc:4)
    uint8_t  pp[4];           // PP for each move
    uint8_t  level_again;     // Level (duplicate)
    uint16_t max_hp;          // Max HP (big-endian)
    uint16_t atk;             // Attack stat (big-endian)
    uint16_t def;             // Defense stat (big-endian)
    uint16_t spd;             // Speed stat (big-endian)
    uint16_t spc;             // Special stat (big-endian)
} PokemonParty;

/**
 * Name structure - 11 bytes for all name fields
 */
typedef struct __attribute__((__packed__)) {
    uint8_t str[PKMN_NAME_LENGTH];
} PokemonName;

/**
 * Complete trade block for Gen I (415 bytes)
 * This is the full data structure exchanged during trading.
 */
typedef struct __attribute__((__packed__)) {
    PokemonName trainer_name;               // Trainer name (11 bytes)
    uint8_t     party_count;                // Number of Pokemon (1-6)
    uint8_t     party_species[7];           // Species list (6 + terminator FF)
    PokemonParty party[PKMN_PARTY_SIZE];    // Party Pokemon data (6 * 44 = 264)
    PokemonName ot_names[PKMN_PARTY_SIZE];  // OT names (6 * 11 = 66)
    PokemonName nicknames[PKMN_PARTY_SIZE]; // Nicknames (6 * 11 = 66)
} TradeBlock;

// Verify sizes at compile time
_Static_assert(sizeof(PokemonParty) == 44, "PokemonParty must be 44 bytes");
_Static_assert(sizeof(TradeBlock) == 415, "TradeBlock must be 415 bytes");

//--------------------------------------------------------------------
// Patch List for 0xFE byte handling
//--------------------------------------------------------------------

/**
 * Patch list for handling 0xFE (NO_DATA) bytes during transmission.
 * The protocol can't transmit 0xFE, so we replace it with 0xFF
 * and record the location in the patch list.
 */
#define PATCH_LIST_MAX_SIZE 200

typedef struct {
    uint8_t data[PATCH_LIST_MAX_SIZE];
    size_t  length;
} PatchList;

//--------------------------------------------------------------------
// Functions
//--------------------------------------------------------------------

/**
 * Convert ASCII character to Pokemon character encoding
 */
uint8_t pokemon_char_encode(char c);

/**
 * Convert Pokemon character encoding to ASCII
 */
char pokemon_char_decode(uint8_t c);

/**
 * Encode ASCII string to Pokemon format
 * @param dest Destination buffer (should be PKMN_NAME_LENGTH)
 * @param src Source ASCII string
 * @param max_len Maximum characters to encode (excluding terminator)
 */
void pokemon_string_encode(uint8_t *dest, const char *src, size_t max_len);

/**
 * Decode Pokemon string to ASCII
 * @param dest Destination buffer
 * @param src Source Pokemon-encoded string
 * @param max_len Maximum characters to decode
 */
void pokemon_string_decode(char *dest, const uint8_t *src, size_t max_len);

/**
 * Swap bytes for 16-bit value (host <-> Game Boy endianness)
 */
static inline uint16_t pokemon_swap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

/**
 * Calculate experience points for a given level and growth rate
 */
uint32_t pokemon_calc_exp(uint8_t level, uint8_t growth_rate);

/**
 * Calculate a stat from base, IV, EV, and level
 */
uint16_t pokemon_calc_stat(uint8_t base, uint8_t iv, uint16_t ev, uint8_t level, bool is_hp);

/**
 * Initialize a trade block with default values
 */
void pokemon_trade_block_init(TradeBlock *block);

/**
 * Create a dummy tradeable Pokemon (Bulbasaur level 5)
 * Sets up a valid Pokemon in slot 0 of the trade block
 */
void pokemon_create_bulbasaur(TradeBlock *block, const char *trainer_name, uint16_t trainer_id);

/**
 * Build patch list from trade block party data
 * Replaces 0xFE bytes with 0xFF and records positions
 */
void pokemon_build_patch_list(PatchList *plist, TradeBlock *block);

/**
 * Apply received patch list to party data
 * Restores 0xFE bytes based on patch indices
 */
void pokemon_apply_patch_list(uint8_t *party_data, const uint8_t *patch_data, size_t patch_len);

/**
 * Print Pokemon info to debug output
 */
void pokemon_print_info(const PokemonParty *pokemon, const char *nickname);

/**
 * Get species name from index
 */
const char* pokemon_get_species_name(uint8_t index);

#endif // POKEMON_DATA_H
