/**
 * WebSerial Handler for Pokemon Bank
 * Handles connection to RP2040 via USB CDC
 */

class PokemonBankSerial {
    constructor() {
        this.port = null;
        this.reader = null;
        this.writer = null;
        this.readBuffer = '';
        this.isConnected = false;
        this.onConnect = null;
        this.onDisconnect = null;
        this.onData = null;
        this.responsePromise = null;
        this.responseResolve = null;
    }

    /**
     * Check if WebSerial is supported
     */
    static isSupported() {
        return 'serial' in navigator;
    }

    /**
     * Connect to the Pokemon Bank device
     */
    async connect() {
        if (!PokemonBankSerial.isSupported()) {
            throw new Error('WebSerial is not supported in this browser. Please use Chrome or Edge.');
        }

        try {
            // Request port with RP2040 vendor/product ID filter (optional)
            this.port = await navigator.serial.requestPort({
                // RP2040 default USB CDC doesn't have a specific VID/PID filter
                // so we allow any device
            });

            // Open with 115200 baud (standard for RP2040 USB CDC)
            await this.port.open({ baudRate: 115200 });

            // Set up reader and writer
            this.reader = this.port.readable.getReader();
            this.writer = this.port.writable.getWriter();

            this.isConnected = true;

            // Start reading
            this.readLoop();

            if (this.onConnect) {
                this.onConnect();
            }

            return true;
        } catch (error) {
            console.error('Connection failed:', error);
            throw error;
        }
    }

    /**
     * Disconnect from the device
     */
    async disconnect() {
        this.isConnected = false;

        try {
            if (this.reader) {
                await this.reader.cancel();
                this.reader.releaseLock();
                this.reader = null;
            }

            if (this.writer) {
                this.writer.releaseLock();
                this.writer = null;
            }

            if (this.port) {
                await this.port.close();
                this.port = null;
            }

            if (this.onDisconnect) {
                this.onDisconnect();
            }
        } catch (error) {
            console.error('Disconnect error:', error);
        }
    }

    /**
     * Continuous read loop
     */
    async readLoop() {
        while (this.isConnected && this.reader) {
            try {
                const { value, done } = await this.reader.read();

                if (done) {
                    break;
                }

                if (value) {
                    // Decode bytes to string
                    const text = new TextDecoder().decode(value);
                    this.readBuffer += text;

                    // Check for complete JSON responses
                    this.processBuffer();
                }
            } catch (error) {
                if (this.isConnected) {
                    console.error('Read error:', error);
                    this.disconnect();
                }
                break;
            }
        }
    }

    /**
     * Process read buffer looking for JSON responses
     */
    processBuffer() {
        // Look for JSON responses (start with { and end with })
        let startIdx = this.readBuffer.indexOf('{');

        while (startIdx !== -1) {
            // Find matching closing brace
            let depth = 0;
            let endIdx = -1;

            for (let i = startIdx; i < this.readBuffer.length; i++) {
                if (this.readBuffer[i] === '{') depth++;
                if (this.readBuffer[i] === '}') depth--;

                if (depth === 0) {
                    endIdx = i;
                    break;
                }
            }

            if (endIdx !== -1) {
                // Extract JSON
                const jsonStr = this.readBuffer.substring(startIdx, endIdx + 1);
                this.readBuffer = this.readBuffer.substring(endIdx + 1);

                try {
                    const data = JSON.parse(jsonStr);

                    // Resolve pending response promise if exists
                    if (this.responseResolve) {
                        this.responseResolve(data);
                        this.responseResolve = null;
                    }

                    if (this.onData) {
                        this.onData(data);
                    }
                } catch (e) {
                    console.warn('Invalid JSON received:', jsonStr);
                }

                // Look for next JSON
                startIdx = this.readBuffer.indexOf('{');
            } else {
                // Incomplete JSON, wait for more data
                break;
            }
        }

        // Trim buffer if it gets too large (prevent memory issues)
        if (this.readBuffer.length > 10000) {
            this.readBuffer = this.readBuffer.substring(this.readBuffer.length - 1000);
        }
    }

    /**
     * Send a command and wait for response
     */
    async sendCommand(cmd) {
        if (!this.isConnected || !this.writer) {
            throw new Error('Not connected');
        }

        // Create response promise
        const responsePromise = new Promise((resolve, reject) => {
            this.responseResolve = resolve;

            // Timeout after 5 seconds
            setTimeout(() => {
                if (this.responseResolve === resolve) {
                    this.responseResolve = null;
                    reject(new Error('Command timeout'));
                }
            }, 5000);
        });

        // Send JSON command
        const jsonStr = JSON.stringify(cmd) + '\n';
        const data = new TextEncoder().encode(jsonStr);

        await this.writer.write(data);

        return responsePromise;
    }

    /**
     * List all stored Pokemon
     */
    async listPokemon() {
        const response = await this.sendCommand({ cmd: 'list' });
        return response.pokemon || [];
    }

    /**
     * Get detailed Pokemon data for a slot
     */
    async getPokemon(slot) {
        const response = await this.sendCommand({ cmd: 'get', slot: slot });
        return response;
    }

    /**
     * Update Pokemon data in a slot
     */
    async setPokemon(slot, data) {
        const response = await this.sendCommand({ cmd: 'set', slot: slot, data: data });
        return response;
    }

    /**
     * Delete Pokemon from a slot
     */
    async deletePokemon(slot) {
        const response = await this.sendCommand({ cmd: 'delete', slot: slot });
        return response;
    }

    /**
     * Get device status
     */
    async getStatus() {
        const response = await this.sendCommand({ cmd: 'status' });
        return response;
    }
}

// Export singleton instance
const pokemonSerial = new PokemonBankSerial();
