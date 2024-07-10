// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "../tk1/assert.h"
#include "../tk1/led.h"
#include "../tk1/lib.h"
#include "../tk1/proto.h"
#include "../tk1_mem.h"
// #include <stdbool.h>
// #include <stdint.h>
// #include <tkey/touch.h>
#include "../tk1/types.h"

#include "flash.h"
#include "spi.h"

// clang-format off
static volatile uint32_t *trng_status     = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
static volatile uint32_t *trng_entropy    = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
// clang-format on

static uint8_t rnd_byte(void)
{
	while ((*trng_status & (1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
	}
	return (*trng_entropy & 0xff);
}

//------------------------------------------------------------------
// dump_memory
// dump the complete contents of the memory
//------------------------------------------------------------------
void spi_dump_memory(void)
{
	flash_release_powerdown();

	uint8_t rx_buf[4096] = {0x00};

	for (int block = 0; block < 0x02; block += 1) {
		uint32_t address = 0x00000000;
		address |= (block << ADDR_BYTE_3_BIT) & 0x00FF0000;
		for (int i = 0; i < 16; i++) {
			memset(rx_buf, 0x00, sizeof(rx_buf));
			flash_read_data(address, rx_buf, sizeof(rx_buf));
			write(rx_buf, sizeof(rx_buf));
			address += 4096;
		}
	}
}

bool comp(uint8_t *a, uint8_t b, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (a[i] != b) {
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------
// Test sequence.
// Green LED = OK, Red LED = NOK.
// It is fail fast, so any failed test gives an instant RED LED.
//
// * Check if the SPI Master exists by reading if it is ready.
// * Wait for touch to start test (blue flash).
// * Read IDs and print to serial port.
// * Erase area (address: 0x00080000).
// * Read area, compare to 0xFF.
// * Write known pattern of a random byte.
// * Read area, compare to pattern.
// * Write known pattern of a random byte to another page.
// * Read area, compare to pattern.
// * If successful, LED Green.
// * Wait for touch to dump beginning of flash (bitstream)

//* one can use for example vimdiff <(xxd output.bin) <(xxd
// application_fpga.bin)

//------------------------------------------------------------------
int main(void)
{
	// Check if the SPI master exists on the hardware.
	// Since this is directly in main, the SPI master should
	// be ready, otherwise we can assume it does not exists in
	// the FGPA.
	if (!spi_ready()) {
		assert(1 == 2);
	}

	// Wait for terminal program and a character to be typed
	readbyte();

	uint8_t read_buf[0xff] = {0x00};

	// touch_wait(LED_BLUE, 0); // start test
	flash_release_powerdown();

	// touch_wait(LED_BLUE, 0); // start test
	flash_release_powerdown();
	// Read out IDs
	flash_read_manufacturer_device_id(read_buf);
	write(read_buf, 2);

	memset(read_buf, 0x00, sizeof(read_buf));
	flash_read_unique_id(read_buf);
	write(read_buf, 8);

	memset(read_buf, 0x00, sizeof(read_buf));
	flash_read_jedec_id(read_buf);
	write(read_buf, 3);

	// Erase area
	uint32_t start_address = 0x00080000;

	set_led(LED_RED);
	flash_sector_erase(start_address); // block 8
	set_led(LED_BLUE);

	// Read area, compare to 0xFF
	memset(read_buf, 0x00, sizeof(read_buf));
	flash_read_data(start_address, read_buf, 0xff);
	write(read_buf, sizeof(read_buf));

	if (!comp(read_buf, 0xff, sizeof(read_buf))) {
		set_led(LED_RED);
		assert(1 == 2);
	}

	// Write a known pattern of a random byte
	uint8_t write_buf[255] = {0x00};
	uint8_t byte = rnd_byte();
	memset(write_buf, byte, sizeof(write_buf));

	flash_write_data(start_address, write_buf, sizeof(write_buf));

	// Read area, compare to pattern
	memset(read_buf, 0x00, sizeof(read_buf));
	flash_read_data(start_address, read_buf, 0xff);
	write(read_buf, sizeof(read_buf));

	if (!comp(read_buf, byte, sizeof(read_buf))) {
		set_led(LED_RED);
		assert(1 == 2);
	}

	// Write a known pattern of a random byte
	start_address += 0x100; // next page

	byte = rnd_byte();
	memset(write_buf, byte, sizeof(write_buf));
	flash_write_data(start_address, write_buf, sizeof(write_buf));

	// Read area, compare to pattern
	memset(read_buf, 0x00, sizeof(read_buf));
	flash_read_data(start_address, read_buf, 0xff);
	write(read_buf, sizeof(read_buf));

	if (!comp(read_buf, byte, sizeof(read_buf))) {
		set_led(LED_RED);
		assert(1 == 2);
	}

	// It this is reached, the read/writing is successful.
	// touch_wait(LED_GREEN, 0); // start dump
	set_led(LED_BLACK);
	spi_dump_memory();

	for (;;) {
		set_led(LED_GREEN);
	}
}
