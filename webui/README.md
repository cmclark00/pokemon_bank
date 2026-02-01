# Pokemon Bank WebUI

A browser-based interface for managing Pokemon stored on your Pokemon Bank device.

## Requirements

- **Browser**: Chrome or Edge (WebSerial API required)
- **USB Connection**: Pokemon Bank device connected via USB
- **Firmware**: v1.0+ with WebUI protocol support

## Quick Start

1. Connect your Pokemon Bank device via USB

2. Start the local server:
   ```bash
   python3 serve.py
   # or
   python serve.py
   ```

3. Open Chrome/Edge and navigate to:
   ```
   http://localhost:8000
   ```

4. Click "Connect to Pokemon Bank" and select your device

## Features

### View Pokemon
- See all stored Pokemon in a grid view
- View sprites, nicknames, levels, and species

### Pokemon Details
- Click any Pokemon to see full details:
  - Stats (HP, ATK, DEF, SPD, SPC) with visual bars
  - Moves and PP
  - Original Trainer info

### Edit Pokemon
- Modify nickname (up to 10 characters)
- Change moves from the Gen I move list
- Changes are saved directly to the device's flash storage

### Delete Pokemon
- Remove Pokemon from storage
- Frees up space for new trades

## Protocol

The WebUI communicates with the device using JSON commands over USB CDC:

```json
// List all Pokemon
-> {"cmd": "list"}
<- {"pokemon": [...]}

// Get Pokemon details
-> {"cmd": "get", "slot": 0}
<- {"slot": 0, "species": "Bulbasaur", ...}

// Update Pokemon
-> {"cmd": "set", "slot": 0, "data": {"nickname": "SPARKY", "moves": [84, 85, 86, 87]}}
<- {"ok": true}

// Delete Pokemon
-> {"cmd": "delete", "slot": 0}
<- {"ok": true}

// Get device status
-> {"cmd": "status"}
<- {"count": 5, "max": 20, ...}
```

## Sprites

Pokemon sprites are loaded from the [PokeAPI sprites repository](https://github.com/PokeAPI/sprites). 
An internet connection is required for sprites to display.

## Troubleshooting

### "WebSerial not supported"
- Make sure you're using Chrome or Edge
- WebSerial doesn't work in Firefox or Safari

### Device not appearing in connection dialog
- Make sure the device is connected via USB
- Try a different USB cable
- Check that the firmware is running (LED should be blue)

### Connection timeout
- Disconnect and reconnect the USB cable
- Press 'r' in a serial terminal to reset the device
- Re-flash the firmware if issues persist

### Changes not saving
- Make sure the device isn't in the middle of a trade
- Check for error messages in the browser console
- Verify flash storage isn't corrupted (use 'c' command to clear and start fresh)

## Development

The WebUI consists of:
- `index.html` - Main interface
- `styles.css` - Styling
- `pokemon.js` - Pokemon data definitions (species, moves, sprites)
- `serial.js` - WebSerial connection handler
- `app.js` - Main application logic

No build step required - just edit and refresh!
