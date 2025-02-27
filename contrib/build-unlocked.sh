#!/bin/bash

# Check if make is in the PATH
if ! command -v make &> /dev/null
then
    echo "make is not installed or not in the PATH. Please install make and try again."
    exit 1
fi

# Check if podman is in the PATH
if ! command -v podman &> /dev/null
then
    echo "podman is not installed or not in the PATH. Please install podman and try again."
    exit 1
fi

# Run the make command
make build-unlocked
exit 0
