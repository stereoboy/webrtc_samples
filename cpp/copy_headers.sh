#!/bin/bash


# set -x

SELECTED="api \
audio \
base \
call \
common_audio \
common_video \
experiments \
infra \
logging \
media \
modules \
net \
p2p \
pc \
resources \
rtc_base \
rtc_tools \
system_wrappers \
test \
third_party/abseil-cpp \
video \
sdk"


# Check if two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <selected_directories> <target_directory>"
    exit 1
fi

# Assign source and target directories from arguments
SRC_BASE_DIR=$1
DEST_BASE_DIR=$2

# Convert the string to an array
IFS=' ' read -r -a DIR_ARRAY <<< "$SELECTED"

# Check if source directory exists
if [ ! -d "$SRC_BASE_DIR" ]; then
    echo "Source directory $SRC_BASE_DIR does not exist."
    exit 1
fi

# Create the target directory if it doesn't exist
mkdir -p "$DEST_BASE_DIR"

# Iterate over the array
for DIR in "${DIR_ARRAY[@]}"; do
    echo "Directory: $SRC_BASE_DIR/$DIR"
    SRC_DIR="$SRC_BASE_DIR/$DIR"
    # Use find to locate all .h and .hpp files and copy them preserving the directory structure
    find "$SRC_DIR" -type f \( -name "*.h" -o -name "*.hpp" \) | while read -r file; do
        # Get the directory structure relative to the source directory
        RELATIVE_DIR=$(dirname "${file#$SRC_BASE_DIR/}")

        # Create the corresponding directory in the destination
        mkdir -p "$DEST_BASE_DIR/$RELATIVE_DIR"

        # Copy the header file to the target directory while preserving the structure
        cp "$file" "$DEST_BASE_DIR/$RELATIVE_DIR/"
    done
done

echo "Header files copied successfully from the selected directories."
