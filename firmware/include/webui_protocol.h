/**
 * @file webui_protocol.h
 * @brief WebUI JSON Protocol Handler for Pokemon Bank
 * 
 * Handles JSON commands from the WebUI via USB CDC.
 * 
 * Commands:
 *   {"cmd": "list"}                    - List all stored Pokemon
 *   {"cmd": "get", "slot": N}          - Get detailed Pokemon data
 *   {"cmd": "set", "slot": N, "data": {...}} - Update Pokemon data
 *   {"cmd": "delete", "slot": N}       - Delete Pokemon from slot
 *   {"cmd": "status"}                  - Get device status
 */

#ifndef WEBUI_PROTOCOL_H
#define WEBUI_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Maximum JSON buffer size
#define JSON_BUFFER_SIZE 2048

/**
 * Process incoming JSON command string
 * @param json_str Input JSON string (null-terminated)
 * @param response_buf Output buffer for JSON response
 * @param response_size Size of output buffer
 * @return true if command was processed successfully
 */
bool webui_process_command(const char *json_str, char *response_buf, size_t response_size);

/**
 * Check if a character might be start of JSON
 */
static inline bool webui_is_json_start(char c) {
    return c == '{';
}

#endif // WEBUI_PROTOCOL_H
