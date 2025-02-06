/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "proto.h"
#include "../tk1_mem.h"
#include "assert.h"
#include "led.h"
#include "lib.h"
#include "state.h"
#include "types.h"

// USB Mode Protocol:
//   1 byte mode
//   1 byte length
//
// Our USB Mode Protocol packets has room for 255 bytes according to
// the header but we use a packet size of 62 so we limit transfers to
// 64 bytes (2 byte header + 62 byte data) to fit in a single USB
// frame.
#define USBMODE_PACKET_SIZE 64

// clang-format off
static volatile uint32_t *can_rx = (volatile uint32_t *)TK1_MMIO_UART_RX_STATUS;
static volatile uint32_t *rx =     (volatile uint32_t *)TK1_MMIO_UART_RX_DATA;
static volatile uint32_t *can_tx = (volatile uint32_t *)TK1_MMIO_UART_TX_STATUS;
static volatile uint32_t *tx =     (volatile uint32_t *)TK1_MMIO_UART_TX_DATA;
// clang-format on

static uint8_t genhdr(uint8_t id, uint8_t endpoint, uint8_t status,
		      enum cmdlen len);
static int parseframe(uint8_t b, struct frame_header *hdr);
static uint8_t readbyte(void);
static void write_with_header(const uint8_t *buf, size_t nbytes,
			      enum mode mode);
static void writebyte(uint8_t b);
static size_t bytelen(enum cmdlen cmdlen);

static uint8_t genhdr(uint8_t id, uint8_t endpoint, uint8_t status,
		      enum cmdlen len)
{
	return (id << 5) | (endpoint << 3) | (status << 2) | len;
}

int readcommand(struct frame_header *hdr, uint8_t *cmd, int state)
{
	uint8_t in = 0;

	set_led((state == FW_STATE_LOADING) ? LED_BLACK : LED_WHITE);

	if (read(&in, 1, 1, MODE_CDC) == -1) {
		return -1;
	}

	if (parseframe(in, hdr) == -1) {
		htif_puts("Couldn't parse header\n");
		return -1;
	}

	(void)memset(cmd, 0, CMDLEN_MAXBYTES);
	// Now we know the size of the cmd frame, read it all
	if (read(cmd, CMDLEN_MAXBYTES, hdr->len, MODE_CDC) != 0) {
		htif_puts("read: buffer overrun\n");
		return -1;
	}

	// Is it for us?
	if (hdr->endpoint != DST_FW) {
		htif_puts("Message not meant for us\n");
		return -1;
	}

	return 0;
}

static int parseframe(uint8_t b, struct frame_header *hdr)
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
	hdr->len = bytelen(b & 0x3);

	return 0;
}

#define FWFRAMESIZE (2 + 128)
// Send a firmware reply with a frame header, response code rspcode and
// following data in buf
void fwreply(struct frame_header hdr, enum fwcmd rspcode, uint8_t *buf)
{
	size_t nbytes = 0;
	enum cmdlen len = 0;	    // length covering (rspcode + length of buf)
	uint8_t frame[FWFRAMESIZE]; // Frame header + longest response

	switch (rspcode) {
	case FW_RSP_NAME_VERSION:
		len = LEN_32;
		break;

	case FW_RSP_LOAD_APP:
		len = LEN_4;
		break;

	case FW_RSP_LOAD_APP_DATA:
		len = LEN_4;
		break;

	case FW_RSP_LOAD_APP_DATA_READY:
		len = LEN_128;
		break;

	case FW_RSP_GET_UDI:
		len = LEN_32;
		break;

	default:
		htif_puts("fwreply(): Unknown response code: 0x");
		htif_puthex(rspcode);
		htif_lf();
		return;
	}

	nbytes = bytelen(len);

	// Frame Protocol Header
	frame[0] = genhdr(hdr.id, hdr.endpoint, 0x0, len);

	// FW protocol header
	frame[1] = rspcode;

	// Payload
	memcpy_s(&frame[2], FWFRAMESIZE, buf, nbytes);

	// 1 byte framing header + length + payload
	write(frame, 1 + nbytes, MODE_CDC);
}

static void writebyte(uint8_t b)
{
	for (;;) {
		if (*can_tx) {
			*tx = b;
			return;
		}
	}
}

static void write_with_header(const uint8_t *buf, size_t nbytes, enum mode mode)
{
	// Append USB Mode Protocol header:
	//   1 byte mode
	//   1 byte length

	writebyte(mode);
	writebyte(nbytes);

	for (int i = 0; i < nbytes; i++) {
		writebyte(buf[i]);
	}
}

// write blockingly writes nbytes bytes of data from buf to the UART,
// framing the data in USB Mode Protocol with mode mode, either
// MODE_CDC or MODE_HID.
void write(const uint8_t *buf, size_t nbytes, enum mode mode)
{
	while (nbytes > 0) {
		// We split the data into chunks that will fit in the
		// USB Mode Protocol with some spare change.
		uint8_t len =
		    nbytes < USBMODE_PACKET_SIZE ? nbytes : USBMODE_PACKET_SIZE;

		write_with_header((const uint8_t *)buf, len, mode);

		buf += len;
		nbytes -= len;
	}
}

static uint8_t readbyte(void)
{
	for (;;) {
		if (*can_rx) {
			uint32_t b = *rx;
			return b;
		}
	}
}

// read blockingly reads nbytes bytes of data into buffer buf, a
// maximum bufsize bytes.
//
// Caller asks for the expected USB mode expect_mode: MODE_CDC or
// MODE_HID, which represents different endpoints on the USB
// controller.
//
// If data is readable but with another mode set, it is silently
// discarded and we keep on reading until nbytes bytes have appeared.
//
int read(uint8_t *buf, size_t bufsize, size_t nbytes, enum mode expect_mode)
{
	static uint8_t mode = 0;
	static uint8_t mode_bytes_left = 0;

	if (nbytes > bufsize) {
		return -1;
	}

	int n = 0;
	while (n < nbytes) {
		if (mode_bytes_left == 0) {
			// Read USB Mode Protocol header:
			//   1 byte mode
			//   1 byte length
			mode = readbyte();
			mode_bytes_left = readbyte();
		}

		if (mode == expect_mode) {
			// Reading payload.
			buf[n] = readbyte();
			n++;
			mode_bytes_left--;
		} else {
			// Not the USB mode caller asked for. Eat the rest.
			for (int i = 0; i < mode_bytes_left; i++) {
				(void)readbyte();
			}

			mode_bytes_left = 0;
		}
	}

	return 0;
}

// bytelen returns the number of bytes a cmdlen takes
static size_t bytelen(enum cmdlen cmdlen)
{
	int len = 0;

	switch (cmdlen) {
	case LEN_1:
		len = 1;
		break;

	case LEN_4:
		len = 4;
		break;

	case LEN_32:
		len = 32;
		break;

	case LEN_128:
		len = 128;
		break;

	default:
		// Shouldn't happen
		assert(1 == 2);
	}

	return len;
}
