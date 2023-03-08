#!/usr/bin/bash
set -e

export PICO_SDK_PATH=~/pico-sdk

BUILD_DIR=build

if [ -d "${BUILD_DIR}" ]; then rm -rf ${BUILD_DIR}; fi
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ..
make -j$(nproc)
