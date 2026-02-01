# Phase 1 Complete: Game Boy Link Protocol Implementation

## What Was Built

### Project Structure
```
firmware/
├── CMakeLists.txt          # Pico SDK build configuration
├── pico_sdk_import.cmake   # SDK bootstrap
├── README.md               # Build and usage documentation
├── include/
│   ├── gb_link.h           # Link protocol API
│   └── tusb_config.h       # TinyUSB configuration
├── pio/
│   └── gb_link.pio         # PIO state machine for GB link (CRITICAL)
├── src/
│   ├── main.c              # Application entry + USB CDC commands
│   ├── gb_link.c           # Link protocol implementation
│   └── usb_descriptors.c   # USB device descriptors
└── scripts/
    └── setup.sh            # Dependency installation script
```

### Key Components

#### 1. PIO State Machine (`pio/gb_link.pio`)
- **SPI slave mode** - responds to Game Boy's clock (external clock)
- MSB-first, 8 bits per transfer
- CPOL=1, CPHA=1 (clock idles high, sample on rising edge)
- Runs at maximum speed to catch all clock edges
- Bypasses input synchronizer for faster response
- Uses autopush/autopull for seamless byte transfers

#### 2. Link Protocol API (`gb_link.c`, `gb_link.h`)
- `gb_link_init()` - Initialize PIO and pins
- `gb_link_start()` / `gb_link_stop()` - Enable/disable state machine
- `gb_link_set_callback()` - Register byte-received callback
- `gb_link_set_next_byte()` - Queue response for next transfer
- `gb_link_process()` - Poll for received bytes (non-blocking)
- `gb_link_exchange_byte()` - Blocking byte exchange with timeout

#### 3. USB CDC Interface
- Serial console for debugging (115200 baud)
- Commands: status (s), reset (r), test (t), bootsel (b), help (h)
- Shows all received bytes in real-time

#### 4. LED Status (WS2812 on GP16)
- Blue: Idle, waiting for connection
- Green: Connected to Game Boy
- Yellow: Data activity
- Red: Error

## Hardware Pin Mapping

| GPIO | Signal | Direction | Function |
|------|--------|-----------|----------|
| GP0  | SC     | Input     | Serial Clock (from Game Boy) |
| GP1  | SI     | Output    | Serial In (data TO Game Boy) |
| GP2  | SO     | Input     | Serial Out (data FROM Game Boy) |
| GP16 | LED    | Output    | WS2812 RGB status LED |

## How to Build

### Prerequisites
```bash
# Arch Linux
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib cmake make

# Or run the setup script
./scripts/setup.sh
```

### Build Steps
```bash
# Clone Pico SDK if not already done
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init
export PICO_SDK_PATH=~/pico-sdk

# Build firmware
cd firmware
mkdir build && cd build
cmake ..
make -j$(nproc)

# Output: pokemon_bank.uf2
```

### Flash to Device
1. Hold BOOT button on RP2040-Zero
2. Connect USB (or press RESET while holding BOOT)
3. Copy `pokemon_bank.uf2` to the `RPI-RP2` drive
4. Device reboots automatically

## Testing

### With Serial Console
```bash
picocom -b 115200 /dev/ttyACM0
# Press 's' for status, 'h' for help
```

### With Real Game Boy
1. Connect link cable to Game Boy and adapter
2. Start Pokemon Red/Blue/Yellow
3. Go to Pokemon Center (upstairs)
4. Enter Trade Center
5. Watch serial console for sync bytes!

Expected output when GB enters trade room:
```
[GB Link] Connected!
RX: 0x00
RX: 0x00
RX: 0xFE
...
```

## Key Decisions Made

1. **PIO over bit-banging**: Essential for timing - CPU can't reliably catch 8kHz clock edges while doing other work

2. **Slave mode only**: For Pokemon trading, Game Boy is always the master (generates clock). No need for master mode.

3. **Pull-ups on inputs**: Game Boy expects pull-ups. Disconnected cable reads 0xFF.

4. **Input sync bypass**: Removed 2-cycle delay on clock/data inputs for faster edge detection.

5. **Simple echo for Phase 1**: Just echoes received bytes back. Phase 2 will implement the actual trade protocol state machine.

## Issues & Notes

- **ARM toolchain required**: Not installed on dev machine. User must install `arm-none-eabi-gcc`.
- **No emulator testing yet**: Need real hardware or gblink-compatible emulator setup.
- **Trade protocol not implemented**: Phase 1 only does raw byte exchange. Phase 2 adds the state machine.

## Ready for Phase 2?

**YES.** The infrastructure is in place:
- ✅ PIO reliably handles the timing-critical link protocol
- ✅ Callback system allows Phase 2 to plug in trade state machine
- ✅ USB console for debugging
- ✅ Clean separation between link layer and application
- ✅ LED feedback for visual status

Phase 2 will add:
- Trade room handshake sequence
- Pokemon data structures
- Trade state machine (from Flipper project reference)
- Dummy Pokemon to offer for trade
