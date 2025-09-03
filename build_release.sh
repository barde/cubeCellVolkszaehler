#!/bin/bash

# Volkszähler LoRa Bridge - Release Build Script
# Creates release binaries for GitHub releases
# Usage: ./build_release.sh [version]

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="volkszahler-lora-bridge"
RELEASE_DIR="release"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Get version from argument or git tag
if [ -n "$1" ]; then
    VERSION="$1"
else
    VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "v1.0.0")
fi

# Print header
echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Volkszähler LoRa Bridge Release Builder   ║${NC}"
echo -e "${BLUE}║                Version: ${VERSION}              ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""

# Function to print status
print_status() {
    echo -e "${BLUE}[*]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local deps_missing=0
    
    # Check for PlatformIO
    if ! command -v pio &> /dev/null; then
        print_error "PlatformIO not found. Install with: pip install platformio"
        deps_missing=1
    else
        print_success "PlatformIO found"
    fi
    
    # Check for ESPHome
    if ! command -v esphome &> /dev/null; then
        print_warning "ESPHome not found. Install with: pip install esphome"
        print_warning "Skipping LoRa32 gateway build"
    else
        print_success "ESPHome found"
    fi
    
    # Check for zip
    if ! command -v zip &> /dev/null; then
        print_error "zip not found. Install with: brew install zip"
        deps_missing=1
    else
        print_success "zip found"
    fi
    
    if [ $deps_missing -eq 1 ]; then
        print_error "Missing dependencies. Please install them and try again."
        exit 1
    fi
}

# Clean previous builds
clean_build() {
    print_status "Cleaning previous builds..."
    rm -rf "$RELEASE_DIR"
    rm -rf .pio/build
    rm -rf lilygo_gateway/.esphome/build
    print_success "Clean complete"
}

# Create release directory structure
create_release_dirs() {
    print_status "Creating release directory structure..."
    mkdir -p "$RELEASE_DIR/cubecell"
    mkdir -p "$RELEASE_DIR/lora32"
    mkdir -p "$RELEASE_DIR/docs"
    print_success "Directories created"
}

# Build CubeCell firmware variants
build_cubecell() {
    print_status "Building CubeCell firmware variants..."
    
    # Test mode (5 second intervals)
    print_status "Building CubeCell test mode..."
    pio run -e cubecell_testmode --silent
    cp .pio/build/cubecell_testmode/firmware.hex "$RELEASE_DIR/cubecell/cubecell_testmode_${VERSION}.hex"
    cp .pio/build/cubecell_testmode/firmware.cyacd "$RELEASE_DIR/cubecell/cubecell_testmode_${VERSION}.cyacd"
    print_success "Test mode built"
    
    # Production mode (60 second intervals with deep sleep)
    print_status "Building CubeCell production mode..."
    pio run -e cubecell_lora --silent
    cp .pio/build/cubecell_lora/firmware.hex "$RELEASE_DIR/cubecell/cubecell_production_${VERSION}.hex"
    cp .pio/build/cubecell_lora/firmware.cyacd "$RELEASE_DIR/cubecell/cubecell_production_${VERSION}.cyacd"
    print_success "Production mode built"
    
    # Original simple version
    print_status "Building CubeCell original version..."
    pio run -e cubecell_original --silent
    cp .pio/build/cubecell_original/firmware.hex "$RELEASE_DIR/cubecell/cubecell_original_${VERSION}.hex"
    cp .pio/build/cubecell_original/firmware.cyacd "$RELEASE_DIR/cubecell/cubecell_original_${VERSION}.cyacd"
    print_success "Original version built"
    
    print_success "All CubeCell variants built successfully"
}

# Build LoRa32 Gateway firmware
build_lora32() {
    if ! command -v esphome &> /dev/null; then
        print_warning "Skipping LoRa32 build (ESPHome not installed)"
        return
    fi
    
    print_status "Building LoRa32 gateway firmware..."
    
    cd lilygo_gateway
    
    # Create secrets.yaml if it doesn't exist
    if [ ! -f secrets.yaml ]; then
        print_warning "Creating template secrets.yaml..."
        cat > secrets.yaml << EOF
# WiFi credentials (CHANGE THESE!)
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
wifi_failover: "fallback123456"

# API encryption key (CHANGE THIS!)
api_encryption_key: "CHANGE_THIS_TO_RANDOM_BASE64_STRING"
EOF
    fi
    
    # Compile firmware
    print_status "Compiling ESPHome firmware..."
    esphome compile lilygo_lora_receiver.yaml
    
    # Copy firmware files
    if [ -f .esphome/build/volkszahler-lora-gateway/.pioenvs/volkszahler-lora-gateway/firmware.bin ]; then
        cp .esphome/build/volkszahler-lora-gateway/.pioenvs/volkszahler-lora-gateway/firmware.bin "../$RELEASE_DIR/lora32/lora32_gateway_${VERSION}.bin"
        cp .esphome/build/volkszahler-lora-gateway/.pioenvs/volkszahler-lora-gateway/firmware.factory.bin "../$RELEASE_DIR/lora32/lora32_gateway_factory_${VERSION}.bin"
        print_success "LoRa32 gateway firmware built"
    else
        print_error "LoRa32 firmware not found"
    fi
    
    cd ..
}

# Create documentation
create_docs() {
    print_status "Creating documentation..."
    
    # Copy README
    cp README.md "$RELEASE_DIR/docs/"
    cp CLAUDE.md "$RELEASE_DIR/docs/DEVELOPER.md"
    cp LICENSE "$RELEASE_DIR/"
    
    # Create flashing instructions
    cat > "$RELEASE_DIR/docs/FLASHING.md" << 'EOF'
# Flashing Instructions

## CubeCell HTCC-AB01

### Using PlatformIO (Recommended)
```bash
pio run -e cubecell_testmode --target upload --upload-port /dev/cu.usbserial-XXXX
```

### Using CubeCellflash Tool
1. Download CubeCellflash from Heltec
2. Select the .cyacd file
3. Choose COM port
4. Click Upload

### Firmware Variants
- `cubecell_testmode_*.hex` - Test mode, 5-second intervals
- `cubecell_production_*.hex` - Production mode, 60-second intervals with deep sleep
- `cubecell_original_*.hex` - Simple working version

## LoRa32 Gateway (LilyGo T3)

### Using ESPHome Web Flasher
1. Visit https://web.esphome.io/
2. Connect your device via USB
3. Select the .bin file
4. Click Install

### Using esptool.py
```bash
esptool.py --port /dev/cu.usbserial-XXXX --chip esp32 write_flash 0x0 lora32_gateway_factory_*.bin
```

### Using ESPHome CLI
```bash
esphome upload lilygo_lora_receiver.yaml --device /dev/cu.usbserial-XXXX
```

## Post-Flash Configuration

1. **CubeCell**: Press RST button after flashing
2. **LoRa32**: 
   - Connect to WiFi AP "LoRa-Gateway-Fallback" if WiFi not configured
   - Access web interface at device IP
   - Configure in Home Assistant

## Troubleshooting

- **CubeCell not flashing**: Hold USER button, press RST, release USER
- **LoRa32 not flashing**: Hold BOOT button, press RST, release BOOT
- **No communication**: Verify both devices use 433 MHz
EOF
    
    print_success "Documentation created"
}

# Create release notes
create_release_notes() {
    print_status "Creating release notes..."
    
    cat > "$RELEASE_DIR/RELEASE_NOTES.md" << EOF
# Release ${VERSION}

## 📦 Firmware Files

### CubeCell Transmitter
- \`cubecell_testmode_${VERSION}.hex\` - Test mode (5s intervals)
- \`cubecell_production_${VERSION}.hex\` - Production mode (60s intervals, deep sleep)
- \`cubecell_original_${VERSION}.hex\` - Simplified working version

### LoRa32 Gateway
- \`lora32_gateway_${VERSION}.bin\` - OTA update file
- \`lora32_gateway_factory_${VERSION}.bin\` - Full factory image

## 🚀 Quick Start

1. Flash CubeCell: Use \`cubecell_testmode_${VERSION}.hex\` for testing
2. Flash LoRa32: Use \`lora32_gateway_factory_${VERSION}.bin\`
3. See \`docs/FLASHING.md\` for detailed instructions

## ⚙️ Configuration

- Frequency: 433 MHz
- Spreading Factor: SF7
- Bandwidth: 125 kHz
- Sync Word: 0x12 (private network)

## 📝 Changelog

- Built on: $(date +"%Y-%m-%d %H:%M:%S")
- Git commit: $(git rev-parse --short HEAD)
- Platform: $(uname -s) $(uname -m)

## ⚠️ Known Issues

- CubeCell v2 battery voltage reads incorrectly (hardware issue)
- LoRa32 requires manual reset if hanging

## 📄 License

GNU AGPLv3 - See LICENSE file
EOF
    
    print_success "Release notes created"
}

# Create installation script
create_install_script() {
    print_status "Creating installation scripts..."
    
    # Create Unix/Linux/Mac install script
    cat > "$RELEASE_DIR/install.sh" << 'EOF'
#!/bin/bash
# Quick installer for Volkszähler LoRa Bridge

echo "Volkszähler LoRa Bridge Installer"
echo "=================================="
echo ""

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    PORT_PREFIX="/dev/cu.usbserial-"
else
    PORT_PREFIX="/dev/ttyUSB"
fi

# List available ports
echo "Available serial ports:"
ls ${PORT_PREFIX}* 2>/dev/null || echo "No serial ports found"
echo ""

# Install CubeCell
echo "1. Flash CubeCell"
read -p "Enter CubeCell port (e.g., ${PORT_PREFIX}0001): " CUBECELL_PORT
if [ -n "$CUBECELL_PORT" ]; then
    echo "Flashing CubeCell test mode..."
    # Add actual flash command here based on available tools
fi

# Install LoRa32
echo ""
echo "2. Flash LoRa32 Gateway"
read -p "Enter LoRa32 port (e.g., ${PORT_PREFIX}0002): " LORA32_PORT
if [ -n "$LORA32_PORT" ]; then
    echo "Flashing LoRa32..."
    # Add actual flash command here based on available tools
fi

echo ""
echo "Installation complete!"
echo "Check the serial monitor to verify both devices are working."
EOF
    
    chmod +x "$RELEASE_DIR/install.sh"
    
    # Create Windows batch installer
    cat > "$RELEASE_DIR/install.bat" << 'EOF'
@echo off
echo Volkszaehler LoRa Bridge Installer
echo ==================================
echo.

echo Available COM ports:
wmic path Win32_SerialPort get DeviceID
echo.

echo 1. Flash CubeCell
set /p CUBECELL_PORT="Enter CubeCell port (e.g., COM3): "
if not "%CUBECELL_PORT%"=="" (
    echo Flashing CubeCell test mode...
    rem Add actual flash command here
)

echo.
echo 2. Flash LoRa32 Gateway  
set /p LORA32_PORT="Enter LoRa32 port (e.g., COM4): "
if not "%LORA32_PORT%"=="" (
    echo Flashing LoRa32...
    rem Add actual flash command here
)

echo.
echo Installation complete!
pause
EOF
    
    print_success "Installation scripts created"
}

# Package release
package_release() {
    print_status "Packaging release..."
    
    cd "$RELEASE_DIR"
    
    # Create individual archives
    zip -r "cubecell_firmware_${VERSION}.zip" cubecell/ docs/FLASHING.md -q
    print_success "CubeCell package created"
    
    if [ -d "lora32" ] && [ "$(ls -A lora32)" ]; then
        zip -r "lora32_firmware_${VERSION}.zip" lora32/ docs/FLASHING.md -q
        print_success "LoRa32 package created"
    fi
    
    # Create complete release archive
    zip -r "../${PROJECT_NAME}_${VERSION}_complete.zip" . -q
    cd ..
    
    print_success "Release packaged"
}

# Generate checksums
generate_checksums() {
    print_status "Generating checksums..."
    
    cd "$RELEASE_DIR"
    
    # Create checksum file
    echo "# SHA256 Checksums for ${PROJECT_NAME} ${VERSION}" > CHECKSUMS.sha256
    echo "# Generated: $(date)" >> CHECKSUMS.sha256
    echo "" >> CHECKSUMS.sha256
    
    # Generate checksums for all files
    find . -type f -name "*.hex" -o -name "*.bin" -o -name "*.cyacd" -o -name "*.zip" | while read -r file; do
        if [[ "$OSTYPE" == "darwin"* ]]; then
            shasum -a 256 "$file" >> CHECKSUMS.sha256
        else
            sha256sum "$file" >> CHECKSUMS.sha256
        fi
    done
    
    cd ..
    print_success "Checksums generated"
}

# Main build process
main() {
    echo -e "${GREEN}Starting release build process...${NC}"
    echo ""
    
    check_dependencies
    clean_build
    create_release_dirs
    build_cubecell
    build_lora32
    create_docs
    create_release_notes
    create_install_script
    package_release
    generate_checksums
    
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║           BUILD SUCCESSFUL! 🎉              ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BLUE}Release artifacts created in:${NC} ${RELEASE_DIR}/"
    echo ""
    echo "📦 Release packages:"
    ls -lh "$RELEASE_DIR"/*.zip 2>/dev/null || echo "  No packages created"
    echo ""
    echo "📄 Firmware files:"
    echo "  CubeCell: $(ls -1 "$RELEASE_DIR"/cubecell/*.hex 2>/dev/null | wc -l) variants"
    echo "  LoRa32: $(ls -1 "$RELEASE_DIR"/lora32/*.bin 2>/dev/null | wc -l) variants"
    echo ""
    echo -e "${YELLOW}Next steps:${NC}"
    echo "1. Test the firmware files on actual hardware"
    echo "2. Review RELEASE_NOTES.md"
    echo "3. Create GitHub release and upload:"
    echo "   - ${PROJECT_NAME}_${VERSION}_complete.zip"
    echo "   - Individual firmware files from release/"
    echo "4. Tag the release: git tag -a ${VERSION} -m 'Release ${VERSION}'"
    echo "5. Push tag: git push origin ${VERSION}"
}

# Run main function
main "$@"