/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hardware/gpio.h>
#include <hardware/adc.h>
#include "pico/bootrom.h"
#include "bsp/board.h"
#include "tusb.h"
#include "pio_spi.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,
};


static bool do_reset = false;

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

#define BULK_TRANSFER_MAX_SIZE 2048 // Maximum control transfer data length
#define SPI_MAX_TRANSFER_SIZE (BULK_TRANSFER_MAX_SIZE-8)


pio_spi_inst_t spi = {
    .pio = pio0,
    .sm = 0,
    .cs_pin = 0
};

uint pio_offset;

void led_blinking_task(void);

void pins_init();

/*------------- MAIN -------------*/
int main(void)
{
    board_init();
    tusb_init();

    pio_offset = pio_add_program(spi.pio, &spi_cpha0_program);

    pins_init();

    adc_init();

    while (1)
    {
        tud_task(); // tinyusb device task
        led_blinking_task();

        if(do_reset) {
            reset_usb_boot(0,0);
        }
    }

    return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

#define PIN_COUNT 32 // PIN_MASK is 32 bits, so that's the max number of pins available
#define PIN_MASK (0b00011100011111111111111111111111)
// #define PIN_MASK ((1<<PIN_COUNT) - 1)
// #define PIN_MASK (0xFFFFFFFF)

void pins_init()
{
    gpio_init_mask(PIN_MASK);
}

//! @brief Set the pull-up state for the gpios specified in the pinmask
void set_pulls_masked(
    const uint32_t mask,
    const uint32_t ups,
    const uint32_t downs)
{
    for (int gpio = 0; gpio < PIN_COUNT; gpio++)
    {
        if (!(PIN_MASK & (1 << gpio)))
            continue;

        if (mask & (1 << gpio))
        {
            gpio_set_pulls(gpio, ups & (1 << gpio), downs & (1 << gpio));
        }
    }
}

uint32_t get_directions_all() {
    uint32_t pin_directions = 0;

    for (int gpio = 0; gpio < PIN_COUNT; gpio++)
    {
        pin_directions |= (gpio_get_dir(gpio)) << gpio;
    }

    return pin_directions;
}

void spi_xfer(bool handle_cs,
                 const uint32_t byte_count,
                 uint8_t const *buf_out,
                 uint8_t *buf_in)
{
    if(byte_count > SPI_MAX_TRANSFER_SIZE)
        return;

    if((buf_in == NULL) && (buf_out == NULL))
        return;

    if(buf_in != NULL)
        memset(buf_in, 0, SPI_MAX_TRANSFER_SIZE);

    if(handle_cs)
        gpio_put(spi.cs_pin, false);

    if(buf_in == NULL) { // Write transaction
        pio_spi_write8_blocking(&spi, buf_out, byte_count);
    }
    else if(buf_out == NULL) { // Read transaction
        pio_spi_read8_blocking(&spi, buf_in, byte_count);
    }
    else { // Full-duplex transaction
        pio_spi_write8_read8_blocking(&spi, buf_out, buf_in, byte_count);
    }

    if(handle_cs)
        gpio_put(spi.cs_pin, true);
}

uint32_t adc_sample_input(uint8_t input)
{
    if (input > 3)
    {
        return 0;
    }

    // Configure the GPIO
    adc_gpio_init(input + 26);

    adc_select_input(input);

    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const uint32_t conversion_factor = 3300000 / (1 << 12); // microvolts per sample
    uint16_t result = adc_read();

    return result * conversion_factor;
}

uint32_t read_uint32(uint8_t const *buffer)
{
    const uint32_t val =
        (*(buffer + 0) << 24) + (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + (*(buffer + 3) << 0);
    return val;
}

void write_uint32(uint32_t val, uint8_t *buffer)
{
    buffer[0] = ((val >> 24) & 0xFF);
    buffer[1] = ((val >> 16) & 0xFF);
    buffer[2] = ((val >> 8) & 0xFF);
    buffer[3] = ((val >> 0) & 0xFF);
}


typedef enum
{
    COMMAND_PIN_DIRECTION = 0x30,           // Configurure GPIO pin directions
    COMMAND_PULLUPS = 0x31,                 // Configure GPIO pullups
    COMMAND_PIN_VALUES = 0x32,              // Set GPIO output values
    COMMAND_SPI_CONFIGURE = 0x40,           // Configure SPI pins
    COMMAND_SPI_XFER = 0x41,                // SPI transaction with CS pin
    COMMAND_SPI_CLKOUT = 0x42,              // Just toggle the clock
    COMMAND_ADC_READ = 0x50,                // Read ADC inputs
    COMMAND_BOOTLOADER = 0xE0,              // Jump to bootloader mode

    COMMAND_MS_OS_DESCRIPTOR = 0xF8
} command_t;

static const uint8_t microsoft_os_compatible_id_desc[] = {
	40, 0, 0, 0, // total length, 16 header + 24 function * 1
	0, 1, // Version 1
	4, 0, // Extended compatibility ID descriptor index
	1, // Number of function sections
	0, 0, 0, 0, 0, 0, 0, // Reserved
	0, // Interface number
	1, // Reserved
	'W','I','N','U','S','B',0,0, // compatibleID
	0,0,0,0,0,0,0,0,             // subCompatibleID
	0,0,0,0,0,0 // Reserved
};

uint8_t out_buffer[BULK_TRANSFER_MAX_SIZE]; // Host->Device data
uint8_t in_buffer[BULK_TRANSFER_MAX_SIZE];  // Holds Device->Host data

uint8_t spi_in_buffer[SPI_MAX_TRANSFER_SIZE];   // Holds read values from last SPI transfer

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // Only handle vendor requests
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR)
        return false;

    if ((stage == CONTROL_STAGE_SETUP) && (request->bmRequestType_bit.direction == 1))
    { // Device to host (IN)
        memset(in_buffer, 0, sizeof(in_buffer));

        switch (request->bRequest)
        {
	    case COMMAND_MS_OS_DESCRIPTOR:
		// TODO: Check index
            memcpy(in_buffer,
			    microsoft_os_compatible_id_desc,
			    sizeof(microsoft_os_compatible_id_desc)
			    );
            return tud_control_xfer(rhport,
			    request,
			    (void *)(uintptr_t)in_buffer,
			    sizeof(microsoft_os_compatible_id_desc)
			    );
            break;

        case COMMAND_PIN_VALUES:
            write_uint32(gpio_get_all() & PIN_MASK, &in_buffer[0]);
            return tud_control_xfer(rhport, request, (void *)(uintptr_t)in_buffer, 4);
            break;

        case COMMAND_ADC_READ:
            write_uint32(adc_sample_input(0), &in_buffer[0]);
            write_uint32(adc_sample_input(1), &in_buffer[4]);
            write_uint32(adc_sample_input(2), &in_buffer[8]);
            return tud_control_xfer(rhport, request, (void *)(uintptr_t)in_buffer, (3*4));
            break;

        case COMMAND_SPI_XFER:
            return tud_control_xfer(rhport, request, (void *)(uintptr_t)spi_in_buffer, sizeof(spi_in_buffer));
            break;

        default:
            break;
        }
    }
    else if ((stage == CONTROL_STAGE_SETUP) && (request->bmRequestType_bit.direction == 0))
    { // Host to device (OUT): Set up request to handle DATA stage
        switch (request->bRequest)
        {
        case COMMAND_PIN_DIRECTION:
        case COMMAND_PULLUPS:
        case COMMAND_PIN_VALUES:
        case COMMAND_SPI_CONFIGURE:
        case COMMAND_SPI_XFER:
        case COMMAND_SPI_CLKOUT:
            return tud_control_xfer(rhport, request, (void *)(uintptr_t)out_buffer, sizeof(out_buffer));
            break;

	case COMMAND_BOOTLOADER:
            do_reset = true;
            return true;
            break;

        default:
            break;
        }
    }
    else if ((stage == CONTROL_STAGE_ACK) && (request->bmRequestType_bit.direction == 0))
    { // Host to device (OUT): Handle data
        switch (request->bRequest)
        {
        case COMMAND_PIN_DIRECTION:
            gpio_set_dir_masked(
                read_uint32(&out_buffer[0]) & PIN_MASK,
                read_uint32(&out_buffer[4]) & PIN_MASK);
            return true;
            break;

        case COMMAND_PULLUPS: // set pullups/pulldowns
            set_pulls_masked(
                read_uint32(&out_buffer[0]) & PIN_MASK,
                read_uint32(&out_buffer[4]) & PIN_MASK,
                read_uint32(&out_buffer[8]) & PIN_MASK);
            return true;
            break;
            
        case COMMAND_PIN_VALUES: // set pin values
            gpio_put_masked(
                read_uint32(&out_buffer[0]) & PIN_MASK,
                read_uint32(&out_buffer[4]) & PIN_MASK);
            return true;
            break;

        case COMMAND_SPI_CONFIGURE:
            if(out_buffer[4] == 0)
                return false;

            pio_spi_init(spi.pio, spi.sm, pio_offset,
                         8,       // 8 bits per SPI frame
                         (31.25f/out_buffer[4]),  // 1 MHz @ 125 clk_sys
                         false,   // CPHA = 0
                         false,   // CPOL = 0
                         out_buffer[0], //sck
                         out_buffer[2], // mosi
                         out_buffer[3]  // miso
            );
            spi.cs_pin = out_buffer[1]; // cs

            return true;
            break;

        case COMMAND_SPI_XFER:
            spi_xfer(
                out_buffer[0],  // CS
                read_uint32(&out_buffer[1]),    // byte length
                &out_buffer[5], // data
                spi_in_buffer
                );
            
            return true;
            break;

        case COMMAND_SPI_CLKOUT:
            spi_xfer(
                false,
                read_uint32(&out_buffer[0]),
                NULL,              
                spi_in_buffer
                );
            
            return true;
            break;



        default:
            break;
        }
    }
    else {
        return true;
    }

    // stall unknown request
    return false;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
    static uint32_t start_ms = 0;
    static bool led_state = false;

    // Blink every interval ms
    if (board_millis() - start_ms < blink_interval_ms)
        return; // not enough time
    start_ms += blink_interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}
