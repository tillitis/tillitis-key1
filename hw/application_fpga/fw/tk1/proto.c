// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "proto.h"
#include "state.h"

static uint8_t genhdr(uint8_t id, uint8_t endpoint, uint8_t status,
		      enum cmdlen len);
static int parseframe(uint8_t b, struct frame_header *hdr);
static size_t bytelen(enum cmdlen cmdlen);

static uint8_t genhdr(uint8_t id, uint8_t endpoint, uint8_t status,
		      enum cmdlen len)
{
	return (id << 5) | (endpoint << 3) | (status << 2) | len;
}

int readcommand(struct frame_header *hdr, uint8_t *cmd, int state)
{
	uint8_t in = 0;
	uint8_t available = 0;
	enum ioend endpoint = IO_NONE;

	led_set((state == FW_STATE_LOADING) ? LED_BLACK : LED_WHITE);

	debug_puts("readcommand\n");

	if (readselect(IO_CDC, &endpoint, &available) < 0) {
		return -1;
	}

	if (read(IO_CDC, &in, 1, 1) == -1) {
		return -1;
	}

	debug_puts("read 1 byte\n");

	if (parseframe(in, hdr) == -1) {
		debug_puts("Couldn't parse header\n");
		return -1;
	}

	debug_puts("parseframe succeeded\n");

	(void)memset(cmd, 0, CMDSIZE);

	// Now we know the size of the cmd frame, read it all
	uint8_t n = 0;
	while (n < hdr->len) {
		// Wait for something to be available
		if (readselect(IO_CDC, &endpoint, &available) < 0) {
			return -1;
		}

		// Read as much as is available of what we expect
		available = available > hdr->len ? hdr->len : available;

		assert(n < CMDSIZE);
		int n_bytes_read =
		    read(IO_CDC, &cmd[n], CMDSIZE - n, available);
		if (n_bytes_read < 0) {
			return -1;
		}

		n += n_bytes_read;
	}

	// Is it for us?
	if (hdr->endpoint != DST_FW) {
		debug_puts("Message not meant for us\n");
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

// Send a firmware reply with a frame header, response code rspcode and
// following data in buf
void fwreply(struct frame_header hdr, enum fwcmd rspcode, uint8_t *buf)
{
	size_t nbytes = 0;
	enum cmdlen len = 0;	// length covering (rspcode + length of buf)
	uint8_t frame[1 + 128]; // Frame header + longest response

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
		debug_puts("fwreply(): Unknown response code: 0x");
		debug_puthex(rspcode);
		debug_lf();
		return;
	}

	nbytes = bytelen(len);

	// Frame Protocol Header
	frame[0] = genhdr(hdr.id, hdr.endpoint, 0x0, len);
	// App protocol header
	frame[1] = rspcode;

	// Payload
	memcpy(&frame[2], buf, nbytes - 1);

	write(IO_CDC, frame, 1 + nbytes);
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
