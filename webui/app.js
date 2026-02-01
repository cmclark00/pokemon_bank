/**
 * Pokemon Bank WebUI - Main Application
 */

// State
let currentSlot = -1;
let pokemonList = [];

// DOM Elements
const connectBtn = document.getElementById('connect-btn');
const disconnectBtn = document.getElementById('disconnect-btn');
const statusIndicator = document.getElementById('status-indicator');
const statusText = document.getElementById('status-text');
const deviceInfo = document.getElementById('device-info');
const storageCount = document.getElementById('storage-count');
const storageMax = document.getElementById('storage-max');
const pokemonGrid = document.getElementById('pokemon-grid');
const modal = document.getElementById('pokemon-modal');

/**
 * Initialize the application
 */
function init() {
    // Check WebSerial support
    if (!PokemonBankSerial.isSupported()) {
        statusText.textContent = 'WebSerial not supported';
        connectBtn.disabled = true;
        pokemonGrid.innerHTML = '<p class="placeholder">WebSerial is not supported in this browser. Please use Chrome or Edge.</p>';
        return;
    }

    // Set up event handlers
    connectBtn.addEventListener('click', handleConnect);
    disconnectBtn.addEventListener('click', handleDisconnect);

    // Set up serial callbacks
    pokemonSerial.onConnect = onDeviceConnected;
    pokemonSerial.onDisconnect = onDeviceDisconnected;
    pokemonSerial.onData = onDataReceived;

    // Populate move selects
    populateMoveSelects();
}

/**
 * Populate move dropdown selects with all Gen I moves
 */
function populateMoveSelects() {
    const moves = getMoveOptions();
    
    for (let i = 0; i < 4; i++) {
        const select = document.getElementById(`edit-move-${i}`);
        select.innerHTML = '';
        
        moves.forEach(move => {
            const option = document.createElement('option');
            option.value = move.value;
            option.textContent = move.name;
            select.appendChild(option);
        });
    }
}

/**
 * Handle connect button click
 */
async function handleConnect() {
    try {
        connectBtn.disabled = true;
        statusText.textContent = 'Connecting...';
        await pokemonSerial.connect();
    } catch (error) {
        console.error('Connection failed:', error);
        statusText.textContent = 'Connection failed';
        connectBtn.disabled = false;
    }
}

/**
 * Handle disconnect button click
 */
async function handleDisconnect() {
    await pokemonSerial.disconnect();
}

/**
 * Called when device is connected
 */
async function onDeviceConnected() {
    statusIndicator.className = 'connected';
    statusText.textContent = 'Connected';
    connectBtn.disabled = true;
    disconnectBtn.disabled = false;
    deviceInfo.classList.remove('hidden');

    // Refresh Pokemon list
    await refreshPokemonList();
}

/**
 * Called when device is disconnected
 */
function onDeviceDisconnected() {
    statusIndicator.className = 'disconnected';
    statusText.textContent = 'Disconnected';
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
    deviceInfo.classList.add('hidden');
    pokemonGrid.innerHTML = '<p class="placeholder">Connect to device to view Pokemon</p>';
    pokemonList = [];
}

/**
 * Called when data is received from device
 */
function onDataReceived(data) {
    console.log('Data received:', data);
}

/**
 * Refresh the Pokemon list from device
 */
async function refreshPokemonList() {
    try {
        pokemonGrid.innerHTML = '<p class="placeholder">Loading...</p>';
        
        pokemonList = await pokemonSerial.listPokemon();
        
        storageCount.textContent = pokemonList.length;
        
        renderPokemonGrid();
    } catch (error) {
        console.error('Failed to load Pokemon list:', error);
        pokemonGrid.innerHTML = '<p class="placeholder">Failed to load Pokemon. Try reconnecting.</p>';
    }
}

/**
 * Render the Pokemon grid
 */
function renderPokemonGrid() {
    if (pokemonList.length === 0) {
        pokemonGrid.innerHTML = '<p class="placeholder">No Pokemon stored. Trade some from your Game Boy!</p>';
        return;
    }

    pokemonGrid.innerHTML = '';

    pokemonList.forEach((pokemon, index) => {
        const card = document.createElement('div');
        card.className = 'pokemon-card';
        card.onclick = () => openPokemonDetail(index);

        const spriteUrl = getSpriteUrl(pokemon.index);
        const speciesName = getSpeciesName(pokemon.index);

        card.innerHTML = `
            <img src="${spriteUrl}" alt="${speciesName}" onerror="this.src='data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 width=%2296%22 height=%2296%22><text x=%2248%22 y=%2260%22 text-anchor=%22middle%22 font-size=%2240%22>?</text></svg>'">
            <div class="name">${pokemon.nickname || speciesName}</div>
            <div class="level">Lv. ${pokemon.level}</div>
            <div class="species">${speciesName}</div>
        `;

        pokemonGrid.appendChild(card);
    });
}

/**
 * Open Pokemon detail modal
 */
async function openPokemonDetail(slot) {
    currentSlot = slot;

    try {
        // Get full Pokemon data
        const pokemon = await pokemonSerial.getPokemon(slot);
        
        if (pokemon.error) {
            alert('Failed to load Pokemon details: ' + pokemon.error);
            return;
        }

        // Update modal with Pokemon data
        const spriteUrl = getSpriteUrl(pokemon.index);
        const speciesName = getSpeciesName(pokemon.index);

        document.getElementById('detail-sprite').src = spriteUrl;
        document.getElementById('detail-nickname').textContent = pokemon.nickname || speciesName;
        document.getElementById('detail-species').textContent = speciesName;
        document.getElementById('detail-level').textContent = `Level: ${pokemon.level}`;
        document.getElementById('detail-ot').textContent = `OT: ${pokemon.ot_name} (${pokemon.ot_id})`;

        // Stats
        const maxStat = 255; // Max visible stat value for bar scaling
        
        document.getElementById('stat-hp').textContent = `${pokemon.hp}/${pokemon.max_hp}`;
        document.getElementById('stat-hp-bar').style.width = `${Math.min(pokemon.max_hp / maxStat * 100, 100)}%`;
        
        document.getElementById('stat-atk').textContent = pokemon.atk;
        document.getElementById('stat-atk-bar').style.width = `${Math.min(pokemon.atk / maxStat * 100, 100)}%`;
        
        document.getElementById('stat-def').textContent = pokemon.def;
        document.getElementById('stat-def-bar').style.width = `${Math.min(pokemon.def / maxStat * 100, 100)}%`;
        
        document.getElementById('stat-spd').textContent = pokemon.spd;
        document.getElementById('stat-spd-bar').style.width = `${Math.min(pokemon.spd / maxStat * 100, 100)}%`;
        
        document.getElementById('stat-spc').textContent = pokemon.spc;
        document.getElementById('stat-spc-bar').style.width = `${Math.min(pokemon.spc / maxStat * 100, 100)}%`;

        // Moves
        const movesGrid = document.getElementById('moves-grid');
        movesGrid.innerHTML = '';
        pokemon.moves.forEach((moveIndex, i) => {
            const moveName = getMoveName(moveIndex);
            const pp = pokemon.pp ? pokemon.pp[i] : '??';
            const div = document.createElement('div');
            div.className = 'move-slot';
            div.textContent = moveName !== '-' ? `${moveName} (PP: ${pp})` : '-';
            movesGrid.appendChild(div);
        });

        // Edit form
        document.getElementById('edit-nickname').value = pokemon.nickname || '';
        
        for (let i = 0; i < 4; i++) {
            document.getElementById(`edit-move-${i}`).value = pokemon.moves[i];
        }

        // Show modal
        modal.classList.remove('hidden');

    } catch (error) {
        console.error('Failed to load Pokemon details:', error);
        alert('Failed to load Pokemon details. Please try again.');
    }
}

/**
 * Close the modal
 */
function closeModal() {
    modal.classList.add('hidden');
    currentSlot = -1;
}

/**
 * Save Pokemon changes
 */
async function savePokemon() {
    if (currentSlot < 0) return;

    try {
        const nickname = document.getElementById('edit-nickname').value.trim().toUpperCase();
        const moves = [
            parseInt(document.getElementById('edit-move-0').value),
            parseInt(document.getElementById('edit-move-1').value),
            parseInt(document.getElementById('edit-move-2').value),
            parseInt(document.getElementById('edit-move-3').value)
        ];

        const response = await pokemonSerial.setPokemon(currentSlot, {
            nickname: nickname,
            moves: moves
        });

        if (response.ok) {
            alert('Pokemon saved successfully!');
            closeModal();
            await refreshPokemonList();
        } else {
            alert('Failed to save: ' + (response.error || 'Unknown error'));
        }
    } catch (error) {
        console.error('Save failed:', error);
        alert('Failed to save Pokemon: ' + error.message);
    }
}

/**
 * Delete Pokemon from slot
 */
async function deletePokemon() {
    if (currentSlot < 0) return;

    if (!confirm('Are you sure you want to delete this Pokemon? This cannot be undone!')) {
        return;
    }

    try {
        const response = await pokemonSerial.deletePokemon(currentSlot);

        if (response.ok) {
            alert('Pokemon deleted.');
            closeModal();
            await refreshPokemonList();
        } else {
            alert('Failed to delete: ' + (response.error || 'Unknown error'));
        }
    } catch (error) {
        console.error('Delete failed:', error);
        alert('Failed to delete Pokemon: ' + error.message);
    }
}

// Close modal on escape key
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && !modal.classList.contains('hidden')) {
        closeModal();
    }
});

// Close modal on backdrop click
modal.addEventListener('click', (e) => {
    if (e.target === modal) {
        closeModal();
    }
});

// Initialize on load
document.addEventListener('DOMContentLoaded', init);
