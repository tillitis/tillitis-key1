/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "proto.h"
#include "../tk1_mem.h"
#include "lib.h"
#include "types.h"

// clang-format off
static volatile uint32_t *can_rx = (volatile uint32_t *)TK1_MMIO_UART_RX_STATUS;
static volatile uint32_t *rx =     (volatile uint32_t *)TK1_MMIO_UART_RX_DATA;
static volatile uint32_t *can_tx = (volatile uint32_t *)TK1_MMIO_UART_TX_STATUS;
static volatile uint32_t *tx =     (volatile uint32_t *)TK1_MMIO_UART_TX_DATA;
static volatile uint32_t *led =    (volatile uint32_t *)TK1_MMIO_TK1_LED;
// clang-format on

uint8_t genhdr(uint8_t id, uint8_t endpoint, uint8_t status, enum cmdlen len)
{
	return (id << 5) | (endpoint << 3) | (status << 2) | len;
}

int parseframe(uint8_t b, struct frame_header *hdr)
{
	if ((b & 0x80) != 0) {
		// Bad version
		return -1;
	}

	if ((b & 0x4) != 0) {
		// Must be 0
		return -1;
	}

	hdr->id = (b & 0x60) >> 5;
	hdr->endpoint = (b & 0x18) >> 3;

	// Length
	switch (b & 0x3) {
	case LEN_1:
		hdr->len = 1;
		break;
	case LEN_4:
		hdr->len = 4;
		break;
	case LEN_32:
		hdr->len = 32;
		break;
	case LEN_128:
		hdr->len = 128;
		break;
	default:
		// Unknown length
		return -1;
	}

	return 0;
}

// Send a firmware reply with a frame header, response code rspcode and
// following data in buf
void fwreply(struct frame_header hdr, enum fwcmd rspcode, uint8_t *buf)
{
	size_t nbytes;
	enum cmdlen len; // length covering (rspcode + length of buf)

	switch (rspcode) {
	case FW_RSP_NAME_VERSION:
		len = LEN_32;
		nbytes = 32;
		break;

	case FW_RSP_LOAD_APP:
		len = LEN_4;
		nbytes = 4;
		break;

	case FW_RSP_LOAD_APP_DATA:
		len = LEN_4;
		nbytes = 4;
		break;

	case FW_RSP_LOAD_APP_DATA_READY:
		len = LEN_128;
		nbytes = 128;
		break;

	case FW_RSP_GET_UDI:
		len = LEN_32;
		nbytes = 32;
		break;

	default:
		htif_puts("fwreply(): Unknown response code: 0x");
		htif_puthex(rspcode);
		htif_lf();
		return;
	}

	// Frame Protocol Header
	writebyte(genhdr(hdr.id, hdr.endpoint, 0x0, len));

	// FW protocol header
	writebyte(rspcode);
	nbytes--;

	write(buf, nbytes);
}

void writebyte(uint8_t b)
{
	for (;;) {
		if (*can_tx) {
			*tx = b;
			return;
		}
	}
}

void write(uint8_t *buf, size_t nbytes)
{
	for (int i = 0; i < nbytes; i++) {
		writebyte(buf[i]);
	}
}

uint8_t readbyte()
{
	for (;;) {
		if (*can_rx) {
			return *rx;
		}
	}
}

uint8_t readbyte_ledflash(int ledvalue, int loopcount)
{
	int led_on = 0;
	for (;;) {
		*led = led_on ? ledvalue : 0;
		for (int i = 0; i < loopcount; i++) {
			if (*can_rx) {
				return *rx;
			}
		}
		led_on = !led_on;
	}
}

void read(uint8_t *buf, size_t nbytes)
{
	for (int n = 0; n < nbytes; n++) {
		buf[n] = readbyte();
	}
}
