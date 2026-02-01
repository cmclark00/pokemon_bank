#!/bin/bash
# Setup script for Pokemon Bank firmware development
# Run from the firmware directory

set -e

echo "=== Pokemon Bank Firmware Setup ==="
echo

# Detect OS
if [ -f /etc/arch-release ]; then
    DISTRO="arch"
elif [ -f /etc/debian_version ]; then
    DISTRO="debian"
else
    DISTRO="unknown"
fi

echo "Detected distro: $DISTRO"
echo

# Install ARM toolchain
echo "Installing ARM toolchain..."
case $DISTRO in
    arch)
        sudo pacman -S --needed arm-none-eabi-gcc arm-none-eabi-newlib cmake make
        ;;
    debian)
        sudo apt-get update
        sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi cmake make
        ;;
    *)
        echo "Unknown distro. Please install ARM toolchain manually:"
        echo "  - arm-none-eabi-gcc"
        echo "  - arm-none-eabi-newlib"
        echo "  - cmake"
        echo "  - make"
        exit 1
        ;;
esac

# Check if PICO_SDK_PATH is set
if [ -z "$PICO_SDK_PATH" ]; then
    echo
    echo "PICO_SDK_PATH not set. Installing Pico SDK..."
    
    SDK_DIR="$HOME/pico-sdk"
    
    if [ ! -d "$SDK_DIR" ]; then
        git clone https://github.com/raspberrypi/pico-sdk.git "$SDK_DIR"
        cd "$SDK_DIR"
        git submodule update --init
        cd -
    fi
    
    echo
    echo "Add this to your ~/.bashrc or ~/.zshrc:"
    echo "  export PICO_SDK_PATH=$SDK_DIR"
    echo
    echo "Then run: source ~/.bashrc"
    
    export PICO_SDK_PATH="$SDK_DIR"
fi

echo
echo "PICO_SDK_PATH = $PICO_SDK_PATH"
echo

# Verify toolchain
echo "Verifying toolchain..."
arm-none-eabi-gcc --version | head -1
cmake --version | head -1

echo
echo "=== Setup Complete ==="
echo
echo "To build:"
echo "  cd firmware"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
echo
echo "Output: build/pokemon_bank.uf2"
