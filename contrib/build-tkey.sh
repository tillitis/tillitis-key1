#!/bin/bash

# Function to return status of previous command.
check_status() {
    if [ $? -eq 0 ]; then
        echo " done."
    else
        echo " failed."
        exit 1
    fi
}

echo "This script will assist in building a bitstream for the TKey Unlocked."
echo "It will automatically check for:"
echo " - Installed prerequisites"
echo " - Download the tillitis-key1 repository"
echo " - Download and start the tkey-builder OCI image"
echo " - Help generate the secrets needed"

read -p "Do you want to continue? [y/n] " user_input

if [[ ! "$user_input" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo "Exiting."
    exit 1
fi

echo -n "Checking prerequisites..."

# Check if podman is in the PATH
if ! command -v podman &> /dev/null
then
    echo "podman is not installed or not in the PATH. Please install podman and try again."
    exit 1
fi

use_wget=0
# Check if podman is in the PATH
if ! command -v curl &> /dev/null
then
    if ! command -v wget &> /dev/null
    then
        echo "curl or wget is not installed or not in the PATH. Please install either curl or wget and try again."
        exit 1
    fi
    use_wget=1
fi

echo " done."

REPO=tillitis-key1
TAG="TK1-24.03"
URL="https://github.com/tillitis/$REPO/archive/refs/tags/$TAG.zip"
OUTPUT="$REPO-$TAG.zip"

if [ -f "$OUTPUT" ]; then
    echo "$OUTPUT exists, don't know what to do.. Remove to continue. Exiting."
    exit 1
fi

if [ -d "$REPO-$TAG" ]; then
    echo "$REPO-$TAG exists, don't know what to do.. Remove to continue. Exiting."
    exit 1
fi

echo -n "Downloading $OUTPUT..."
# Fetch the archive using curl
if [ "$use_wget" -eq 1 ]; then
    wget -O "$OUTPUT" "$URL"
else
    curl --no-progress-meter -L -o "$OUTPUT" "$URL"
fi
check_status

echo -n "Extracting..."
unzip $REPO-$TAG.zip > /dev/null
check_status

VERBOSE=false
IMAGE_NAME=ghcr.io/tillitis/tkey-builder:4
CONTAINER_NAME="tkeybuilder-container"
HOST_PORT=8080
CONTAINER_PORT=80

# Function to direct output based on verbosity
run_command() {
    if $VERBOSE; then
        "$@"
    else
        "$@" > /dev/null 2>&1
    fi
}

# Pull the latest nginx image
echo -n "Pulling $IMAGE_NAME image..."
run_command podman pull $IMAGE_NAME
check_status

# Run the container
echo -n "Starting the container..."
# run_command podman run -d --name $CONTAINER_NAME -p $HOST_PORT:$CONTAINER_PORT $IMAGE_NAME /usr/bin/bash

run_command podman run -d --name $CONTAINER_NAME --mount type=bind,source="`pwd`/$REPO-$TAG/",target=/build -w /build -it $IMAGE_NAME
check_status

# Give the container a moment to initialize
sleep 2

PID=5
VID=1337
REV=1

echo "Generate secret.."
# Create secret
podman exec -it -w /build/hw/application_fpga/data $CONTAINER_NAME python3 ../tools/tpt/tpt.py --vid $VID --pid $PID --rev $REV 

# Build bitstream
echo -n "Build bitstream, this may take a few minutes... "
run_command podman exec -it -w /build/hw/application_fpga $CONTAINER_NAME make application_fpga.bin 
echo " done."

echo -n "Stopping container..."
# Stopping the container
run_command podman stop $CONTAINER_NAME
check_status

echo -n "Removing container..."
# Removing the container
run_command podman rm $CONTAINER_NAME
check_status

exit 0
