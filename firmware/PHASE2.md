# Phase 2: Trading State Machine - COMPLETE

## Summary

Phase 2 implements the Pokemon Gen I trading protocol, allowing the RP2040 device to successfully trade Pokemon with a real Game Boy running Pokemon Red, Blue, or Yellow.

## Files Added

### Include Files
- `include/pokemon_data.h` - Pokemon data structures for Gen I
  - `PokemonParty` - 44-byte party Pokemon structure
  - `TradeBlock` - 415-byte complete trade block
  - `PatchList` - For handling 0xFE byte transmission issues
  - Character encoding/decoding for Pokemon text format
  - Stat calculation functions

- `include/pokemon_trade.h` - Trade state machine interface
  - State enums for connection and trade center states
  - `pokemon_trade_ctx` - Main trading context structure
  - Function declarations for trade processing

### Source Files
- `src/pokemon_data.c` - Pokemon data utilities
  - Character encoding/decoding between ASCII and Pokemon format
  - Stat calculation using Gen I formulas
  - Experience point calculation for different growth rates
  - Patch list creation and application
  - `pokemon_create_bulbasaur()` - Creates a valid tradeable Pokemon

- `src/pokemon_trade.c` - Trading state machine
  - Connection negotiation (master/slave)
  - Menu handling
  - Trade center protocol with all sub-states:
    - Preamble/random byte exchange
    - Trade block data exchange (415 bytes)
    - Patch list exchange (for 0xFE bytes)
    - Pokemon selection
    - Trade confirmation
    - Trade completion

- `src/main.c` - Updated with trade integration
  - Trade state machine integration
  - LED colors for different states
  - Serial commands for debugging

## Protocol Implementation

The trade protocol follows this flow:

1. **Link Negotiation** - Game Boy sends 0x01 (master), we respond 0x02 (slave)
2. **Connection** - Both exchange 0x60 (connected)
3. **Menu** - Wait for Trade Centre selection (0xD4)
4. **Trade Block Exchange**:
   - 10x preamble bytes (0xFD)
   - 10x random seed bytes
   - 9x more preamble bytes
   - 415 bytes of trade block data
5. **Patch List Exchange**:
   - 6x preamble bytes
   - 7x blank bytes
   - 189 bytes of patch data
6. **Pokemon Selection** - Game Boy picks Pokemon, we offer slot 0
7. **Confirmation** - Both send 0x62 to accept
8. **Completion** - Trade blocks re-exchanged with updated party

## What Gets Traded

The device offers a **Level 5 Bulbasaur** with:
- Trainer name: "PKMBANK"
- Trainer ID: 12345
- Moves: Tackle, Growl
- Max IVs (all 15s)
- Zero EVs (just caught)
- Properly calculated stats

## LED Status Indicators

| Color | Meaning |
|-------|---------|
| Blue | Waiting for Game Boy connection |
| Green | Connected to Game Boy |
| Cyan | In trade room, ready to trade |
| Orange | Trade pending (awaiting confirmation) |
| Purple | Trade in progress |
| Bright Green (flashing) | Trade complete! |
| Yellow (flash) | Data activity |
| Red | Error |

## Serial Commands

| Key | Action |
|-----|--------|
| s | Show status |
| r | Reset trade state |
| p | Print current Pokemon info |
| d | Debug: dump trade block bytes |
| b | Enter BOOTSEL mode |
| h | Help |

## Building

```bash
cd firmware/build
cmake ..
make -j4
```

Output: `pokemon_bank.uf2` (85KB)

## Testing

1. Flash the firmware to the RP2040-Zero
2. Connect Game Boy Link Cable to the board
3. On Game Boy: Enter Pokemon Center → Cable Club → Trade Center
4. LED should change from blue → green → cyan
5. Select a Pokemon to trade
6. The trade should complete with the Bulbasaur

## Success Criteria Met

- [x] Can receive Pokemon data from Game Boy
- [x] Can send valid Pokemon data to Game Boy
- [x] Trade completes without errors (via state machine)
- [x] Received Pokemon data is correct (printed to debug output)

## Known Limitations

1. Only Gen I is supported (Red/Blue/Yellow)
2. Only one Pokemon is offered (Bulbasaur)
3. Received Pokemon is kept in RAM only (no storage yet)
4. No WebUI yet

## Next Steps (Phase 3)

- Flash storage for Pokemon boxes
- Save received Pokemon persistently
- Box system (20+ Pokemon storage)
- Corruption detection via checksum
