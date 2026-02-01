# Pokemon Bank for RP2040-Zero

Build a Pokemon storage/trading system using the RP2040-Zero-based Game Boy Link Board.

## Project Overview

**Goal:** Create a device that:
1. Acts as a "second Game Boy" for trading Pokemon via Link Cable
2. Stores traded Pokemon persistently in flash
3. Provides a WebUI (via USB + WebSerial) to view/edit stored Pokemon

**Hardware:** 
- RP2040-Zero with bi-directional logic level converter
- Game Boy Link Cable connection (DMG/GBC/GBA compatible)
- USB-C for power + data

## Architecture

```
RP2040 Firmware (C/C++ with Pico SDK):
├── gb_link.c          - Game Boy Link Cable protocol (SPI-like, directly on PIO)
├── pokemon_trade.c    - Pokemon Gen I trade state machine
├── pokemon_data.c     - Pokemon data structures + validation
├── storage.c          - Flash storage for Pokemon boxes
├── usb_serial.c       - TinyUSB CDC for WebSerial communication
└── main.c             - State machine coordinator

WebUI (static HTML/JS served from flash or hosted):
├── index.html         - Main interface
├── pokemon.js         - Pokemon data parsing/editing
├── serial.js          - WebSerial API wrapper
└── sprites/           - Gen I Pokemon sprites
```

## Phase 1: GB Link Protocol + Basic Trade Handshake
**Goal:** RP2040 can communicate with Game Boy and complete trade room handshake

Tasks:
1. Set up Pico SDK project structure
2. Implement GB Link Cable protocol using PIO (clock + data lines)
3. Handle the Gen I Pokemon trade room synchronization
4. Verify handshake works with real Pokemon Blue cartridge

Success criteria:
- [ ] Can detect Game Boy connection
- [ ] Trade room sync bytes exchanged correctly
- [ ] Game Boy shows "waiting" state properly
- [ ] No lockups or desyncs

References:
- https://github.com/stacksmashing/gb-link-firmware (base protocol)
- https://github.com/kbembedded/Flipper-Zero-Game-Boy-Pokemon-Trading (trade state machine)
- See HARDWARE.md for complete pinout (GP0=CLK, GP1=SI, GP2=SO, GP3=SD)

Output <promise>PHASE1_DONE</promise> when complete.

---

## Phase 2: Pokemon Trading State Machine
**Goal:** Successfully trade a Pokemon from Game Boy to RP2040 and back

Tasks:
1. Implement Pokemon Gen I data structure (44 bytes party, 33 bytes box)
2. Port trade state machine from Flipper project
3. Handle party data exchange protocol
4. Create a "dummy" tradeable Pokemon on RP2040 side
5. Complete full trade cycle (GB gives Pokemon, RP2040 gives dummy)

Success criteria:
- [ ] Can receive Pokemon data from Game Boy
- [ ] Can send valid Pokemon data to Game Boy
- [ ] Trade completes without errors
- [ ] Received Pokemon data is correct (verify species, level, moves)

References:
- Gen I Pokemon data structure: https://bulbapedia.bulbagarden.net/wiki/Pokémon_data_structure_(Generation_I)
- Trade protocol: reverse engineered in Flipper project

Output <promise>PHASE2_DONE</promise> when complete.

---

## Phase 3: Flash Storage (Pokemon Boxes)
**Goal:** Persistently store multiple Pokemon in RP2040 flash

Tasks:
1. Design flash storage layout (wear leveling friendly)
2. Implement Pokemon box system (start with 1 box = 20 Pokemon)
3. Save Pokemon after each trade
4. Load Pokemon on boot
5. Handle "box full" scenario gracefully

Storage layout suggestion:
```
Flash sector (4KB):
├── Header (magic, version, pokemon count)
├── Pokemon[0..19] (44 bytes each = 880 bytes)
└── Checksum
```

Success criteria:
- [ ] Pokemon survives power cycle
- [ ] Can store at least 20 Pokemon
- [ ] Can retrieve specific Pokemon by slot
- [ ] Corruption detection (checksum)

Output <promise>PHASE3_DONE</promise> when complete.

---

## Phase 4: WebUI via WebSerial
**Goal:** Browser-based interface to view and edit stored Pokemon

Tasks:
1. Implement USB CDC serial protocol (TinyUSB)
2. Design simple command protocol (JSON or binary)
3. Create WebSerial connection handler in JS
4. Build Pokemon list view (show all stored Pokemon)
5. Build Pokemon detail view (stats, moves, nickname)
6. Implement Pokemon editor (modify and write back)

Command protocol example:
```
-> {"cmd": "list"}
<- {"pokemon": [{"slot": 0, "species": "BULBASAUR", "level": 5}, ...]}

-> {"cmd": "get", "slot": 0}
<- {"slot": 0, "species": "BULBASAUR", "level": 5, "moves": [...], ...}

-> {"cmd": "set", "slot": 0, "data": {...}}
<- {"ok": true}
```

Success criteria:
- [ ] WebSerial connects to RP2040
- [ ] Can list all stored Pokemon
- [ ] Can view Pokemon details with sprites
- [ ] Can edit Pokemon (nickname, moves at minimum)
- [ ] Changes persist to flash
- [ ] Works on Chrome/Edge (WebSerial support)

Output <promise>PHASE4_DONE</promise> when complete.

---

## Phase 5: Polish + Full Integration
**Goal:** Seamless end-to-end experience

Tasks:
1. Add LED status indicators (connected, trading, idle)
2. Handle edge cases (cable disconnect mid-trade, etc.)
3. Add "select Pokemon to trade" in WebUI
4. Support multiple boxes
5. Export/import Pokemon data (backup)
6. Documentation + README

Success criteria:
- [ ] Can trade from Game Boy → storage → back to Game Boy
- [ ] WebUI is intuitive and responsive
- [ ] No crashes or data loss in normal use
- [ ] README explains setup and usage

Output <promise>COMPLETE</promise> when all done.

---

## Key Resources

### Existing Code to Study/Port
- **Flipper Pokemon Trading:** https://github.com/kbembedded/Flipper-Zero-Game-Boy-Pokemon-Trading
  - `pokemon_data.h/c` - Data structures
  - `trade.c` - Trade state machine
  - Best reference for the protocol
  
- **GB Link Firmware:** https://github.com/stacksmashing/gb-link-firmware
  - Base RP2040 link cable protocol
  - Already works with your board
  
- **Reconfigurable Firmware:** https://github.com/Lorenzooone/gb-link-firmware-reconfigurable
  - Enhanced version with more flexibility

### Documentation
- **Gen I Pokemon Data:** https://bulbapedia.bulbagarden.net/wiki/Pokémon_data_structure_(Generation_I)
- **GB Link Protocol:** https://gbdev.io/pandocs/Serial_Data_Transfer_(Link_Cable).html
- **Pico SDK:** https://github.com/raspberrypi/pico-sdk
- **TinyUSB:** https://github.com/hathach/tinyusb
- **WebSerial API:** https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API

### Hardware Reference
- Your board: https://github.com/weimanc/game-boy-zero-link-board
- RP2040-Zero pinout: https://www.waveshare.com/wiki/RP2040-Zero
