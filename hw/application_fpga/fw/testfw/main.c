/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../mta1_mkdf/lib.h"
#include "../mta1_mkdf/proto.h"
#include "../mta1_mkdf/types.h"
#include "../mta1_mkdf_mem.h"

// clang-format off
volatile uint32_t *mta1name0 = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_NAME0;
volatile uint32_t *mta1name1 = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_NAME1;
volatile uint32_t *uds = (volatile uint32_t *)MTA1_MKDF_MMIO_UDS_FIRST;
volatile uint32_t *uda = (volatile uint32_t *)MTA1_MKDF_MMIO_QEMU_UDA; // Only in QEMU right now
volatile uint32_t *cdi = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_CDI_FIRST;
volatile uint32_t *udi = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_UDI_FIRST;
volatile uint32_t *switch_app = (volatile uint32_t *)MTA1_MKDF_MMIO_MTA1_SWITCH_APP;
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

int main()
{
	uint8_t in;

	// Wait for terminal program and a character to be typed
	in = readbyte();

	test_puts("Hello, I'm testfw on:");
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
		test_puts("FAIL: UDS empty!\r\n");
		anyfailed = 1;
	}
	// Should NOT be able to read from UDS again
	wordcpy(uds_local, (void *)uds, 8);
	if (!memeq(uds_local, uds_zeros, 8 * 4)) {
		test_puts("FAIL: Could read UDS a second time!\r\n");
		anyfailed = 1;
	}

	// TODO test UDA once we have it in real hw
	// uint32_t uda_local[UDA_WORDS];
	// uint32_t uda_zeros[UDA_WORDS];
	// memset(uda_zeros, 0, UDA_WORDS*4);
	// // Should get non-empty UDA
	// wordcpy(uda_local, (void *)uda, UDA_WORDS);
	// if (memeq(uda_local, uda_zeros, UDA_WORDS*4)) {
	// 	test_puts("FAIL: UDA empty!\r\n");
	// 	anyfailed = 1;
	// }

	uint32_t udi_local[2];
	uint32_t udi_zeros[2];
	memset(udi_zeros, 0, 2 * 4);
	// Should get non-empty UDI
	wordcpy(udi_local, (void *)udi, 2);
	if (memeq(udi_local, udi_zeros, 2 * 4)) {
		test_puts("FAIL: UDI empty!\r\n");
		anyfailed = 1;
	}

	// Should be able to write to CDI in non-app mode.
	uint32_t cdi_writetest[8] = {0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
				     0xdeafbeef, 0xdeafbeef, 0xdeafbeef,
				     0xdeafbeef, 0xdeafbeef};
	uint32_t cdi_readback[8];
	wordcpy((void *)cdi, cdi_writetest, 8);
	wordcpy(cdi_readback, (void *)cdi, 8);
	if (!memeq(cdi_writetest, cdi_readback, 8 * 4)) {
		test_puts("FAIL: Could not write to CDI in non-app mode!\r\n");
		anyfailed = 1;
	}

	// Turn on application mode
	*switch_app = 1;

	// Should NOT be able to read from UDS in app-mode.
	wordcpy(uds_local, (void *)uds, 8);
	if (!memeq(uds_local, uds_zeros, 8 * 4)) {
		test_puts("FAIL: Could read from UDS in app-mode!\r\n");
		anyfailed = 1;
	}

	// TODO test UDA once we have in in real hw
	// // Now we should NOT be able to read from UDA.
	// wordcpy(uda_local, (void *)uda, UDA_WORDS);
	// if (!memeq(uda_local, uda_zeros, UDA_WORDS*4)) {
	// 	test_puts("FAIL: Could read from UDA in app-mode!\r\n");
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
		test_puts("FAIL: Could write to CDI in app-mode!\r\n");
		anyfailed = 1;
	}

	if (anyfailed) {
		test_puts("Some test failed!\r\n");
	} else {
		test_puts("All tests passed.\r\n");
	}

	test_puts("Now echoing what you type...\r\n");
	for (;;) {
		in = readbyte(); // blocks
		writebyte(in);
	}
}
