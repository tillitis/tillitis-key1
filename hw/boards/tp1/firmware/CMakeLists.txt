cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
#include(pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)


project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(main)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/src/spi.pio)

target_sources(main PUBLIC
        src/main.c
        src/usb_descriptors.c
        src/pio_spi.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(main PUBLIC
        src
        )

pico_add_extra_outputs(main)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(main PUBLIC
        pico_stdlib
        hardware_adc
        hardware_pio
        tinyusb_device
        tinyusb_board
        )

# add url via pico_set_program_url
pico_set_program_name(main "ice40 flasher")
pico_set_program_url(main "https://github.com/blinkinlabs/ice40_flasher")
