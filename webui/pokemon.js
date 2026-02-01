/**
 * Pokemon Data Definitions and Utilities
 * Gen I Pokemon Bank WebUI
 */

// Species index to name mapping (Gen I internal indices)
const SPECIES_INDEX = {
    0x01: "Rhydon",
    0x02: "Kangaskhan",
    0x03: "Nidoran♂",
    0x04: "Clefairy",
    0x05: "Spearow",
    0x06: "Voltorb",
    0x07: "Nidoking",
    0x08: "Slowbro",
    0x09: "Ivysaur",
    0x0A: "Exeggutor",
    0x0B: "Lickitung",
    0x0C: "Exeggcute",
    0x0D: "Grimer",
    0x0E: "Gengar",
    0x0F: "Nidoran♀",
    0x10: "Nidoqueen",
    0x11: "Cubone",
    0x12: "Rhyhorn",
    0x13: "Lapras",
    0x14: "Arcanine",
    0x15: "Mew",
    0x16: "Gyarados",
    0x17: "Shellder",
    0x18: "Tentacool",
    0x19: "Gastly",
    0x1A: "Scyther",
    0x1B: "Staryu",
    0x1C: "Blastoise",
    0x1D: "Pinsir",
    0x1E: "Tangela",
    0x21: "Growlithe",
    0x22: "Onix",
    0x23: "Fearow",
    0x24: "Pidgey",
    0x25: "Slowpoke",
    0x26: "Kadabra",
    0x27: "Graveler",
    0x28: "Chansey",
    0x29: "Machoke",
    0x2A: "Mr. Mime",
    0x2B: "Hitmonlee",
    0x2C: "Hitmonchan",
    0x2D: "Arbok",
    0x2E: "Parasect",
    0x2F: "Psyduck",
    0x30: "Drowzee",
    0x31: "Golem",
    0x33: "Magmar",
    0x35: "Electabuzz",
    0x36: "Magneton",
    0x37: "Koffing",
    0x39: "Mankey",
    0x3A: "Seel",
    0x3B: "Diglett",
    0x3C: "Tauros",
    0x40: "Farfetch'd",
    0x41: "Venonat",
    0x42: "Dragonite",
    0x46: "Doduo",
    0x47: "Poliwag",
    0x48: "Jynx",
    0x49: "Moltres",
    0x4A: "Articuno",
    0x4B: "Zapdos",
    0x4C: "Ditto",
    0x4D: "Meowth",
    0x4E: "Krabby",
    0x52: "Vulpix",
    0x53: "Ninetales",
    0x54: "Pikachu",
    0x55: "Raichu",
    0x58: "Dratini",
    0x59: "Dragonair",
    0x5A: "Kabuto",
    0x5B: "Kabutops",
    0x5C: "Horsea",
    0x5D: "Seadra",
    0x60: "Sandshrew",
    0x61: "Sandslash",
    0x62: "Omanyte",
    0x63: "Omastar",
    0x64: "Jigglypuff",
    0x65: "Wigglytuff",
    0x66: "Eevee",
    0x67: "Flareon",
    0x68: "Jolteon",
    0x69: "Vaporeon",
    0x6A: "Machop",
    0x6B: "Zubat",
    0x6C: "Ekans",
    0x6D: "Paras",
    0x6E: "Poliwhirl",
    0x6F: "Poliwrath",
    0x70: "Weedle",
    0x71: "Kakuna",
    0x72: "Beedrill",
    0x74: "Dodrio",
    0x75: "Primeape",
    0x76: "Dugtrio",
    0x77: "Venomoth",
    0x78: "Dewgong",
    0x7B: "Caterpie",
    0x7C: "Metapod",
    0x7D: "Butterfree",
    0x7E: "Machamp",
    0x80: "Golduck",
    0x81: "Hypno",
    0x82: "Golbat",
    0x83: "Mewtwo",
    0x84: "Snorlax",
    0x85: "Magikarp",
    0x88: "Muk",
    0x8A: "Kingler",
    0x8B: "Cloyster",
    0x8D: "Electrode",
    0x8E: "Clefable",
    0x8F: "Weezing",
    0x90: "Persian",
    0x91: "Marowak",
    0x93: "Haunter",
    0x94: "Abra",
    0x95: "Alakazam",
    0x96: "Pidgeotto",
    0x97: "Pidgeot",
    0x98: "Starmie",
    0x99: "Bulbasaur",
    0x9A: "Venusaur",
    0x9B: "Tentacruel",
    0x9D: "Goldeen",
    0x9E: "Seaking",
    0xA3: "Ponyta",
    0xA4: "Rapidash",
    0xA5: "Rattata",
    0xA6: "Raticate",
    0xA7: "Nidorino",
    0xA8: "Nidorina",
    0xA9: "Geodude",
    0xAA: "Porygon",
    0xAB: "Aerodactyl",
    0xAD: "Magnemite",
    0xB0: "Charmander",
    0xB1: "Squirtle",
    0xB2: "Charmeleon",
    0xB3: "Wartortle",
    0xB4: "Charizard",
    0xB9: "Oddish",
    0xBA: "Gloom",
    0xBB: "Vileplume",
    0xBC: "Bellsprout",
    0xBD: "Weepinbell",
    0xBE: "Victreebel"
};

// Species index to Pokedex number (for sprite lookup)
const SPECIES_TO_DEX = {
    0x99: 1,   // Bulbasaur
    0x09: 2,   // Ivysaur
    0x9A: 3,   // Venusaur
    0xB0: 4,   // Charmander
    0xB2: 5,   // Charmeleon
    0xB4: 6,   // Charizard
    0xB1: 7,   // Squirtle
    0xB3: 8,   // Wartortle
    0x1C: 9,   // Blastoise
    0x7B: 10,  // Caterpie
    0x7C: 11,  // Metapod
    0x7D: 12,  // Butterfree
    0x70: 13,  // Weedle
    0x71: 14,  // Kakuna
    0x72: 15,  // Beedrill
    0x24: 16,  // Pidgey
    0x96: 17,  // Pidgeotto
    0x97: 18,  // Pidgeot
    0xA5: 19,  // Rattata
    0xA6: 20,  // Raticate
    0x05: 21,  // Spearow
    0x23: 22,  // Fearow
    0x6C: 23,  // Ekans
    0x2D: 24,  // Arbok
    0x54: 25,  // Pikachu
    0x55: 26,  // Raichu
    0x60: 27,  // Sandshrew
    0x61: 28,  // Sandslash
    0x0F: 29,  // Nidoran♀
    0xA8: 30,  // Nidorina
    0x10: 31,  // Nidoqueen
    0x03: 32,  // Nidoran♂
    0xA7: 33,  // Nidorino
    0x07: 34,  // Nidoking
    0x04: 35,  // Clefairy
    0x8E: 36,  // Clefable
    0x52: 37,  // Vulpix
    0x53: 38,  // Ninetales
    0x64: 39,  // Jigglypuff
    0x65: 40,  // Wigglytuff
    0x6B: 41,  // Zubat
    0x82: 42,  // Golbat
    0xB9: 43,  // Oddish
    0xBA: 44,  // Gloom
    0xBB: 45,  // Vileplume
    0x6D: 46,  // Paras
    0x2E: 47,  // Parasect
    0x41: 48,  // Venonat
    0x77: 49,  // Venomoth
    0x3B: 50,  // Diglett
    0x76: 51,  // Dugtrio
    0x4D: 52,  // Meowth
    0x90: 53,  // Persian
    0x2F: 54,  // Psyduck
    0x80: 55,  // Golduck
    0x39: 56,  // Mankey
    0x75: 57,  // Primeape
    0x21: 58,  // Growlithe
    0x14: 59,  // Arcanine
    0x47: 60,  // Poliwag
    0x6E: 61,  // Poliwhirl
    0x6F: 62,  // Poliwrath
    0x94: 63,  // Abra
    0x26: 64,  // Kadabra
    0x95: 65,  // Alakazam
    0x6A: 66,  // Machop
    0x29: 67,  // Machoke
    0x7E: 68,  // Machamp
    0xBC: 69,  // Bellsprout
    0xBD: 70,  // Weepinbell
    0xBE: 71,  // Victreebel
    0x18: 72,  // Tentacool
    0x9B: 73,  // Tentacruel
    0xA9: 74,  // Geodude
    0x27: 75,  // Graveler
    0x31: 76,  // Golem
    0xA3: 77,  // Ponyta
    0xA4: 78,  // Rapidash
    0x25: 79,  // Slowpoke
    0x08: 80,  // Slowbro
    0xAD: 81,  // Magnemite
    0x36: 82,  // Magneton
    0x40: 83,  // Farfetch'd
    0x46: 84,  // Doduo
    0x74: 85,  // Dodrio
    0x3A: 86,  // Seel
    0x78: 87,  // Dewgong
    0x0D: 88,  // Grimer
    0x88: 89,  // Muk
    0x17: 90,  // Shellder
    0x8B: 91,  // Cloyster
    0x19: 92,  // Gastly
    0x93: 93,  // Haunter
    0x0E: 94,  // Gengar
    0x22: 95,  // Onix
    0x30: 96,  // Drowzee
    0x81: 97,  // Hypno
    0x4E: 98,  // Krabby
    0x8A: 99,  // Kingler
    0x06: 100, // Voltorb
    0x8D: 101, // Electrode
    0x0C: 102, // Exeggcute
    0x0A: 103, // Exeggutor
    0x11: 104, // Cubone
    0x91: 105, // Marowak
    0x2B: 106, // Hitmonlee
    0x2C: 107, // Hitmonchan
    0x0B: 108, // Lickitung
    0x37: 109, // Koffing
    0x8F: 110, // Weezing
    0x12: 111, // Rhyhorn
    0x01: 112, // Rhydon
    0x28: 113, // Chansey
    0x1E: 114, // Tangela
    0x02: 115, // Kangaskhan
    0x5C: 116, // Horsea
    0x5D: 117, // Seadra
    0x9D: 118, // Goldeen
    0x9E: 119, // Seaking
    0x1B: 120, // Staryu
    0x98: 121, // Starmie
    0x2A: 122, // Mr. Mime
    0x1A: 123, // Scyther
    0x48: 124, // Jynx
    0x35: 125, // Electabuzz
    0x33: 126, // Magmar
    0x1D: 127, // Pinsir
    0x3C: 128, // Tauros
    0x85: 129, // Magikarp
    0x16: 130, // Gyarados
    0x13: 131, // Lapras
    0x4C: 132, // Ditto
    0x66: 133, // Eevee
    0x69: 134, // Vaporeon
    0x68: 135, // Jolteon
    0x67: 136, // Flareon
    0xAA: 137, // Porygon
    0x62: 138, // Omanyte
    0x63: 139, // Omastar
    0x5A: 140, // Kabuto
    0x5B: 141, // Kabutops
    0xAB: 142, // Aerodactyl
    0x84: 143, // Snorlax
    0x4A: 144, // Articuno
    0x4B: 145, // Zapdos
    0x49: 146, // Moltres
    0x58: 147, // Dratini
    0x59: 148, // Dragonair
    0x42: 149, // Dragonite
    0x83: 150, // Mewtwo
    0x15: 151  // Mew
};

// Move index to name mapping (Gen I)
const MOVES = {
    0x00: "-",
    0x01: "Pound",
    0x02: "Karate Chop",
    0x03: "Double Slap",
    0x04: "Comet Punch",
    0x05: "Mega Punch",
    0x06: "Pay Day",
    0x07: "Fire Punch",
    0x08: "Ice Punch",
    0x09: "Thunder Punch",
    0x0A: "Scratch",
    0x0B: "Vice Grip",
    0x0C: "Guillotine",
    0x0D: "Razor Wind",
    0x0E: "Swords Dance",
    0x0F: "Cut",
    0x10: "Gust",
    0x11: "Wing Attack",
    0x12: "Whirlwind",
    0x13: "Fly",
    0x14: "Bind",
    0x15: "Slam",
    0x16: "Vine Whip",
    0x17: "Stomp",
    0x18: "Double Kick",
    0x19: "Mega Kick",
    0x1A: "Jump Kick",
    0x1B: "Rolling Kick",
    0x1C: "Sand Attack",
    0x1D: "Headbutt",
    0x1E: "Horn Attack",
    0x1F: "Fury Attack",
    0x20: "Horn Drill",
    0x21: "Tackle",
    0x22: "Body Slam",
    0x23: "Wrap",
    0x24: "Take Down",
    0x25: "Thrash",
    0x26: "Double-Edge",
    0x27: "Tail Whip",
    0x28: "Poison Sting",
    0x29: "Twineedle",
    0x2A: "Pin Missile",
    0x2B: "Leer",
    0x2C: "Bite",
    0x2D: "Growl",
    0x2E: "Roar",
    0x2F: "Sing",
    0x30: "Supersonic",
    0x31: "Sonic Boom",
    0x32: "Disable",
    0x33: "Acid",
    0x34: "Ember",
    0x35: "Flamethrower",
    0x36: "Mist",
    0x37: "Water Gun",
    0x38: "Hydro Pump",
    0x39: "Surf",
    0x3A: "Ice Beam",
    0x3B: "Blizzard",
    0x3C: "Psybeam",
    0x3D: "Bubble Beam",
    0x3E: "Aurora Beam",
    0x3F: "Hyper Beam",
    0x40: "Peck",
    0x41: "Drill Peck",
    0x42: "Submission",
    0x43: "Low Kick",
    0x44: "Counter",
    0x45: "Seismic Toss",
    0x46: "Strength",
    0x47: "Absorb",
    0x48: "Mega Drain",
    0x49: "Leech Seed",
    0x4A: "Growth",
    0x4B: "Razor Leaf",
    0x4C: "Solar Beam",
    0x4D: "Poison Powder",
    0x4E: "Stun Spore",
    0x4F: "Sleep Powder",
    0x50: "Petal Dance",
    0x51: "String Shot",
    0x52: "Dragon Rage",
    0x53: "Fire Spin",
    0x54: "Thunder Shock",
    0x55: "Thunderbolt",
    0x56: "Thunder Wave",
    0x57: "Thunder",
    0x58: "Rock Throw",
    0x59: "Earthquake",
    0x5A: "Fissure",
    0x5B: "Dig",
    0x5C: "Toxic",
    0x5D: "Confusion",
    0x5E: "Psychic",
    0x5F: "Hypnosis",
    0x60: "Meditate",
    0x61: "Agility",
    0x62: "Quick Attack",
    0x63: "Rage",
    0x64: "Teleport",
    0x65: "Night Shade",
    0x66: "Mimic",
    0x67: "Screech",
    0x68: "Double Team",
    0x69: "Recover",
    0x6A: "Harden",
    0x6B: "Minimize",
    0x6C: "Smokescreen",
    0x6D: "Confuse Ray",
    0x6E: "Withdraw",
    0x6F: "Defense Curl",
    0x70: "Barrier",
    0x71: "Light Screen",
    0x72: "Haze",
    0x73: "Reflect",
    0x74: "Focus Energy",
    0x75: "Bide",
    0x76: "Metronome",
    0x77: "Mirror Move",
    0x78: "Self-Destruct",
    0x79: "Egg Bomb",
    0x7A: "Lick",
    0x7B: "Smog",
    0x7C: "Sludge",
    0x7D: "Bone Club",
    0x7E: "Fire Blast",
    0x7F: "Waterfall",
    0x80: "Clamp",
    0x81: "Swift",
    0x82: "Skull Bash",
    0x83: "Spike Cannon",
    0x84: "Constrict",
    0x85: "Amnesia",
    0x86: "Kinesis",
    0x87: "Soft-Boiled",
    0x88: "High Jump Kick",
    0x89: "Glare",
    0x8A: "Dream Eater",
    0x8B: "Poison Gas",
    0x8C: "Barrage",
    0x8D: "Leech Life",
    0x8E: "Lovely Kiss",
    0x8F: "Sky Attack",
    0x90: "Transform",
    0x91: "Bubble",
    0x92: "Dizzy Punch",
    0x93: "Spore",
    0x94: "Flash",
    0x95: "Psywave",
    0x96: "Splash",
    0x97: "Acid Armor",
    0x98: "Crabhammer",
    0x99: "Explosion",
    0x9A: "Fury Swipes",
    0x9B: "Bonemerang",
    0x9C: "Rest",
    0x9D: "Rock Slide",
    0x9E: "Hyper Fang",
    0x9F: "Sharpen",
    0xA0: "Conversion",
    0xA1: "Tri Attack",
    0xA2: "Super Fang",
    0xA3: "Slash",
    0xA4: "Substitute",
    0xA5: "Struggle"
};

/**
 * Get species name from internal index
 */
function getSpeciesName(index) {
    return SPECIES_INDEX[index] || `Unknown (0x${index.toString(16).toUpperCase()})`;
}

/**
 * Get Pokedex number from internal index
 */
function getDexNumber(index) {
    return SPECIES_TO_DEX[index] || 0;
}

/**
 * Get sprite URL for a Pokemon
 * Using PokeAPI sprites which work well for Gen I
 */
function getSpriteUrl(speciesIndex) {
    const dexNum = getDexNumber(speciesIndex);
    if (dexNum === 0) {
        // Fallback - question mark or placeholder
        return 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="96" height="96" viewBox="0 0 96 96"><text x="48" y="60" text-anchor="middle" font-size="40">?</text></svg>';
    }
    // Use PokeAPI sprite endpoint (Gen I compatible)
    return `https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/${dexNum}.png`;
}

/**
 * Get move name from index
 */
function getMoveName(index) {
    return MOVES[index] || "-";
}

/**
 * Get all available moves as options for select
 */
function getMoveOptions() {
    return Object.entries(MOVES).map(([index, name]) => ({
        value: parseInt(index),
        name: name
    }));
}

/**
 * Swap bytes for 16-bit value (big-endian to little-endian)
 */
function swap16(val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

/**
 * Parse Pokemon party data from raw bytes
 */
function parsePokemonParty(bytes) {
    if (bytes.length < 44) {
        console.error('Invalid party data length:', bytes.length);
        return null;
    }

    // Read 16-bit values and swap from big-endian
    const readU16BE = (offset) => (bytes[offset] << 8) | bytes[offset + 1];

    return {
        index: bytes[0],
        hp: readU16BE(1),
        level: bytes[3],
        status: bytes[4],
        type1: bytes[5],
        type2: bytes[6],
        catchRate: bytes[7],
        moves: [bytes[8], bytes[9], bytes[10], bytes[11]],
        otId: readU16BE(12),
        exp: (bytes[14] << 16) | (bytes[15] << 8) | bytes[16],
        hpEv: readU16BE(17),
        atkEv: readU16BE(19),
        defEv: readU16BE(21),
        spdEv: readU16BE(23),
        spcEv: readU16BE(25),
        iv: readU16BE(27),
        pp: [bytes[29], bytes[30], bytes[31], bytes[32]],
        levelAgain: bytes[33],
        maxHp: readU16BE(34),
        atk: readU16BE(36),
        def: readU16BE(38),
        spd: readU16BE(40),
        spc: readU16BE(42)
    };
}

/**
 * Format Pokemon data for display
 */
function formatPokemonForDisplay(pokemon) {
    return {
        ...pokemon,
        speciesName: getSpeciesName(pokemon.index),
        spriteUrl: getSpriteUrl(pokemon.index),
        moveNames: pokemon.moves.map(m => getMoveName(m)),
        dexNumber: getDexNumber(pokemon.index)
    };
}
