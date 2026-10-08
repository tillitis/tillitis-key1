// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include "tkey/platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/proto.h>
#include <tkey/tk1_mem.h>

// Maximum payload size sent over the USB Mode Protocol.
//
// USB Mode Protocol:
//   1 byte mode
//   1 byte length
//
// Our USB Mode Protocol packets has room for 255 bytes according to
// the header but we send at most 64 bytes of payload + the 2 byte
// header. The header is removed in the USB controller and the maximum
// payload fits in a single USB frame on the other side.
#define USBMODE_PACKET_SIZE 64

static void hex(uint8_t buf[2], const uint8_t c);
static uint8_t readbyte(void);
static void writebyte(uint8_t b);

struct usb_mode {
	enum ioend endpoint; // Current USB endpoint with data
	uint8_t len;	     // Data available in from current USB mode.
};

static struct usb_mode cur_endpoint = {
    IO_NONE,
    0,
};

// clang-format off
static volatile uint32_t* const can_rx  = (volatile uint32_t *)TK1_MMIO_UART_RX_STATUS;
static volatile uint32_t* const rx      = (volatile uint32_t *)TK1_MMIO_UART_RX_DATA;
static volatile uint32_t* const can_tx  = (volatile uint32_t *)TK1_MMIO_UART_TX_STATUS;
static volatile uint32_t* const tx      = (volatile uint32_t *)TK1_MMIO_UART_TX_DATA;
static volatile uint8_t*  const debugtx = (volatile uint8_t *)TK1_MMIO_QEMU_DEBUG;
static volatile uint32_t* const ver	= (volatile uint32_t *) TK1_MMIO_TK1_VERSION;
// clang-format on

// writebyte blockingly writes byte b to UART
static void writebyte(uint8_t b)
{
	for (;;) {
		if (*can_tx) {
			*tx = b;
			return;
		}
	}
}

// write_with_header writes nbytes of buf to UART with a USB Mode
// Protocol header telling the receiver about the mode and length.
static void write_with_header(enum ioend dest, const uint8_t *buf,
			      size_t nbytes)
{
	// USB Mode Protocol header:
	//   1 byte mode
	//   1 byte length

	writebyte(dest);
	writebyte(nbytes);

	for (int i = 0; i < nbytes; i++) {
		writebyte(buf[i]);
	}
}

// write blockingly writes nbytes bytes of data from buf to dest which
// is either:
//
// - IO_UART: Low-level UART access, no USB Mode Header added.
//
// - IO_QEMU: QEMU debug port
//
// - IO_CH552: Internal communication between the FPGA and the
//   CH552, with header.
//
// - IO_CDC: Through the UART for the CDC endpoint, with header.
//
// - IO_FIDO: Through the UART for the FIDO endpoint, with header.
//
// - IO_CCID: Through the UART for the CCID endpoint, with header.
//
// - IO_DEBUG: Through the UART for the DEBUG endpoint (USB HID), with
//   header.
void write(enum ioend dest, const uint8_t *buf, size_t nbytes)
{
	if (dest == IO_QEMU) {
		for (int i = 0; i < nbytes; i++) {
			*debugtx = buf[i];
		}

		return;
	} else if (dest == IO_UART) {
		for (int i = 0; i < nbytes; i++) {
			writebyte(buf[i]);
		}

		return;
	}

	while (nbytes > 0) {
		// We split the data into chunks that will fit in the
		// USB Mode Protocol and fits neatly in the USB frames
		// on the other side of the USB controller.
		uint8_t len =
		    nbytes < USBMODE_PACKET_SIZE ? nbytes : USBMODE_PACKET_SIZE;

		write_with_header(dest, (const uint8_t *)buf, len);

		buf += len;
		nbytes -= len;
	}
}

// readbyte reads a byte from UART and returns it. Blocking.
static uint8_t readbyte(void)
{
	for (;;) {
		if (*can_rx) {
			return *rx;
		}
	}

	return 0;
}

// read reads into buf of size bufsize from UART, nbytes or less, from
// USB endpoint src.
//
// If used with the USB-mode protocol and readselect(), it doesn't block.
// Only for version > Bellatrix.
//
// If called with IO_UART it will do low-level UART access, blockingly.
//
// Returns the number of bytes read. Empty data returns 0.
int read(enum ioend src, uint8_t *buf, size_t bufsize, size_t nbytes)
{
	if (buf == NULL || nbytes > bufsize) {
		return -1;
	}

	if (src == IO_NONE || src == IO_QEMU) {
		// Destination only endpoints
		return -1;
	}
	int n = 0;
	if (src == IO_UART) {
		for (n = 0; n < nbytes; n++) {
			buf[n] = readbyte();
		}
		return n;
	}

	if (src != cur_endpoint.endpoint) {
		// No data for this source available right now.
		return 0;
	}

	for (n = 0; n < nbytes && cur_endpoint.len > 0; n++) {
		buf[n] = readbyte();
		cur_endpoint.len--;
	}

	return n;
}

// serial_write writes nbytes of data from buf to IO_CDC when using Castor, or
// to IO_UART when using Bellatrix.
int serial_write(const uint8_t *buf, size_t nbytes)
{
	if (*ver < TKEY_VERSION_CASTOR) {
		write(IO_UART, buf, nbytes);
		return 0;
	}

	write(IO_CDC, buf, nbytes);

	return 0;
}

// serial_read blocks and returns when nbytes is read.
// Returns number of bytes read on success, negative on error.
//
// On Castor serial_read will use the IO_CDC endpoint, on Bellatrix it will use
// the IO_UART.
//
// When used on Castor serial_read will not handle interleaved frames from
// different endpoints, and hence should not be used if additional endpoints
// have been enabled using config_endpoints.
//
// The allocated size of buf, bufsize, needs to be equal or greater than
// nbytes. Otherwise no data will be read and an error will be returned.
int serial_read(uint8_t *buf, size_t bufsize, size_t nbytes)
{
	if (*ver < TKEY_VERSION_CASTOR) {
		return read(IO_UART, buf, bufsize, nbytes);
	}

	uint8_t available = 0;
	enum ioend endpoint = IO_NONE;
	size_t n = 0;
	size_t remaining = nbytes;
	uint8_t read_length = 0;

	while (remaining > 0) {
		if (readselect(IO_CDC, false, &endpoint, &available) < 0) {
			return -1;
		}
		// readselect should not return anything else than IO_CDC
		if (endpoint != IO_CDC) {
			return -1;
		}

		// Read as much as is available of what we expect from
		// the frame.
		read_length = available;

		if (remaining < read_length) {
			read_length = remaining;
		}

		int rd = read(IO_CDC, &buf[n], bufsize - n, read_length);
		if (rd < 0) {
			return -1;
		}
		n += rd;
		remaining -= rd;
	}
	return n;
}

// Discards nbytes of data.
// Uses IO_CDC for Castor or IO_UART for Bellatrix.
int serial_discard(size_t nbytes)
{
	if (*ver < TKEY_VERSION_CASTOR) {
		return discard(IO_UART, nbytes);
	}

	uint8_t available = 0;
	enum ioend endpoint = IO_NONE;
	size_t remaining = nbytes;
	uint8_t discard_len = 0;

	while (remaining > 0) {
		if (readselect(IO_CDC, false, &endpoint, &available) < 0) {
			return -1;
		}
		// readselect should not return anything else than IO_CDC
		if (endpoint != IO_CDC) {
			return -1;
		}

		discard_len = available;

		if (remaining < discard_len) {
			discard_len = remaining;
		}

		int rd = discard(IO_CDC, discard_len);
		if (rd < 0) {
			return -1;
		}
		remaining -= rd;
	}
	return 0;
}

// discard nbytes of what's available.
//
// Use IO_UART to discard bytes in the UART fifo-buffer
//
// Use IO_CDC, IO_FIDO, IO_CCID or IO_DEBUG to discard for respective endpoint.
//
// Returns how many bytes were discarded, negative on error.
int discard(enum ioend src, size_t nbytes)
{
	int n = 0;

	if (src == IO_NONE || src == IO_QEMU) {
		// Destination only endpoints
		return -1;
	}

	if (src == IO_UART) {
		for (n = 0; n < nbytes; n++) {
			(void)readbyte();
		}
	}

	if (src != cur_endpoint.endpoint) {
		// Nothing to discard for this endpoint
		return 0;
	}

	uint8_t len = nbytes < cur_endpoint.len ? nbytes : cur_endpoint.len;

	for (n = 0; n < len; n++) {
		(void)readbyte();
		cur_endpoint.len--;
	}

	return n;
}

// readselect optionally blocks and returns when there is something readable
// from some mode.
//
// Use like this:
//
//   readselect(IO_CDC|IO_FIDO, false, &endpoint, &len)
//
// to wait for some data from either the CDC or the FIDO endpoint.
//
// NOTE WELL: You need to call readselect() first, before doing any
// calls to read() if using Castor.
//
// Only endpoints available for read are:
//
// - IO_CH552
// - IO_CDC
// - IO_FIDO
// - IO_CCID
// - IO_DEBUG
//
// If you need blocking low-level UART reads, use uart_read() instead.
//
// When readselect is used with non_blocking set to true, it will either return
// zero or the entire header. As soon as the first byte is read, it waits
// for the next before returning.
//
// If readselect reads data from an endpoint not set in bitmask, it will be
// discarded.
//
// When reading data from an endpoint in bitmask the source of the data is
// returned in *endpoint and the length is returned in *len.
//
// Returns non-zero on error.
int readselect(int bitmask, bool non_blocking, enum ioend *endpoint,
	       uint8_t *len)
{
	if ((bitmask & IO_UART) || (bitmask & IO_QEMU)) {
		// Not possible to use readselect() on these
		// endpoints.
		return -1;
	}

	for (;;) {
		// Check what is in the current UART buffer.
		//
		// - If nothing known, block until something comes along.
		//
		// - If not in bitmask, discard the data available
		//   from that endpoint.
		//
		// - If in the bitmask, return the first endpoint with
		//   data available and indicate how much data in len.
		if (cur_endpoint.len == 0) {
			// Check if readselect should block
			if (non_blocking && !*can_rx) {
				*len = 0;
				return 0;
			}
			// Read USB Mode Protocol header:
			//   1 byte mode
			//   1 byte length
			cur_endpoint.endpoint = readbyte();
			cur_endpoint.len = readbyte();
		}

		*len = cur_endpoint.len;

		if (cur_endpoint.endpoint & bitmask) {
			*endpoint = cur_endpoint.endpoint;

			return 0;
		}

		// Not the USB endpoint caller asked for. Discard the
		// rest from this endpoint.
		if (discard(cur_endpoint.endpoint, *len) != *len) {
			// We couldn't discard what the USB Mode
			// Protocol itself reported was available!
			// Something's fishy. Halt.
			assert(1 == 2);
		}
	}

	return 0;
}

void putchar(enum ioend dest, const uint8_t ch)
{
	write(dest, &ch, 1);
}

static void hex(uint8_t buf[2], const uint8_t c)
{
	unsigned int upper = (c >> 4) & 0xf;
	unsigned int lower = c & 0xf;

	buf[0] = upper < 10 ? '0' + upper : 'a' - 10 + upper;
	buf[1] = lower < 10 ? '0' + lower : 'a' - 10 + lower;
}

void puthex(enum ioend dest, const uint8_t c)
{
	uint8_t hexbuf[2] = {0};

	hex(hexbuf, c);
	write(dest, hexbuf, 2);
}

// Size of of a maximum integer in hex text format
#define INTBUFSIZE 10

void putinthex(enum ioend dest, const uint32_t n)
{
	uint8_t buf[INTBUFSIZE] = {0};
	uint8_t hexbuf[2] = {0};
	uint8_t *intbuf = (uint8_t *)&n;
	int j = 0;

	buf[j++] = '0';
	buf[j++] = 'x';

	for (int i = 3; i > -1; i--) {
		hex(hexbuf, intbuf[i]);
		buf[j++] = hexbuf[0];
		buf[j++] = hexbuf[1];
	}

	write(dest, buf, INTBUFSIZE);
}

void puts(enum ioend dest, const char *s)
{
	write(dest, (const uint8_t *)s, strlen(s));
}

// Size of a hex row: Contains 16 bytes where each byte is printed as
// 3 characters (hex + hex + space). Every row ends with newline or at
// most CR+LF.
#define FULLROW (16 * 3)
#define ROWBUFSIZE (FULLROW + 2)

void hexdump(enum ioend dest, const void *buf, int len)
{
	uint8_t rowbuf[ROWBUFSIZE] = {0};
	uint8_t hexbuf[2] = {0};
	const uint8_t *byte_buf = buf;

	int rowpos = 0;
	for (int i = 0; i < len; i++) {
		hex(hexbuf, byte_buf[i]);
		rowbuf[rowpos++] = hexbuf[0];
		rowbuf[rowpos++] = hexbuf[1];
		rowbuf[rowpos++] = ' ';

		// If the row is full, print it now.
		if (rowpos == FULLROW) {
			if (dest == IO_CDC) {
				rowbuf[rowpos++] = '\r';
			}
			rowbuf[rowpos++] = '\n';
			write(dest, rowbuf, rowpos);
			rowpos = 0;
		}
	}

	// If final row wasn't full, print it now.
	if (rowpos != 0) {
		if (dest == IO_CDC) {
			rowbuf[rowpos++] = '\r';
		}
		rowbuf[rowpos++] = '\n';
		write(dest, rowbuf, rowpos);
	}
}

// Configure USB endpoints that should be enabled/disabled
//
// Allowed options are:
//   - IO_FIDO (can't be used used together with IO_CCID)
//   - IO_CCID (can't be used used together with IO_FIDO)
//   - IO_DEBUG
//
// The following are always enabled:
//   - IO_CDC
//   - IO_CH552
//
// Use like this in the bitmask:
//
//   config_endpoints(IO_FIDO|IO_DEBUG)
//
void config_endpoints(uint8_t endpoints)
{
	uint8_t cmdbuf[2] = {0};

	cmdbuf[0] = SET_ENDPOINTS;
	cmdbuf[1] = endpoints;

	write(IO_CH552, cmdbuf, 2);
}
