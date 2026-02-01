# Pokemon Bank Firmware for RP2040-Zero
# Uses Game Boy Zero Link Board

cmake_minimum_required(VERSION 3.13)

# Initialize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

project(pokemon_bank C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(pokemon_bank
    src/main.c
    src/gb_link.c
    src/pokemon_data.c
    src/pokemon_trade.c
    src/pokemon_storage.c
    src/webui_protocol.c
    src/usb_descriptors.c
)

# Generate PIO header from .pio file
pico_generate_pio_header(pokemon_bank ${CMAKE_CURRENT_LIST_DIR}/pio/gb_link.pio)

target_include_directories(pokemon_bank PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Link libraries
target_link_libraries(pokemon_bank
    pico_stdlib
    pico_unique_id
    hardware_pio
    hardware_dma
    hardware_flash
    tinyusb_device
    tinyusb_board
    m  # Math library for sqrt()
)

# Enable USB output, disable UART
pico_enable_stdio_usb(pokemon_bank 1)
pico_enable_stdio_uart(pokemon_bank 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(pokemon_bank)

# Print memory usage
# pico_set_linker_script(pokemon_bank ${CMAKE_CURRENT_LIST_DIR}/memmap_custom.ld)
