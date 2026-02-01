/**
 * @file pokemon_data.c
 * @brief Pokemon Gen I data utilities implementation
 */

#include "pokemon_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

//--------------------------------------------------------------------
// Base stats for common Pokemon (for stat calculation)
//--------------------------------------------------------------------

// Bulbasaur base stats
#define BULBASAUR_BASE_HP  45
#define BULBASAUR_BASE_ATK 49
#define BULBASAUR_BASE_DEF 49
#define BULBASAUR_BASE_SPD 45
#define BULBASAUR_BASE_SPC 65

// Growth rates
#define GROWTH_MEDIUM_FAST 0
#define GROWTH_MEDIUM_SLOW 3
#define GROWTH_FAST        4
#define GROWTH_SLOW        5

//--------------------------------------------------------------------
// Character encoding lookup tables
//--------------------------------------------------------------------

uint8_t pokemon_char_encode(char c) {
    if (c >= 'A' && c <= 'Z') {
        return PKMN_CHAR_A + (c - 'A');
    }
    if (c >= 'a' && c <= 'z') {
        return PKMN_CHAR_a + (c - 'a');
    }
    if (c >= '0' && c <= '9') {
        return PKMN_CHAR_0 + (c - '0');
    }
    if (c == ' ') {
        return PKMN_CHAR_SPACE;
    }
    return PKMN_CHAR_TERM;  // Default to terminator
}

char pokemon_char_decode(uint8_t c) {
    if (c >= PKMN_CHAR_A && c <= PKMN_CHAR_Z) {
        return 'A' + (c - PKMN_CHAR_A);
    }
    if (c >= PKMN_CHAR_a && c <= PKMN_CHAR_z) {
        return 'a' + (c - PKMN_CHAR_a);
    }
    if (c >= PKMN_CHAR_0 && c <= PKMN_CHAR_9) {
        return '0' + (c - PKMN_CHAR_0);
    }
    if (c == PKMN_CHAR_SPACE) {
        return ' ';
    }
    if (c == PKMN_CHAR_TERM) {
        return '\0';
    }
    return '?';  // Unknown character
}

void pokemon_string_encode(uint8_t *dest, const char *src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len && src[i] != '\0'; i++) {
        dest[i] = pokemon_char_encode(src[i]);
    }
    // Fill remaining with terminator
    for (; i < PKMN_NAME_LENGTH; i++) {
        dest[i] = PKMN_CHAR_TERM;
    }
}

void pokemon_string_decode(char *dest, const uint8_t *src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len; i++) {
        if (src[i] == PKMN_CHAR_TERM) {
            break;
        }
        dest[i] = pokemon_char_decode(src[i]);
    }
    dest[i] = '\0';
}

//--------------------------------------------------------------------
// Stat calculations
//--------------------------------------------------------------------

uint32_t pokemon_calc_exp(uint8_t level, uint8_t growth_rate) {
    uint32_t exp = 0;
    uint32_t l = level;
    
    switch (growth_rate) {
        case GROWTH_FAST:
            exp = (4 * l * l * l) / 5;
            break;
        case GROWTH_MEDIUM_FAST:
            exp = l * l * l;
            break;
        case GROWTH_MEDIUM_SLOW:
            exp = ((6 * l * l * l) / 5) - (15 * l * l) + (100 * l) - 140;
            break;
        case GROWTH_SLOW:
            exp = (5 * l * l * l) / 4;
            break;
        default:
            exp = l * l * l;  // Default to medium-fast
    }
    return exp;
}

uint16_t pokemon_calc_stat(uint8_t base, uint8_t iv, uint16_t ev, uint8_t level, bool is_hp) {
    // Gen I formula: ((2 * (Base + IV) + sqrt(EV) / 4) * Level / 100) + extra
    // HP: extra = Level + 10
    // Other: extra = 5
    
    uint32_t ev_sqrt = (uint32_t)sqrt((double)pokemon_swap16(ev));
    uint32_t calc = (((2 * (base + iv)) + (ev_sqrt / 4)) * level) / 100;
    
    if (is_hp) {
        calc += level + 10;
    } else {
        calc += 5;
    }
    
    return (uint16_t)calc;
}

//--------------------------------------------------------------------
// Trade block initialization
//--------------------------------------------------------------------

void pokemon_trade_block_init(TradeBlock *block) {
    memset(block, 0, sizeof(TradeBlock));
    
    // Set party species list terminator
    memset(block->party_species, 0xFF, sizeof(block->party_species));
    block->party_count = 0;
    
    // Initialize all name fields with terminator
    memset(&block->trainer_name, PKMN_CHAR_TERM, sizeof(PokemonName));
    for (int i = 0; i < PKMN_PARTY_SIZE; i++) {
        memset(&block->ot_names[i], PKMN_CHAR_TERM, sizeof(PokemonName));
        memset(&block->nicknames[i], PKMN_CHAR_TERM, sizeof(PokemonName));
    }
}

void pokemon_create_bulbasaur(TradeBlock *block, const char *trainer_name, uint16_t trainer_id) {
    pokemon_trade_block_init(block);
    
    // Set trainer name
    pokemon_string_encode(block->trainer_name.str, trainer_name, 7);
    
    // One Pokemon in party
    block->party_count = 1;
    block->party_species[0] = SPECIES_BULBASAUR;
    block->party_species[1] = 0xFF;  // Terminator
    
    // Set up the Pokemon
    PokemonParty *bulba = &block->party[0];
    
    bulba->index = SPECIES_BULBASAUR;
    bulba->level = 5;
    bulba->level_again = 5;
    bulba->status = 0;  // Healthy
    
    // Types: Grass/Poison
    bulba->type[0] = TYPE_GRASS;
    bulba->type[1] = TYPE_POISON;
    
    bulba->catch_rate = 0x2D;  // 45
    
    // Moves: Tackle, Growl
    bulba->moves[0] = MOVE_TACKLE;
    bulba->moves[1] = MOVE_GROWL;
    bulba->moves[2] = MOVE_NONE;
    bulba->moves[3] = MOVE_NONE;
    
    // PP for moves (max PP for Tackle = 35, Growl = 40)
    bulba->pp[0] = 35;
    bulba->pp[1] = 40;
    bulba->pp[2] = 0;
    bulba->pp[3] = 0;
    
    // Trainer ID (big-endian)
    bulba->ot_id = pokemon_swap16(trainer_id);
    
    // Experience for level 5, medium-slow growth
    uint32_t exp = pokemon_calc_exp(5, GROWTH_MEDIUM_SLOW);
    bulba->exp[0] = (exp >> 16) & 0xFF;
    bulba->exp[1] = (exp >> 8) & 0xFF;
    bulba->exp[2] = exp & 0xFF;
    
    // Max IVs for a nice Pokemon (0xFFFF)
    bulba->iv = 0xFFFF;
    
    // Zero EVs (just caught)
    bulba->hp_ev = 0;
    bulba->atk_ev = 0;
    bulba->def_ev = 0;
    bulba->spd_ev = 0;
    bulba->spc_ev = 0;
    
    // Extract IVs for stat calculation (all 15s with max IVs)
    uint8_t iv_atk = 15;
    uint8_t iv_def = 15;
    uint8_t iv_spd = 15;
    uint8_t iv_spc = 15;
    uint8_t iv_hp = ((iv_atk & 1) << 3) | ((iv_def & 1) << 2) | 
                    ((iv_spd & 1) << 1) | (iv_spc & 1);  // HP IV derived from others
    
    // Calculate stats
    uint16_t hp = pokemon_calc_stat(BULBASAUR_BASE_HP, iv_hp, 0, 5, true);
    uint16_t atk = pokemon_calc_stat(BULBASAUR_BASE_ATK, iv_atk, 0, 5, false);
    uint16_t def = pokemon_calc_stat(BULBASAUR_BASE_DEF, iv_def, 0, 5, false);
    uint16_t spd = pokemon_calc_stat(BULBASAUR_BASE_SPD, iv_spd, 0, 5, false);
    uint16_t spc = pokemon_calc_stat(BULBASAUR_BASE_SPC, iv_spc, 0, 5, false);
    
    // Store stats (big-endian)
    bulba->hp = pokemon_swap16(hp);
    bulba->max_hp = pokemon_swap16(hp);
    bulba->atk = pokemon_swap16(atk);
    bulba->def = pokemon_swap16(def);
    bulba->spd = pokemon_swap16(spd);
    bulba->spc = pokemon_swap16(spc);
    
    // Set OT name (same as trainer)
    pokemon_string_encode(block->ot_names[0].str, trainer_name, 7);
    
    // Set nickname (default to uppercase species name)
    pokemon_string_encode(block->nicknames[0].str, "BULBASAUR", 10);
}

//--------------------------------------------------------------------
// Patch list handling
//--------------------------------------------------------------------

void pokemon_build_patch_list(PatchList *plist, TradeBlock *block) {
    plist->length = 0;
    
    // Party data is 6 * 44 = 264 bytes
    uint8_t *party_data = (uint8_t *)block->party;
    size_t party_size = sizeof(block->party);
    
    // Scan for 0xFE bytes and build patch list
    // Part 1: bytes 0x00 to 0xFB (indices 1-252)
    // Part 2: bytes 0xFC to end (indices 1-...)
    
    for (size_t i = 0; i < party_size; i++) {
        if (i == 0xFC) {
            // End of part 1
            plist->data[plist->length++] = 0xFF;
        }
        
        if (party_data[i] == 0xFE) {
            // Record position and replace with 0xFF
            uint8_t idx = (i % 0xFC) + 1;
            plist->data[plist->length++] = idx;
            party_data[i] = 0xFF;
        }
    }
    
    // End of part 2
    plist->data[plist->length++] = 0xFF;
    
    // Fill rest with zeros
    while (plist->length < PATCH_LIST_MAX_SIZE) {
        plist->data[plist->length++] = 0x00;
    }
}

void pokemon_apply_patch_list(uint8_t *party_data, const uint8_t *patch_data, size_t patch_len) {
    bool part2 = false;
    
    for (size_t i = 0; i < patch_len; i++) {
        uint8_t val = patch_data[i];
        
        if (val == 0xFF) {
            // Terminator for current part
            part2 = true;
            continue;
        }
        
        if (val == 0x00) {
            // Rest is padding
            continue;
        }
        
        // Calculate actual offset
        size_t offset;
        if (!part2) {
            offset = val - 1;  // Part 1: val is index+1
        } else {
            offset = 0xFC + val - 1;  // Part 2: starts at 0xFC
        }
        
        // Restore the 0xFE byte
        party_data[offset] = 0xFE;
    }
}

//--------------------------------------------------------------------
// Debug output
//--------------------------------------------------------------------

const char* pokemon_get_species_name(uint8_t index) {
    // Common species lookup
    switch (index) {
        case SPECIES_BULBASAUR:  return "Bulbasaur";
        case SPECIES_IVYSAUR:    return "Ivysaur";
        case SPECIES_VENUSAUR:   return "Venusaur";
        case SPECIES_CHARMANDER: return "Charmander";
        case SPECIES_CHARMELEON: return "Charmeleon";
        case SPECIES_CHARIZARD:  return "Charizard";
        case SPECIES_SQUIRTLE:   return "Squirtle";
        case SPECIES_WARTORTLE:  return "Wartortle";
        case SPECIES_BLASTOISE:  return "Blastoise";
        case SPECIES_PIKACHU:    return "Pikachu";
        case SPECIES_RAICHU:     return "Raichu";
        case SPECIES_MEW:        return "Mew";
        case SPECIES_MEWTWO:     return "Mewtwo";
        default:                 return "Unknown";
    }
}

void pokemon_print_info(const PokemonParty *pokemon, const char *nickname) {
    printf("\n=== Pokemon Info ===\n");
    printf("Species: %s (0x%02X)\n", pokemon_get_species_name(pokemon->index), pokemon->index);
    if (nickname) {
        printf("Nickname: %s\n", nickname);
    }
    printf("Level: %d\n", pokemon->level);
    printf("HP: %d / %d\n", pokemon_swap16(pokemon->hp), pokemon_swap16(pokemon->max_hp));
    printf("Stats: ATK=%d DEF=%d SPD=%d SPC=%d\n",
           pokemon_swap16(pokemon->atk),
           pokemon_swap16(pokemon->def),
           pokemon_swap16(pokemon->spd),
           pokemon_swap16(pokemon->spc));
    printf("Moves: %02X %02X %02X %02X\n",
           pokemon->moves[0], pokemon->moves[1], pokemon->moves[2], pokemon->moves[3]);
    printf("OT ID: %d\n", pokemon_swap16(pokemon->ot_id));
    printf("IVs: %04X\n", pokemon->iv);
    printf("====================\n\n");
}
