/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../mta1_mkdf/lib.h"
#include "../mta1_mkdf/proto.h"
#include "../mta1_mkdf/types.h"
#include "../mta1_mkdf_mem.h"

// clang-format off
volatile uint32_t *mta1name0 =  (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_NAME0;
volatile uint32_t *mta1name1 =  (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_NAME1;
volatile uint32_t *uds =        (volatile uint32_t *)MTA1_MKDF_MMIO_UDS_FIRST;
volatile uint32_t *uda =        (volatile uint32_t *)MTA1_MKDF_MMIO_QEMU_UDA; // Only in QEMU right now
volatile uint32_t *cdi =        (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_CDI_FIRST;
volatile uint32_t *udi =        (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_UDI_FIRST;
volatile uint32_t *switch_app = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_SWITCH_APP;
volatile uint8_t  *fw_ram =     (volatile uint8_t  *)MTA1_MKDF_MMIO_FW_RAM_BASE;
volatile uint32_t *timer =           (volatile uint32_t *)MTA1_MKDF_MMIO_TIMER_TIMER;
volatile uint32_t *timer_prescaler = (volatile uint32_t *)MTA1_MKDF_MMIO_TIMER_PRESCALER;
volatile uint32_t *timer_status =    (volatile uint32_t *)MTA1_MKDF_MMIO_TIMER_STATUS;
volatile uint32_t *timer_ctrl =      (volatile uint32_t *)MTA1_MKDF_MMIO_TIMER_CTRL;
volatile uint32_t *trng_status =  (volatile uint32_t *)MTA1_MKDF_MMIO_TRNG_STATUS;
volatile uint32_t *trng_entropy = (volatile uint32_t *)MTA1_MKDF_MMIO_TRNG_ENTROPY;
// clang-format on

// TODO Real UDA is 4 words (16 bytes)
#define UDA_WORDS 1

void test_puts(char *reason)
{
	for (char *c = reason; *c != '\0'; c++) {
		writebyte(*c);
	}
}

void test_putsn(char *p, int n)
{
	for (int i = 0; i < n; i++) {
		writebyte(p[i]);
	}
}

void test_puthex(uint8_t c)
{
	unsigned int upper = (c >> 4) & 0xf;
	unsigned int lower = c & 0xf;
	writebyte(upper < 10 ? '0' + upper : 'a' - 10 + upper);
	writebyte(lower < 10 ? '0' + lower : 'a' - 10 + lower);
}

void test_puthexn(uint8_t *p, int n)
{
	for (int i = 0; i < n; i++) {
		test_puthex(p[i]);
	}
}

void test_reverseword(uint32_t *wordp)
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

	test_puts("I'm testfw on:");
	// Output the MTA1 core's NAME0 and NAME1
	uint32_t name;
	wordcpy(&name, (void *)mta1name0, 1);
	test_reverseword(&name);
	test_putsn((char *)&name, 4);
	test_puts(" ");
	wordcpy(&name, (void *)mta1name1, 1);
	test_reverseword(&name);
	test_putsn((char *)&name, 4);
	test_puts("\r\n");

	int anyfailed = 0;

	uint32_t uds_local[8];
	uint32_t uds_zeros[8];
	memset(uds_zeros, 0, 8 * 4);
	// Should get non-empty UDS
	wordcpy(uds_local, (void *)uds, 8);
	if (memeq(uds_local, uds_zeros, 8 * 4)) {
		test_puts("FAIL: UDS empty\r\n");
		anyfailed = 1;
	}
	// Should NOT be able to read from UDS again
	wordcpy(uds_local, (void *)uds, 8);
	if (!memeq(uds_local, uds_zeros, 8 * 4)) {
		test_puts("FAIL: Read UDS a second time\r\n");
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

	uint32_t udi_local[2];
	uint32_t udi_zeros[2];
	memset(udi_zeros, 0, 2 * 4);
	// Should get non-empty UDI
	wordcpy(udi_local, (void *)udi, 2);
	if (memeq(udi_local, udi_zeros, 2 * 4)) {
		test_puts("FAIL: UDI empty\r\n");
		anyfailed = 1;
	}

	// Should be able to write to CDI in fw (non-app) mode.
	uint32_t cdi_writetest[8] = {0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
				     0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
				     0xdeafbeef, 0xdeafbeef};
	uint32_t cdi_readback[8];
	wordcpy((void *)cdi, cdi_writetest, 8);
	wordcpy(cdi_readback, (void *)cdi, 8);
	if (!memeq(cdi_writetest, cdi_readback, 8 * 4)) {
		test_puts("FAIL: Can't write CDI in fw mode\r\n");
		anyfailed = 1;
	}

	// Test FW-RAM.
	*fw_ram = 0x12;
	if (*fw_ram != 0x12) {
		test_puts("FAIL: Can't write and read FW RAM in fw mode\r\n");
		anyfailed = 1;
	}

	// Turn on application mode.
	// -------------------------
	*switch_app = 1;

	// Should NOT be able to read from UDS in app-mode.
	wordcpy(uds_local, (void *)uds, 8);
	if (!memeq(uds_local, uds_zeros, 8 * 4)) {
		test_puts("FAIL: Read from UDS in app-mode\r\n");
		anyfailed = 1;
	}

	// TODO test UDA once we have in in real hw
	// // Now we should NOT be able to read from UDA.
	// wordcpy(uda_local, (void *)uda, UDA_WORDS);
	// if (!memeq(uda_local, uda_zeros, UDA_WORDS*4)) {
	// 	test_puts("FAIL: Read from UDA in app-mode\r\n");
	// 	anyfailed = 1;
	// }

	uint32_t cdi_local[8];
	uint32_t cdi_local2[8];
	uint32_t cdi_zeros[8];
	memset(cdi_zeros, 0, 8 * 4);
	wordcpy(cdi_local, (void *)cdi, 8);
	// Write to CDI should NOT have any effect in app mode.
	wordcpy((void *)cdi, cdi_zeros, 8);
	wordcpy(cdi_local2, (void *)cdi, 8);
	if (!memeq(cdi_local, cdi_local2, 8 * 4)) {
		test_puts("FAIL: Write to CDI in app-mode\r\n");
		anyfailed = 1;
	}

	// Test FW-RAM.
	*fw_ram = 0x21;
	if (*fw_ram == 0x21) {
		test_puts("FAIL: Write and read FW RAM in app-mode\r\n");
		anyfailed = 1;
	}

	test_puts("Testing timer...\r\n");
	// Matching clock at 18 MHz, giving us timer in seconds
	*timer_prescaler = 18 * 1000000;

	// Test timer expiration after 1s
	*timer = 1;
	// Write anything to start timer
	*timer_ctrl = 1;
	for (;;) {
		if (*timer_status &
		    (1 << MTA1_MKDF_MMIO_TIMER_STATUS_READY_BIT)) {
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

	if (!(*timer_status & (1 << MTA1_MKDF_MMIO_TIMER_STATUS_READY_BIT))) {
		test_puts("FAIL: Timer didn't stop\r\n");
		anyfailed = 1;
	}

	if (*timer != 10) {
		test_puts("FAIL: Timer didn't reset to 10\r\n");
		anyfailed = 1;
	}

	// Check and display test results.
	if (anyfailed) {
		test_puts("Some test FAILED!\r\n");
	} else {
		test_puts("All tests passed.\r\n");
	}

	test_puts("\r\nHere are 256 bytes from the TRNG:\r\n");
	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 8; i++) {
			while ((*trng_status &
				(1 << MTA1_MKDF_MMIO_TRNG_STATUS_READY_BIT)) ==
			       0) {
			}
			uint32_t rnd = *trng_entropy;
			test_puthexn((uint8_t *)&rnd, 4);
			test_puts(" ");
		}
		test_puts("\r\n");
	}
	test_puts("\r\n");

	test_puts("Now echoing what you type...\r\n");
	for (;;) {
		in = readbyte(); // blocks
		writebyte(in);
	}
}
