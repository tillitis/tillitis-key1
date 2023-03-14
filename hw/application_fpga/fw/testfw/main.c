/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/blake2s/blake2s.h"
#include "../tk1/lib.h"
#include "../tk1/proto.h"
#include "../tk1/types.h"
#include "../tk1_mem.h"

// clang-format off
volatile uint32_t *tk1name0        = (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
volatile uint32_t *tk1name1        = (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
volatile uint32_t *uds             = (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
volatile uint32_t *cdi             = (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
volatile uint32_t *udi             = (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
volatile uint32_t *switch_app      = (volatile uint32_t *)TK1_MMIO_TK1_SWITCH_APP;
volatile uint8_t  *fw_ram          = (volatile uint8_t  *)TK1_MMIO_FW_RAM_BASE;
volatile uint32_t *timer           = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
volatile uint32_t *timer_status    = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
volatile uint32_t *timer_ctrl      = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
volatile uint32_t *trng_status     = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
volatile uint32_t *trng_entropy    = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
volatile uint32_t *fw_blake2s_addr = (volatile uint32_t *)TK1_MMIO_TK1_BLAKE2S;
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

void puts(char *reason)
{
	for (char *c = reason; *c != '\0'; c++) {
		writebyte(*c);
	}
}

void putsn(char *p, int n)
{
	for (int i = 0; i < n; i++) {
		writebyte(p[i]);
	}
}

void puthex(uint8_t c)
{
	unsigned int upper = (c >> 4) & 0xf;
	unsigned int lower = c & 0xf;
	writebyte(upper < 10 ? '0' + upper : 'a' - 10 + upper);
	writebyte(lower < 10 ? '0' + lower : 'a' - 10 + lower);
}

void puthexn(uint8_t *p, int n)
{
	for (int i = 0; i < n; i++) {
		puthex(p[i]);
	}
}

void hexdump(uint8_t *buf, int len)
{
	uint8_t *row;
	uint8_t *byte;
	uint8_t *max;

	row = buf;
	max = &buf[len];
	for (byte = 0; byte != max; row = byte) {
		for (byte = row; byte != max && byte != (row + 16); byte++) {
			puthex(*byte);
		}

		puts("\r\n");
	}
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

void zero_fwram()
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

int main()
{
	// Function pointer to blake2s()
	volatile int (*fw_blake2s)(void *, unsigned long, const void *,
				   unsigned long, const void *, unsigned long,
				   blake2s_ctx *);

	uint8_t in;

	// Wait for terminal program and a character to be typed
	in = readbyte();

	puts("I'm testfw on:");
	// Output the TK1 core's NAME0 and NAME1
	uint32_t name;
	wordcpy_s(&name, 1, (void *)tk1name0, 1);
	reverseword(&name);
	putsn((char *)&name, 4);
	puts(" ");
	wordcpy_s(&name, 1, (void *)tk1name1, 1);
	reverseword(&name);
	putsn((char *)&name, 4);
	puts("\r\n");

	uint32_t zeros[8];
	memset(zeros, 0, 8 * 4);

	int anyfailed = 0;

	uint32_t uds_local[UDS_WORDS];

	// Should get non-empty UDS
	wordcpy_s(uds_local, UDS_WORDS, (void *)uds, UDS_WORDS);
	if (memeq(uds_local, zeros, UDS_WORDS * 4)) {
		puts("FAIL: UDS empty\r\n");
		anyfailed = 1;
	}

	// Should NOT be able to read from UDS again
	wordcpy_s(uds_local, UDS_WORDS, (void *)uds, UDS_WORDS);
	if (!memeq(uds_local, zeros, UDS_WORDS * 4)) {
		puts("FAIL: Read UDS a second time\r\n");
		anyfailed = 1;
	}

	uint32_t udi_local[UDI_WORDS];
	// Should get non-empty UDI
	wordcpy_s(udi_local, UDI_WORDS, (void *)udi, UDI_WORDS);
	if (memeq(udi_local, zeros, UDI_WORDS * 4)) {
		puts("FAIL: UDI empty\r\n");
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
		puts("FAIL: Can't write CDI in fw mode\r\n");
		anyfailed = 1;
	}

	// Test FW_RAM.
	puts("Testing FW_RAM (takes 15s on hw)...\r\n");
	for (unsigned int i = 0; i < TK1_MMIO_FW_RAM_SIZE; i++) {
		zero_fwram();
		*(volatile uint8_t *)(TK1_MMIO_FW_RAM_BASE + i) = 0x42;
		anyfailed = check_fwram_zero_except(i, 0x42);
	}
	puts("\r\n");

	uint32_t sw = *switch_app;
	if (sw != 0) {
		puts("FAIL: switch_app is not 0 in fw mode\r\n");
		anyfailed = 1;
	}

	// Store function pointer to blake2s() so it's reachable from app
	*fw_blake2s_addr = (uint32_t)blake2s;

	// Turn on application mode.
	// -------------------------

	*switch_app = 1;

	sw = *switch_app;
	if (sw != 0xffffffff) {
		puts("FAIL: switch_app is not 0xffffffff in app mode\r\n");
		anyfailed = 1;
	}

	// Should NOT be able to read from UDS in app-mode.
	wordcpy_s(uds_local, UDS_WORDS, (void *)uds, UDS_WORDS);
	if (!memeq(uds_local, zeros, UDS_WORDS * 4)) {
		puts("FAIL: Read from UDS in app-mode\r\n");
		anyfailed = 1;
	}

	// Should NOT be able to read from UDI in app-mode.
	wordcpy_s(udi_local, UDI_WORDS, (void *)udi, UDI_WORDS);
	if (!memeq(udi_local, zeros, UDI_WORDS * 4)) {
		puts("FAIL: Read from UDI in app-mode\r\n");
		anyfailed = 1;
	}

	uint32_t cdi_local[CDI_WORDS];
	uint32_t cdi_local2[CDI_WORDS];
	wordcpy_s(cdi_local, CDI_WORDS, (void *)cdi, CDI_WORDS);

	// Write to CDI should NOT have any effect in app mode.
	wordcpy_s((void *)cdi, CDI_WORDS, zeros, CDI_WORDS);
	wordcpy_s(cdi_local2, CDI_WORDS, (void *)cdi, CDI_WORDS);
	if (!memeq(cdi_local, cdi_local2, CDI_WORDS * 4)) {
		puts("FAIL: Write to CDI in app-mode\r\n");
		anyfailed = 1;
	}

	// Test FW_RAM.
	*fw_ram = 0x21;
	if (*fw_ram == 0x21) {
		puts("FAIL: Write and read FW RAM in app-mode\r\n");
		anyfailed = 1;
	}

	puts("Testing timer... 3");
	// Matching clock at 18 MHz, giving us timer in seconds
	*timer_prescaler = 18 * 1000000;

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
		puts("FAIL: Timer didn't stop\r\n");
		anyfailed = 1;
	}

	if (*timer != 10) {
		puts("FAIL: Timer didn't reset to 10\r\n");
		anyfailed = 1;
	}

	// Testing the blake2s MMIO in app mode

	fw_blake2s = (volatile int (*)(void *, unsigned long, const void *,
				       unsigned long, const void *,
				       unsigned long, blake2s_ctx *)) *
		     fw_blake2s_addr;

	char msg[17] = "dldlkjsdkljdslsdj";
	uint32_t digest0[8];
	uint32_t digest1[8];
	blake2s_ctx b2s_ctx;

	blake2s(&digest0[0], 32, NULL, 0, &msg, 17, &b2s_ctx);
	fw_blake2s(&digest1[0], 32, NULL, 0, &msg, 17, &b2s_ctx);

	puts("digest #0: \r\n");
	hexdump((uint8_t *)digest0, 32);

	puts("digest #1: \r\n");
	hexdump((uint8_t *)digest1, 32);

	if (!memeq(digest0, digest1, 32)) {
		puts("FAIL: Digests not the same\r\n");
		anyfailed = 1;
	}

	// Check and display test results.
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

	puts("Now echoing what you type...\r\n");
	for (;;) {
		in = readbyte(); // blocks
		writebyte(in);
	}
}
