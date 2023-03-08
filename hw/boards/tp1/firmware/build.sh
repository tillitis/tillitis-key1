#!/usr/bin/bash
set -e

if [ -d "/usr/local/pico-sdk" ]; then
export PICO_SDK_PATH=/usr/local/pico-sdk
else
export PICO_SDK_PATH=~/pico-sdk
fi

BUILD_DIR=build

if [ -d "${BUILD_DIR}" ]; then rm -rf ${BUILD_DIR}; fi
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ..
make -j$(nproc)
