/**
 * @file webui_protocol.c
 * @brief WebUI JSON Protocol Handler Implementation
 * 
 * Simple JSON parser and command handler for WebUI communication.
 * This is a minimal implementation suitable for embedded systems.
 */

#include "webui_protocol.h"
#include "pokemon_storage.h"
#include "pokemon_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//--------------------------------------------------------------------
// Simple JSON parsing helpers
//--------------------------------------------------------------------

/**
 * Skip whitespace in string
 */
static const char* skip_ws(const char *s) {
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) s++;
    return s;
}

/**
 * Find string value in JSON (simple implementation)
 * Returns pointer to value start (after opening quote) or NULL
 */
static const char* json_find_string(const char *json, const char *key, char *out, size_t out_size) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *p = strstr(json, search);
    if (!p) return NULL;
    
    p += strlen(search);
    p = skip_ws(p);
    if (*p != ':') return NULL;
    p++;
    p = skip_ws(p);
    
    if (*p != '"') return NULL;
    p++;
    
    // Copy string value
    size_t i = 0;
    while (*p && *p != '"' && i < out_size - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    
    return out;
}

/**
 * Find integer value in JSON
 */
static bool json_find_int(const char *json, const char *key, int *out) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *p = strstr(json, search);
    if (!p) return false;
    
    p += strlen(search);
    p = skip_ws(p);
    if (*p != ':') return false;
    p++;
    p = skip_ws(p);
    
    // Parse number
    char *end;
    long val = strtol(p, &end, 10);
    if (end == p) return false;
    
    *out = (int)val;
    return true;
}

/**
 * Find array of integers in JSON (for moves)
 */
static bool json_find_int_array(const char *json, const char *key, int *out, size_t max_count, size_t *out_count) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    
    const char *p = strstr(json, search);
    if (!p) return false;
    
    p += strlen(search);
    p = skip_ws(p);
    if (*p != ':') return false;
    p++;
    p = skip_ws(p);
    
    if (*p != '[') return false;
    p++;
    
    *out_count = 0;
    while (*p && *p != ']' && *out_count < max_count) {
        p = skip_ws(p);
        if (*p == ',' ) { p++; continue; }
        if (*p == ']') break;
        
        char *end;
        long val = strtol(p, &end, 10);
        if (end == p) break;
        
        out[(*out_count)++] = (int)val;
        p = end;
    }
    
    return *out_count > 0;
}

/**
 * Find nested object string (simplified - finds key inside "data" object)
 */
static const char* json_find_data_string(const char *json, const char *key, char *out, size_t out_size) {
    // Find the "data" object first
    const char *data_start = strstr(json, "\"data\"");
    if (!data_start) return NULL;
    
    data_start = strchr(data_start, '{');
    if (!data_start) return NULL;
    
    // Now search within data object
    return json_find_string(data_start, key, out, out_size);
}

/**
 * Find nested object int array
 */
static bool json_find_data_int_array(const char *json, const char *key, int *out, size_t max, size_t *cnt) {
    const char *data_start = strstr(json, "\"data\"");
    if (!data_start) return false;
    
    data_start = strchr(data_start, '{');
    if (!data_start) return false;
    
    return json_find_int_array(data_start, key, out, max, cnt);
}

//--------------------------------------------------------------------
// Command Handlers
//--------------------------------------------------------------------

/**
 * Handle "list" command - return all stored Pokemon
 */
static void handle_list(char *response, size_t response_size) {
    int count = pokemon_storage_count();
    int offset = snprintf(response, response_size, "{\"pokemon\":[");
    
    for (int i = 0; i < count && offset < (int)response_size - 100; i++) {
        const PokemonSlot *slot = pokemon_storage_get_slot(i);
        if (!slot) continue;
        
        char nickname[12];
        pokemon_string_decode(nickname, slot->nickname, 10);
        
        const char *species = pokemon_get_species_name(slot->party.index);
        
        if (i > 0) {
            offset += snprintf(response + offset, response_size - offset, ",");
        }
        
        offset += snprintf(response + offset, response_size - offset,
            "{\"slot\":%d,\"index\":%d,\"species\":\"%s\",\"nickname\":\"%s\",\"level\":%d}",
            i, slot->party.index, species, nickname, slot->party.level);
    }
    
    snprintf(response + offset, response_size - offset, "]}");
}

/**
 * Handle "get" command - return detailed Pokemon data
 */
static void handle_get(int slot, char *response, size_t response_size) {
    const PokemonSlot *pslot = pokemon_storage_get_slot(slot);
    
    if (!pslot) {
        snprintf(response, response_size, "{\"error\":\"Invalid slot\"}");
        return;
    }
    
    char nickname[12], ot_name[12];
    pokemon_string_decode(nickname, pslot->nickname, 10);
    pokemon_string_decode(ot_name, pslot->ot_name, 10);
    
    const PokemonParty *p = &pslot->party;
    const char *species = pokemon_get_species_name(p->index);
    
    snprintf(response, response_size,
        "{"
        "\"slot\":%d,"
        "\"index\":%d,"
        "\"species\":\"%s\","
        "\"nickname\":\"%s\","
        "\"level\":%d,"
        "\"hp\":%d,"
        "\"max_hp\":%d,"
        "\"atk\":%d,"
        "\"def\":%d,"
        "\"spd\":%d,"
        "\"spc\":%d,"
        "\"moves\":[%d,%d,%d,%d],"
        "\"pp\":[%d,%d,%d,%d],"
        "\"ot_name\":\"%s\","
        "\"ot_id\":%d,"
        "\"iv\":%d,"
        "\"status\":%d"
        "}",
        slot,
        p->index,
        species,
        nickname,
        p->level,
        pokemon_swap16(p->hp),
        pokemon_swap16(p->max_hp),
        pokemon_swap16(p->atk),
        pokemon_swap16(p->def),
        pokemon_swap16(p->spd),
        pokemon_swap16(p->spc),
        p->moves[0], p->moves[1], p->moves[2], p->moves[3],
        p->pp[0], p->pp[1], p->pp[2], p->pp[3],
        ot_name,
        pokemon_swap16(p->ot_id),
        p->iv,
        p->status
    );
}

/**
 * Handle "set" command - update Pokemon data
 */
static void handle_set(int slot, const char *json, char *response, size_t response_size) {
    // Get current Pokemon data
    PokemonParty party;
    uint8_t nickname[PKMN_NAME_LENGTH];
    uint8_t ot_name[PKMN_NAME_LENGTH];
    
    storage_status_t status = pokemon_storage_get(slot, &party, nickname, ot_name);
    if (status != STORAGE_OK) {
        snprintf(response, response_size, "{\"ok\":false,\"error\":\"%s\"}", 
                 pokemon_storage_status_name(status));
        return;
    }
    
    bool nickname_updated = false;
    bool party_updated = false;
    
    // Update nickname if provided
    char new_nickname[12];
    if (json_find_data_string(json, "nickname", new_nickname, sizeof(new_nickname))) {
        pokemon_string_encode(nickname, new_nickname, 10);
        nickname_updated = true;
    }
    
    // Update moves if provided
    int moves[4];
    size_t move_count;
    if (json_find_data_int_array(json, "moves", moves, 4, &move_count)) {
        for (size_t i = 0; i < 4; i++) {
            if (i < move_count) {
                party.moves[i] = (uint8_t)moves[i];
                // Reset PP to a reasonable default (we'd need move data for actual max PP)
                if (moves[i] == 0) {
                    party.pp[i] = 0;
                } else if (party.pp[i] == 0) {
                    party.pp[i] = 35;  // Default PP
                }
            }
        }
        party_updated = true;
    }
    
    // Update in place
    status = pokemon_storage_update(slot, 
                                    party_updated ? &party : NULL,
                                    nickname_updated ? nickname : NULL,
                                    NULL);
    if (status != STORAGE_OK) {
        snprintf(response, response_size, "{\"ok\":false,\"error\":\"%s\"}", 
                 pokemon_storage_status_name(status));
        return;
    }
    
    snprintf(response, response_size, "{\"ok\":true}");
}

/**
 * Handle "delete" command - remove Pokemon from slot
 */
static void handle_delete(int slot, char *response, size_t response_size) {
    storage_status_t status = pokemon_storage_remove(slot);
    
    if (status == STORAGE_OK) {
        snprintf(response, response_size, "{\"ok\":true}");
    } else {
        snprintf(response, response_size, "{\"ok\":false,\"error\":\"%s\"}", 
                 pokemon_storage_status_name(status));
    }
}

/**
 * Handle "status" command - return device status
 */
static void handle_status(char *response, size_t response_size) {
    uint32_t write_count;
    bool checksum_valid;
    pokemon_storage_stats(&write_count, &checksum_valid);
    
    snprintf(response, response_size,
        "{"
        "\"count\":%d,"
        "\"max\":%d,"
        "\"writes\":%lu,"
        "\"checksum_ok\":%s,"
        "\"version\":\"1.0\""
        "}",
        pokemon_storage_count(),
        STORAGE_BOX_SIZE,
        write_count,
        checksum_valid ? "true" : "false"
    );
}

//--------------------------------------------------------------------
// Main Command Processor
//--------------------------------------------------------------------

bool webui_process_command(const char *json_str, char *response_buf, size_t response_size) {
    if (!json_str || !response_buf || response_size < 64) {
        return false;
    }
    
    // Parse command type
    char cmd[32];
    if (!json_find_string(json_str, "cmd", cmd, sizeof(cmd))) {
        snprintf(response_buf, response_size, "{\"error\":\"Missing cmd field\"}");
        return true;
    }
    
    printf("[WebUI] Command: %s\n", cmd);
    
    // Dispatch command
    if (strcmp(cmd, "list") == 0) {
        handle_list(response_buf, response_size);
    }
    else if (strcmp(cmd, "get") == 0) {
        int slot;
        if (!json_find_int(json_str, "slot", &slot)) {
            snprintf(response_buf, response_size, "{\"error\":\"Missing slot\"}");
        } else {
            handle_get(slot, response_buf, response_size);
        }
    }
    else if (strcmp(cmd, "set") == 0) {
        int slot;
        if (!json_find_int(json_str, "slot", &slot)) {
            snprintf(response_buf, response_size, "{\"error\":\"Missing slot\"}");
        } else {
            handle_set(slot, json_str, response_buf, response_size);
        }
    }
    else if (strcmp(cmd, "delete") == 0) {
        int slot;
        if (!json_find_int(json_str, "slot", &slot)) {
            snprintf(response_buf, response_size, "{\"error\":\"Missing slot\"}");
        } else {
            handle_delete(slot, response_buf, response_size);
        }
    }
    else if (strcmp(cmd, "status") == 0) {
        handle_status(response_buf, response_size);
    }
    else {
        snprintf(response_buf, response_size, "{\"error\":\"Unknown command: %s\"}", cmd);
    }
    
    return true;
}
