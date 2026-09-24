#!/bin/bash

################################################################################
# Script Name: calculate_stack_size.sh
# Description: Analyzes object files (.o) to calculate and report stack usage
# Usage: ./calculate_stack_size.sh <path_to_object_file.o>
################################################################################

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print usage
usage() {
    echo "Usage: $0 <path_to_object_file.o> [path_to_stack_usage.su]"
    echo ""
    echo "Description:"
    echo "  Analyzes an object file to calculate stack usage information"
    echo ""
    echo "Arguments:"
    echo "  path_to_object_file.o     Path to the object file (required)"
    echo "  path_to_stack_usage.su    Path to the .su file (optional)"
    echo ""
    echo "Examples:"
    echo "  $0 myfile.o"
    echo "  $0 /path/to/build/object.o"
    echo "  $0 /path/to/build/object.o /path/to/build/object.su"
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
SU_FILE_ARG="$2"

# Validate that file exists
if [ ! -f "$OBJ_FILE" ]; then
    error_exit "File '$OBJ_FILE' not found"
fi

# Validate that it's an object file
if [[ ! "$OBJ_FILE" =~ \.o$ ]]; then
    echo -e "${YELLOW}Warning: File doesn't have .o extension${NC}"
fi

# Check if required tools are available
command -v nm >/dev/null 2>&1 || error_exit "Command 'nm' not found. Please install binutils."
command -v objdump >/dev/null 2>&1 || error_exit "Command 'objdump' not found. Please install binutils."
command -v readelf >/dev/null 2>&1 || error_exit "Command 'readelf' not found. Please install binutils."

print_header "STACK USAGE ANALYSIS: $OBJ_FILE"

# Check for .su (stack usage) file
print_header "Stack Usage by Function"

# Use provided .su file path or auto-detect
if [ -n "$SU_FILE_ARG" ]; then
    SU_FILE="$SU_FILE_ARG"
else
    SU_FILE="${OBJ_FILE%.o}.su"
fi

if [ -f "$SU_FILE" ]; then
    echo -e "${GREEN}Stack usage data from: $SU_FILE${NC}"
    echo ""
    cat "$SU_FILE"

    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}SUMMARY${NC}"
    echo -e "${BLUE}========================================${NC}"

    MAX_STACK=$(awk '{print $2}' "$SU_FILE" | sort -n | tail -1)
    TOTAL_STACK=$(awk '{sum += $2} END {print sum}' "$SU_FILE")
    FUNC_COUNT=$(wc -l < "$SU_FILE")

    echo -e "${GREEN}Total functions: $FUNC_COUNT${NC}"
    echo -e "${GREEN}Maximum stack per function: $MAX_STACK bytes${NC}"
    echo -e "${GREEN}Total stack (sum of all): $TOTAL_STACK bytes${NC}"

    echo ""
    echo "Top 10 functions by stack usage:"
    echo "--------------------------------"
    awk '{print $2, $0}' "$SU_FILE" | sort -rn | head -10 | awk '{$1=""; print $0}'
else
    echo -e "${RED}Error: No .su file found at: $SU_FILE${NC}"
    echo ""
    echo "To generate .su files, compile with:"
    echo "  gcc -fstack-usage -c source.c"
    echo "  g++ -fstack-usage -c source.cpp"
    echo ""
    echo "Or enable in CMakeLists.txt:"
    echo "  add_compile_options(-fstack-usage)"
    exit 1
fi

exit 0
