#!/bin/bash

################################################################################
# Script Name: calculate_code_size.sh
# Description: Analyzes object files (.o) to calculate and report code size
# Usage: ./calculate_code_size.sh <path_to_object_file.o>
################################################################################

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print usage
usage() {
    echo "Usage: $0 <path_to_object_file.o>"
    echo ""
    echo "Description:"
    echo "  Analyzes an object file to calculate code size information"
    echo ""
    echo "Arguments:"
    echo "  path_to_object_file.o     Path to the object file (required)"
    echo ""
    echo "Examples:"
    echo "  $0 myfile.o"
    echo "  $0 /path/to/build/object.o"
    echo "  $0 build/CMakeFiles/utests.dir/src/cert_elf_loader.cc.o"
    exit 1
}

# Function to print error and exit
error_exit() {
    echo -e "${RED}Error: $1${NC}" >&2
    exit 1
}

# Function to print section header
print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Check if argument is provided
if [ $# -eq 0 ]; then
    echo -e "${RED}Error: No object file specified${NC}"
    usage
fi

OBJ_FILE="$1"

# Validate that file exists
if [ ! -f "$OBJ_FILE" ]; then
    error_exit "File '$OBJ_FILE' not found"
fi

# Validate that it's an object file
if [[ ! "$OBJ_FILE" =~ \.o$ ]]; then
    echo -e "${YELLOW}Warning: File doesn't have .o extension${NC}"
fi

# Check if required tools are available
command -v size >/dev/null 2>&1 || error_exit "Command 'size' not found. Please install binutils."
command -v nm >/dev/null 2>&1 || error_exit "Command 'nm' not found. Please install binutils."

print_header "CODE SIZE ANALYSIS: $OBJ_FILE"

# Extract overall size
echo -e "${BLUE}Overall Size:${NC}"
size "$OBJ_FILE"
echo ""

# Get size breakdown
SIZE_OUTPUT=$(size "$OBJ_FILE" | tail -1)
TEXT_SIZE=$(echo "$SIZE_OUTPUT" | awk '{print $1}')
DATA_SIZE=$(echo "$SIZE_OUTPUT" | awk '{print $2}')
BSS_SIZE=$(echo "$SIZE_OUTPUT" | awk '{print $3}')
TOTAL_SIZE=$(echo "$SIZE_OUTPUT" | awk '{print $4}')

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}SIZE SUMMARY${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Text (code):        $TEXT_SIZE bytes${NC}"
echo -e "${GREEN}Data (initialized): $DATA_SIZE bytes${NC}"
echo -e "${GREEN}BSS (uninitialized): $BSS_SIZE bytes${NC}"
echo -e "${GREEN}Total:              $TOTAL_SIZE bytes${NC}"
echo ""

# Extract per-function code size
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}PER-FUNCTION CODE SIZE (All Functions)${NC}"
echo -e "${BLUE}========================================${NC}"
echo "FUNCTION                                                      CODE SIZE"
echo "----------------------------------------------------------------------------"

nm --print-size --size-sort "$OBJ_FILE" | \
    grep ' [TtWw] ' | \
    sort -k2 -n -r | \
    while read addr size_hex type mangled; do
        # Convert hex size to decimal
        size_dec=$((16#$size_hex))
        # Try to demangle C++ names
        if command -v c++filt >/dev/null 2>&1; then
            demangled=$(echo "$mangled" | c++filt)
        else
            demangled="$mangled"
        fi
        printf "%-60s %6d bytes\n" "$demangled" "$size_dec"
    done

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Analysis complete!${NC}"
echo -e "${BLUE}========================================${NC}"

exit 0
