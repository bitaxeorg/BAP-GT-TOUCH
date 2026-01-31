#!/bin/bash

# ESP32-S3 Display Project Binary Merger
# Creates a single esp-display.bin file for flashing

# Binary file paths and addresses
BOOTLOADER_BIN="build/bootloader/bootloader.bin"
BOOTLOADER_ADDR=0x0
PARTITION_TABLE="build/partition_table/partition-table.bin"
PARTITION_TABLE_ADDR=0x8000
APP_BIN="build/lvgl_porting.bin"
APP_ADDR=0x10000

# Default output filename
DEFAULT_OUTPUT="esp-display.bin"

function show_help() {
    echo "ESP32-S3 Display Binary Merger"
    echo "Creates a combined binary for ESP32-S3 display project"
    echo ""
    echo "Usage: $0 [OPTIONS] [output_file]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -v, --verbose  Verbose output"
    echo ""
    echo "Arguments:"
    echo "  output_file    Output filename (default: $DEFAULT_OUTPUT)"
    echo ""
    echo "This script combines:"
    echo "  - Bootloader (0x0)"
    echo "  - Partition table (0x8000)" 
    echo "  - Application (0x10000)"
    echo ""
}

function print_error() {
    echo "ERROR: $1" >&2
}

function print_info() {
    if [ "$VERBOSE" -eq 1 ]; then
        echo "INFO: $1"
    fi
}

# Check if esptool.py is available
if ! command -v esptool.py &> /dev/null; then
    print_error "esptool.py is not installed or not in PATH"
    echo "Please install it with: pip install esptool"
    exit 1
fi

# Parse command line arguments
VERBOSE=0
OUTPUT_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -*)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
        *)
            if [ -z "$OUTPUT_FILE" ]; then
                OUTPUT_FILE="$1"
            else
                print_error "Multiple output files specified"
                exit 1
            fi
            shift
            ;;
    esac
done

# Set default output file if none specified
if [ -z "$OUTPUT_FILE" ]; then
    OUTPUT_FILE="$DEFAULT_OUTPUT"
fi

print_info "Output file: $OUTPUT_FILE"

# Check if all required files exist
REQUIRED_FILES=("$BOOTLOADER_BIN" "$PARTITION_TABLE" "$APP_BIN")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        print_error "Required file '$file' does not exist"
        echo "Please run 'idf.py build' first to generate all binaries"
        exit 1
    fi
    print_info "Found: $file"
done

# Create the merged binary
print_info "Creating merged binary..."

esptool.py --chip esp32s3 merge_bin \
    --flash_mode dio \
    --flash_size 16MB \
    --flash_freq 80m \
    $BOOTLOADER_ADDR "$BOOTLOADER_BIN" \
    $PARTITION_TABLE_ADDR "$PARTITION_TABLE" \
    $APP_ADDR "$APP_BIN" \
    -o "$OUTPUT_FILE"

# Check if the merge was successful
if [ $? -eq 0 ]; then
    echo "✅ Successfully created $OUTPUT_FILE"
    
    # Show file size
    if [ -f "$OUTPUT_FILE" ]; then
        SIZE=$(ls -lh "$OUTPUT_FILE" | awk '{print $5}')
        echo "📁 File size: $SIZE"
    fi
    
    echo ""
    echo "To flash the complete firmware:"
    echo "  esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x0 $OUTPUT_FILE"
    echo ""
else
    print_error "Failed to create $OUTPUT_FILE"
    exit 1
fi