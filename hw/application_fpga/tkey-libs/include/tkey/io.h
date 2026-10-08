// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef TKEY_IO_H
#define TKEY_IO_H

// I/O endpoints. Keep it as bits possible to use in a bitmask in
// readselect().
//
// Note that the values for IO_CH552, IO_CDC, IO_FIDO, IO_CCID and IO_DEBUG
// should be kept the same in the code for the CH552 side.
enum ioend {
	IO_NONE = 0x00,	 // No endpoint
	IO_UART = 0x01,	 // Only destination, raw UART access
	IO_QEMU = 0x02,	 // Only destination, QEMU debug port
	IO_CH552 = 0x04, // Internal CH552 control port
	IO_CDC = 0x08,	 // CDC "serial" port
	IO_FIDO = 0x10,	 // FIDO security token port
	IO_CCID = 0x20,	 // CCID "smart card" port
	IO_DEBUG = 0x40, // Debug port over USB HID
};

enum ch552cmd {
	SET_ENDPOINTS = 0x01, // Config USB endpoints on the CH552
	CH552_CMD_MAX,
};

void write(enum ioend dest, const uint8_t *buf, size_t nbytes);
int read(enum ioend src, uint8_t *buf, size_t bufsize, size_t nbytes);
int readselect(int bitmask, bool non_blocking, enum ioend *endpoint,
	       uint8_t *len);
int discard(enum ioend src, size_t nbytes);
int serial_write(const uint8_t *buf, size_t nbytes);
int serial_read(uint8_t *buf, size_t bufsize, size_t nbytes);
int serial_discard(size_t nbytes);
void putchar(enum ioend dest, const uint8_t ch);
void puthex(enum ioend dest, const uint8_t ch);
void putinthex(enum ioend dest, const uint32_t n);
void puts(enum ioend dest, const char *s);
void hexdump(enum ioend dest, const void *buf, int len);
void config_endpoints(uint8_t endpoints);

#endif
