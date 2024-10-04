#!/bin/bash

# Define the source file and output executable name
SOURCE_FILE="main.c"
OUTPUT_FILE="terminal_rpg"

# Check if the source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file $SOURCE_FILE not found!"
    exit 1
fi

# Compile the C code into an executable
gcc -o $OUTPUT_FILE $SOURCE_FILE

# Check if the compilation succeeded
if [ $? -eq 0 ]; then
    echo "Compilation successful! Executable created: $OUTPUT_FILE"
else
    echo "Compilation failed!"
    exit 1
fi
