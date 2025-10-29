// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <fw/tk1/proto.h>
#include <fw/tk1/reset.h>
#include <fw/tk1/syscall_num.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "syscall.h"

#define USBMODE_PACKET_SIZE 64

// clang-format off
volatile uint32_t *tk1name0         = (volatile uint32_t *)TK1_MMIO_TK1_NAME0;
volatile uint32_t *tk1name1         = (volatile uint32_t *)TK1_MMIO_TK1_NAME1;
volatile uint32_t *tk1version       = (volatile uint32_t *)TK1_MMIO_TK1_VERSION;
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

void puthexn(uint8_t *p, int n)
{
	for (int i = 0; i < n; i++) {
		puthex(IO_CDC, p[i]);
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

void failmsg(char *s)
{
	puts(IO_CDC, "FAIL: ");
	puts(IO_CDC, s);
	puts(IO_CDC, "\r\n");
}

int main(void)
{
	uint8_t in = 0;
	uint8_t available = 0;
	enum ioend endpoint = IO_NONE;

	led_set(LED_BLUE);

	// Wait for terminal program and a character to be typed
	if (readselect(IO_CDC, &endpoint, &available) < 0) {
		// readselect failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	if (read(IO_CDC, &in, 1, 1) < 0) {
		// read failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	puts(IO_CDC, "\r\nI'm testapp on:");
	// Output the TK1 core's NAME0 and NAME1
	uint32_t name;
	wordcpy_s(&name, 1, (void *)tk1name0, 1);
	reverseword(&name);
	write(IO_CDC, (const uint8_t *)&name, 4);
	puts(IO_CDC, " ");
	wordcpy_s(&name, 1, (void *)tk1name1, 1);
	reverseword(&name);
	write(IO_CDC, (const uint8_t *)&name, 4);
	puts(IO_CDC, "\r\n");
	puts(IO_CDC, "Version: ");
	putinthex(IO_CDC, *tk1version);
	puts(IO_CDC, "\r\n");

	uint32_t zeros[8];
	memset(zeros, 0, 8 * 4);

	int anyfailed = 0;

	uint32_t uds_local[UDS_WORDS];
	uint32_t udi_local[UDI_WORDS];

	// Should NOT be able to read from UDS in app-mode.
	wordcpy_s(uds_local, UDS_WORDS, (void *)uds, UDS_WORDS);
	if (!memeq(uds_local, zeros, UDS_WORDS * 4)) {
		failmsg("Read from UDS in app-mode");
		anyfailed = 1;
	}

	// Should NOT be able to read from UDI in app-mode.
	wordcpy_s(udi_local, UDI_WORDS, (void *)udi, UDI_WORDS);
	if (!memeq(udi_local, zeros, UDI_WORDS * 4)) {
		failmsg("Read from UDI in app-mode");
		anyfailed = 1;
	}

	// But a syscall to get parts of UDI should be able to run
	int vidpid = syscall(TK1_SYSCALL_GET_VIDPID, 0, 0, 0);

	if (vidpid != 0x073570c0) {
		failmsg("Expected VID/PID to be 0x073570c0");
		anyfailed = 1;
	}

	puts(IO_CDC, "\r\nAllocating storage area...");

	if (syscall(TK1_SYSCALL_ALLOC_AREA, 0, 0, 0) != 0) {
		failmsg("Failed to allocate storage area");
	}
	puts(IO_CDC, "done.\r\n");

	puts(IO_CDC, "\r\nWriting to storage area...");

	uint8_t out_data[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
	if (syscall(TK1_SYSCALL_WRITE_DATA, 0, (uint32_t)out_data,
		    sizeof(out_data)) != 0) {
		failmsg("Failed to write to storage area");
	}
	puts(IO_CDC, "done.\r\n");

	puts(IO_CDC, "\r\nReading data from storage area...");

	uint8_t in_data[14] = {0};
	if (syscall(TK1_SYSCALL_READ_DATA, 0, (uint32_t)in_data,
		    sizeof(in_data)) != 0) {
		failmsg("Failed to write to storage area");
	}
	if (!memeq(in_data, out_data, sizeof(in_data))) {
		failmsg("Failed to read back data from storage area");
		anyfailed = 1;
	}
	puts(IO_CDC, "done.\r\n");

	puts(IO_CDC, "\r\nErasing written data from storage area...");

	if (syscall(TK1_SYSCALL_ERASE_DATA, 0, 4096, 0) != 0) {
		failmsg("Failed to erase storage area");
	}
	puts(IO_CDC, "done.\r\n");

	puts(IO_CDC, "\r\nVerify erased storage area data...");

	if (syscall(TK1_SYSCALL_READ_DATA, 0, (uint32_t)in_data,
		    sizeof(in_data)) != 0) {
		failmsg("Failed to write to storage area");
	}
	uint8_t check_data[sizeof(in_data)] = {0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xff, 0xff, 0xff};
	if (!memeq(in_data, check_data, sizeof(check_data))) {
		failmsg("Failed to read back data from storage area");
		anyfailed = 1;
	}
	puts(IO_CDC, "done.\r\n");

	puts(IO_CDC, "\r\nDeallocating storage area...");

	if (syscall(TK1_SYSCALL_DEALLOC_AREA, 0, 0, 0) != 0) {
		failmsg("Failed to deallocate storage area");
	}
	puts(IO_CDC, "done.\r\n");

	uint32_t cdi_local[CDI_WORDS];
	uint32_t cdi_local2[CDI_WORDS];
	wordcpy_s(cdi_local, CDI_WORDS, (void *)cdi, CDI_WORDS);

	// Write to CDI should NOT have any effect in app mode.
	wordcpy_s((void *)cdi, CDI_WORDS, zeros, CDI_WORDS);
	wordcpy_s(cdi_local2, CDI_WORDS, (void *)cdi, CDI_WORDS);
	if (!memeq(cdi_local, cdi_local2, CDI_WORDS * 4)) {
		failmsg("Write to CDI in app-mode");
		anyfailed = 1;
	}

	// Should NOT be able to reset Tkey from app mode
	puts(IO_CDC, "\r\nTesting system reset...");
	*system_reset = 1;
	puts(IO_CDC, "done.\r\n");

	// Test FW_RAM.
	*fw_ram = 0x21;
	if (*fw_ram == 0x21) {
		failmsg("Write and read FW RAM in app-mode");
		anyfailed = 1;
	}

	puts(IO_CDC, "\r\nTesting timer... 3");
	// Matching clock at 24 MHz, giving us timer in seconds
	*timer_prescaler = 24 * 1000000;

	// Test timer expiration after 1s
	*timer = 1;
	// Start the timer
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);
	while (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
	}
	// Now timer has expired and is ready to run again
	puts(IO_CDC, " 2");

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
	puts(IO_CDC, " 1. done.\r\n");

	if (*timer_status & (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
		failmsg("Timer didn't stop");
		anyfailed = 1;
	}

	if (*timer != 10) {
		failmsg("Timer didn't reset to 10");
		anyfailed = 1;
	}

	// Check and display test results.
	puts(IO_CDC, "\r\n--> ");
	if (anyfailed) {
		puts(IO_CDC, "Some test FAILED!\r\n");
	} else {
		puts(IO_CDC, "All tests passed.\r\n");
	}

	puts(IO_CDC, "\r\nHere are 256 bytes from the TRNG:\r\n");
	for (int j = 0; j < 8; j++) {
		for (int i = 0; i < 8; i++) {
			while ((*trng_status &
				(1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
			}
			uint32_t rnd = *trng_entropy;
			puthexn((uint8_t *)&rnd, 4);
			puts(IO_CDC, " ");
		}
		puts(IO_CDC, "\r\n");
	}
	puts(IO_CDC, "\r\n");

	puts(IO_CDC, "Now echoing what you type...Type + to reset device\r\n");
	for (;;) {
		if (readselect(IO_CDC, &endpoint, &available) < 0) {
			// readselect failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		if (read(IO_CDC, &in, 1, 1) < 0) {
			// read failed! I/O broken? Just redblink.
			assert(1 == 2);
		}

		if (in == '+') {
			struct reset rst;
			memset(&rst, 0, sizeof(rst));
			rst.type = START_DEFAULT;
			syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
		}

		write(IO_CDC, &in, 1);
	}
}
