// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>
#include <tkey/io.h>
#include <tkey/proto.h>

// lentobytes returns the number of bytes a cmdlen takes. Returns non-zero on
// success.
static size_t lentobyte(enum frame_cmdlen cmdlen)
{
	size_t len = 0;

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
		return 0;
	}

	return len;
}

static int bytetolen(size_t nbytes, enum frame_cmdlen *len)
{

	switch (nbytes) {
	case 1:
		*len = LEN_1;
		break;

	case 4:
		*len = LEN_4;
		break;

	case 32:
		*len = LEN_32;
		break;

	case 128:
		*len = LEN_128;
		break;

	default:
		// Shouldn't happen
		return -1;
	}

	return 0;
}

// Takes id, frame_domain, status and nbytes to transmit and return it as a one
// byte header.
//
// nbytes must be either 1, 4, 32, or 128 bytes, according to the framing
// protocol.
//
// Returns negative on error.
int frame_gen_hdr(uint8_t id, uint8_t frame_domain, uint8_t status,
		  size_t nbytes, uint8_t *header)
{
	*header = (id << 5) & 0x60;
	*header |= (frame_domain << 3) & 0x18;
	*header |= (status << 2) & 0x4;
	enum frame_cmdlen len = 0;
	if (bytetolen(nbytes, &len)) {
		return -1;
	}
	*header |= len & 0x3;

	return 0;
}

// Takes a byte b to parse and puts the result into hdr.
// Returns negative on error.
int frame_parse_hdr(uint8_t b, struct frame_header *hdr)
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
	hdr->f_domain = (b & 0x18) >> 3;
	hdr->len = lentobyte(b & 0x3);

	return 0;
}

// frame_read reads an entire frame according to the "framing protocol".
// Handles both Castor and Bellatrix. Uses the CDC endpoint for Castor, direct
// UART access for Bellatrix.
//
// The framing header, with the frame length, will be returned in hdr and the
// data in buf.
//
// The allocated size of buf, bufsize, needs to be equal or greater than
// the largest expected frame (not counting the framing header). If a header is
// received indicating a larger frame than bufsize, an error will be returned
// (-2) and the frame will be discarded. It is recommended to have a bufsize of
// 128 bytes, the maximum frame size according to the framing protocol.
//
// Does not handle interleaved frames from different endpoints, and
// hence should only be used with IO_CDC set via config_endpoints() (default).
//
// Returns number of bytes read on success, negative on error.
int frame_read(uint8_t *buf, size_t bufsize, struct frame_header *hdr)
{
	uint8_t in = 0;

	if (serial_read(&in, 1, 1) != 1) {
		return -1;
	}

	if (frame_parse_hdr(in, hdr) == -1) {
		return -1;
	}

	if (bufsize < hdr->len) {
		if (serial_discard(hdr->len) < 0) {
			return -1;
		}
		return -2; // Discarded invalid frame
	}

	return serial_read(buf, bufsize, hdr->len);
}

// frame_write writes an entire frame according to the "framing protocol".
// Handles both Castor and Bellatrix. Uses the CDC endpoint for Castor, direct
// UART access for Bellatrix.
//
// The frame size, nbytes, needs to be one of the defined lengths for a frame.
// See cmdlen in proto.h.
//
// Returns zero on success, negative on error.
int frame_write(uint8_t status, uint8_t id, enum frame_domain f_domain,
		uint8_t *buf, size_t nbytes)
{
	if (buf == NULL) {
		return -1;
	}

	if (nbytes == 0 || nbytes > CMDLEN_MAXBYTES) {
		return -1;
	}

	// Frame Protocol Header
	uint8_t header = 0;
	if (frame_gen_hdr(id, f_domain, status, nbytes, &header)) {
		return -1;
	}

	serial_write(&header, sizeof(header));
	serial_write(buf, nbytes);
	return 0;
}
