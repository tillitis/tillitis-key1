/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/lib.h"
#include "../tk1/proto.h"
#include "../tk1/types.h"
#include "../tk1_mem.h"

#define USBMODE_PACKET_SIZE 64

// clang-format off
volatile uint32_t *tk1name0         = (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
volatile uint32_t *tk1name1         = (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
volatile uint32_t *uds              = (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
volatile uint32_t *cdi              = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
volatile uint32_t *udi              = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
volatile uint8_t  *fw_ram           = (volatile uint8_t  *)TK1_MMIO_FW_RAM_BASE;
volatile uint32_t *system_reset     = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
volatile uint32_t *timer            = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
volatile uint32_t *timer_prescaler  = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
volatile uint32_t *timer_status     = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
volatile uint32_t *timer_ctrl       = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
volatile uint32_t *trng_status      = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
volatile uint32_t *trng_entropy     = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
// clang-format on

#define UDS_WORDS 8
#define UDI_WORDS 2
#define CDI_WORDS 8

void *memcpy(void *dest, const void *src, size_t n)
{
	uint8_t *src_byte = (uint8_t *)src;
	uint8_t *dest_byte = (uint8_t *)dest;

	for (int i = 0; i < n; i++) {
		dest_byte[i] = src_byte[i];
	}

	return dest;
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

static void write(const uint8_t *buf, size_t nbytes)
{
	uint8_t len;

	while (nbytes > 0) {
		// We split the data into chunks that will fit in the
		// USB Mode Protocol with some spare change.
		len =
		    nbytes < USBMODE_PACKET_SIZE ? nbytes : USBMODE_PACKET_SIZE;

		write_with_header((const uint8_t *)buf, len, MODE_CDC);

		buf += len;
		nbytes -= len;
	}
}

unsigned strlen(const char *str)
{
	const char *s;

	for (s = str; *s; ++s)
		;

	return (s - str);
}

void puts(char *buf)
{
	size_t nbytes = strlen(buf);

	write((const uint8_t *)buf, nbytes);
}

void hex(uint8_t buf[2], const uint8_t c)
{
	unsigned int upper = (c >> 4) & 0xf;
	unsigned int lower = c & 0xf;

	buf[0] = upper < 10 ? '0' + upper : 'a' - 10 + upper;
	buf[1] = lower < 10 ? '0' + lower : 'a' - 10 + lower;
}

void puthex(uint8_t c)
{
	uint8_t buf[2];

	hex(buf, c);
	write(buf, 2);
}

void puthexn(uint8_t *p, int n)
{
	for (int i = 0; i < n; i++) {
		puthex(p[i]);
	}
}

void hexdump(void *buf, int len)
{
	uint8_t *byte_buf = (uint8_t *)buf;

	for (int i = 0; i < len; i++) {
		puthex(byte_buf[i]);
		if (i % 2 == 1) {
			puts(" ");
		}

		if (i != 1 && i % 16 == 1) {
			puts("\r\n");
		}
	}

	puts("\r\n");
}

void reverseword(uint32_t *wordp)
{
	*wordp = ((*wordp & 0xff000000) >> 24) | ((*wordp & 0x00ff0000) >> 8) |
		 ((*wordp & 0x0000ff00) << 8) | ((*wordp & 0x000000ff) << 24);
}

uint32_t wait_timer_tick(uint32_t last_timer)
{
	uint32_t newtimer;
	for (;;) {
		newtimer = *timer;
		if (newtimer != last_timer) {
			return newtimer;
		}
	}
}

void zero_fwram(void)
{
	for (int i = 0; i < TK1_MMIO_FW_RAM_SIZE; i++) {
		fw_ram[i] = 0x00;
	}
}

int check_fwram_zero_except(unsigned int offset, uint8_t expected_val)
{
	int failed = 0;
	for (unsigned int i = 0; i < TK1_MMIO_FW_RAM_SIZE; i++) {
		uint32_t addr = TK1_MMIO_FW_RAM_BASE + i;
		uint8_t *p = (uint8_t *)addr;
		uint8_t val = *(volatile uint8_t *)p;
		int failed_now = 0;
		if (i == offset) {
			if (val != expected_val) {
				failed_now = 1;
				puts("  wrong value at: ");
			}
		} else {
			if (val != 0) {
				failed_now = 1;
				puts("  not zero at: ");
			}
		}
		if (failed_now) {
			failed = 1;
			reverseword(&addr);
			puthexn((uint8_t *)&addr, 4);
			puts("\r\n");
		}
	}
	return failed;
}

void failmsg(char *s)
{
	puts("FAIL: ");
	puts(s);
	puts("\r\n");
}

int main(void)
{
	uint8_t in = 0;
	uint8_t mode = 0;
	uint8_t mode_bytes_left = 0;

	// Hard coded test UDS in ../../data/uds.hex
	// clang-format off
	uint32_t uds_test[8] = {
		0x80818283,
		0x94959697,
		0xa0a1a2a3,
		0xb4b5b6b7,
		0xc0c1c2c3,
		0xd4d5d6d7,
		0xe0e1e2e3,
		0xf4f5f6f7,
	};
	// clang-format on

	// Wait for terminal program and a character to be typed
	in = readbyte(&mode, &mode_bytes_left);

	puts("\r\nI'm testfw on:");
	// Output the TK1 core's NAME0 and NAME1
	uint32_t name;
	wordcpy_s(&name, 1, (void *)tk1name0, 1);
	reverseword(&name);
	write((const uint8_t *)&name, 4);
	puts(" ");
	wordcpy_s(&name, 1, (void *)tk1name1, 1);
	reverseword(&name);
	write((const uint8_t *)&name, 4);
	puts("\r\n");

	uint32_t zeros[8];
	memset(zeros, 0, 8 * 4);

	int anyfailed = 0;

	uint32_t uds_local[UDS_WORDS];

	// Should get non-empty UDS
	wordcpy_s(uds_local, UDS_WORDS, (void *)uds, UDS_WORDS);
	if (memeq(uds_local, zeros, UDS_WORDS * 4)) {
		failmsg("UDS empty");
		anyfailed = 1;
	}

	puts("\r\nUDS: ");
	for (int i = 0; i < UDS_WORDS * 4; i++) {
		puthex(((uint8_t *)uds_local)[i]);
	}
	puts("\r\n");
	if (!memeq(uds_local, uds_test, UDS_WORDS * 4)) {
		failmsg("UDS not equal to test UDS");
		anyfailed = 1;
	}

	// Should NOT be able to read from UDS again
	wordcpy_s(uds_local, UDS_WORDS, (void *)uds, UDS_WORDS);
	if (!memeq(uds_local, zeros, UDS_WORDS * 4)) {
		failmsg("Read UDS a second time");
		anyfailed = 1;
	}

	uint32_t udi_local[UDI_WORDS];
	// Should get non-empty UDI
	wordcpy_s(udi_local, UDI_WORDS, (void *)udi, UDI_WORDS);
	if (memeq(udi_local, zeros, UDI_WORDS * 4)) {
		failmsg("UDI empty");
		anyfailed = 1;
	}

	// Should be able to write to CDI in fw (non-app) mode.
	uint32_t cdi_writetest[CDI_WORDS] = {0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
					     0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
					     0xdeafbeef, 0xdeafbeef};
	uint32_t cdi_readback[CDI_WORDS];

	wordcpy_s((void *)cdi, CDI_WORDS, cdi_writetest, CDI_WORDS);
	wordcpy_s(cdi_readback, CDI_WORDS, (void *)cdi, CDI_WORDS);
	if (!memeq(cdi_writetest, cdi_readback, CDI_WORDS * 4)) {
		failmsg("Can't write CDI in fw mode");
		anyfailed = 1;
	}

	// Should be able to read bytes from CDI.
	uint8_t cdi_readback_bytes[CDI_WORDS * 4];
	memcpy(cdi_readback_bytes, (void *)cdi, CDI_WORDS * 4);
	if (!memeq(cdi_writetest, cdi_readback_bytes, CDI_WORDS * 4)) {
		failmsg("Can't read bytes from CDI");
		anyfailed = 1;
	}

	// Test FW_RAM.
	puts("\r\nTesting FW_RAM (takes 50s on hw)...\r\n");
	for (unsigned int i = 0; i < TK1_MMIO_FW_RAM_SIZE; i++) {
		zero_fwram();
		*(volatile uint8_t *)(TK1_MMIO_FW_RAM_BASE + i) = 0x42;
		int fwram_fail = check_fwram_zero_except(i, 0x42);
		if (fwram_fail) {
			anyfailed = 1;
		}
	}

	puts("\r\nTesting timer... 3");
	// Matching clock at 24 MHz, giving us timer in seconds
	*timer_prescaler = 24 * 1000000;

	// Test timer expiration after 1s
	*timer = 1;
	// Start the timer
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);
	while (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
	}
	// Now timer has expired and is ready to run again
	puts(" 2");

	// Test to interrupt a timer - and reads from timer register
	// Starting 10s timer and interrupting it in 3s...
	*timer = 10;
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);
	uint32_t last_timer = 10;
	for (int i = 0; i < 3; i++) {
		last_timer = wait_timer_tick(last_timer);
	}

	// Stop the timer
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_STOP_BIT);
	puts(" 1. done.\r\n");

	if (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
		failmsg("Timer didn't stop");
		anyfailed = 1;
	}

	if (*timer != 10) {
		failmsg("Timer didn't reset to 10");
		anyfailed = 1;
	}

	// Check and display test results.
	puts("\r\n--> ");
	if (anyfailed) {
		puts("Some test FAILED!\r\n");
	} else {
		puts("All tests passed.\r\n");
	}

	puts("\r\nHere are 256 bytes from the TRNG:\r\n");

	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 8; i++) {
			while ((*trng_status &
				(1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
			}
			uint32_t rnd = *trng_entropy;
			puthexn((uint8_t *)&rnd, 4);
			puts(" ");
		}
		puts("\r\n");
	}
	puts("\r\n");

	puts("Now echoing what you type...Type + to reset device\r\n");
	for (;;) {
		in = readbyte(&mode, &mode_bytes_left);
		if (in == '+') {
			*system_reset = 1;
		}

		writebyte(MODE_CDC);
		writebyte(1);
		writebyte(in);
	}
}
