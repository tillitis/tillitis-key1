/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/lib.h"
#include "../tk1/proto.h"
#include "../tk1/types.h"
#include "../tk1_mem.h"

// clang-format off
volatile uint32_t *tk1name0 =   (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
volatile uint32_t *tk1name1 =   (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
volatile uint32_t *uds =        (volatile uint32_t *)TK1_MMIO_UDS_FIRST;
volatile uint32_t *uda =        (volatile uint32_t *)TK1_MMIO_QEMU_UDA; // Only in QEMU right now
volatile uint32_t *cdi =        (volatile uint32_t *)TK1_MMIO_TK1_CDI_FIRST;
volatile uint32_t *udi =        (volatile uint32_t *)TK1_MMIO_TK1_UDI_FIRST;
volatile uint32_t *switch_app = (volatile uint32_t *)TK1_MMIO_TK1_SWITCH_APP;
volatile uint8_t  *fw_ram =     (volatile uint8_t  *)TK1_MMIO_FW_RAM_BASE;
volatile uint32_t *timer =           (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
volatile uint32_t *timer_status =    (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
volatile uint32_t *timer_ctrl =      (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
volatile uint32_t *trng_status =  (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
volatile uint32_t *trng_entropy = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;
// clang-format on

// TODO Real UDA is 4 words (16 bytes)
#define UDA_WORDS 1

#define UDS_WORDS 8
#define UDI_WORDS 2
#define CDI_WORDS 8

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

int main()
{
	uint8_t in;

	// Wait for terminal program and a character to be typed
	in = readbyte();

	puts("I'm testfw on:");
	// Output the TK1 core's NAME0 and NAME1
	uint32_t name;
	wordcpy(&name, (void *)tk1name0, 1);
	reverseword(&name);
	putsn((char *)&name, 4);
	puts(" ");
	wordcpy(&name, (void *)tk1name1, 1);
	reverseword(&name);
	putsn((char *)&name, 4);
	puts("\r\n");

	uint32_t zeros[8];
	memset(zeros, 0, 8 * 4);

	int anyfailed = 0;

	uint32_t uds_local[UDS_WORDS];
	// Should get non-empty UDS
	wordcpy(uds_local, (void *)uds, UDS_WORDS);
	if (memeq(uds_local, zeros, UDS_WORDS * 4)) {
		puts("FAIL: UDS empty\r\n");
		anyfailed = 1;
	}
	// Should NOT be able to read from UDS again
	wordcpy(uds_local, (void *)uds, UDS_WORDS);
	if (!memeq(uds_local, zeros, UDS_WORDS * 4)) {
		puts("FAIL: Read UDS a second time\r\n");
		anyfailed = 1;
	}

	// TODO test UDA once we have it in real hw
	// uint32_t uda_local[UDA_WORDS];
	// uint32_t uda_zeros[UDA_WORDS];
	// memset(uda_zeros, 0, UDA_WORDS*4);
	// // Should get non-empty UDA
	// wordcpy(uda_local, (void *)uda, UDA_WORDS);
	// if (memeq(uda_local, uda_zeros, UDA_WORDS*4)) {
	// 	test_puts("FAIL: UDA empty\r\n");
	// 	anyfailed = 1;
	// }

	uint32_t udi_local[UDI_WORDS];
	// Should get non-empty UDI
	wordcpy(udi_local, (void *)udi, UDI_WORDS);
	if (memeq(udi_local, zeros, UDI_WORDS * 4)) {
		puts("FAIL: UDI empty\r\n");
		anyfailed = 1;
	}

	// Should be able to write to CDI in fw (non-app) mode.
	uint32_t cdi_writetest[CDI_WORDS] = {0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
					     0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
					     0xdeafbeef, 0xdeafbeef};
	uint32_t cdi_readback[CDI_WORDS];
	wordcpy((void *)cdi, cdi_writetest, CDI_WORDS);
	wordcpy(cdi_readback, (void *)cdi, CDI_WORDS);
	if (!memeq(cdi_writetest, cdi_readback, CDI_WORDS * 4)) {
		puts("FAIL: Can't write CDI in fw mode\r\n");
		anyfailed = 1;
	}

	// Test FW-RAM.
	*fw_ram = 0x12;
	if (*fw_ram != 0x12) {
		puts("FAIL: Can't write and read FW RAM in fw mode\r\n");
		anyfailed = 1;
	}

	uint32_t sw = *switch_app;
	if (sw != 0) {
		puts("FAIL: switch_app is not 0 in fw mode\r\n");
		anyfailed = 1;
	}

	// Turn on application mode.
	// -------------------------
	*switch_app = 1;

	sw = *switch_app;
	if (sw != 0xffffffff) {
		puts("FAIL: switch_app is not 0xffffffff in app mode\r\n");
		anyfailed = 1;
	}

	// Should NOT be able to read from UDS in app-mode.
	wordcpy(uds_local, (void *)uds, UDS_WORDS);
	if (!memeq(uds_local, zeros, UDS_WORDS * 4)) {
		puts("FAIL: Read from UDS in app-mode\r\n");
		anyfailed = 1;
	}

	// TODO test UDA once we have in in real hw
	// // Now we should NOT be able to read from UDA.
	// wordcpy(uda_local, (void *)uda, UDA_WORDS);
	// if (!memeq(uda_local, uda_zeros, UDA_WORDS*4)) {
	// 	test_puts("FAIL: Read from UDA in app-mode\r\n");
	// 	anyfailed = 1;
	// }

	uint32_t cdi_local[CDI_WORDS];
	uint32_t cdi_local2[CDI_WORDS];
	wordcpy(cdi_local, (void *)cdi, CDI_WORDS);
	// Write to CDI should NOT have any effect in app mode.
	wordcpy((void *)cdi, zeros, CDI_WORDS);
	wordcpy(cdi_local2, (void *)cdi, CDI_WORDS);
	if (!memeq(cdi_local, cdi_local2, CDI_WORDS * 4)) {
		puts("FAIL: Write to CDI in app-mode\r\n");
		anyfailed = 1;
	}

	// Test FW-RAM.
	*fw_ram = 0x21;
	if (*fw_ram == 0x21) {
		puts("FAIL: Write and read FW RAM in app-mode\r\n");
		anyfailed = 1;
	}

	puts("Testing timer...\r\n");
	// Matching clock at 18 MHz, giving us timer in seconds
	*timer_prescaler = 18 * 1000000;

	// Test timer expiration after 1s
	*timer = 1;
	// Write anything to start timer
	*timer_ctrl = 1;
	for (;;) {
		if (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_READY_BIT)) {
			// Timer expired (it is ready to start again)
			break;
		}
	}

	// Test to interrupt a timer - and reads from timer register
	// Starting 10s timer and interrupting it in 3s...
	*timer = 10;
	*timer_ctrl = 1;
	uint32_t last_timer = 10;
	for (int i = 0; i < 3; i++) {
		last_timer = wait_timer_tick(last_timer);
	}
	// Write anything to stop the timer
	*timer_ctrl = 1;

	if (!(*timer_status & (1 << TK1_MMIO_TIMER_STATUS_READY_BIT))) {
		puts("FAIL: Timer didn't stop\r\n");
		anyfailed = 1;
	}

	if (*timer != 10) {
		puts("FAIL: Timer didn't reset to 10\r\n");
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
