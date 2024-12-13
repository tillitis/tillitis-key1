/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/types.h"
#include "../tk1/led.h"
#include "../tk1/assert.h"

// Proof-of-concept firmware for handling syscalls.
// This is NOT a best-practice example of secure syscall implementation.

#define SYSCALL_HI (1 << 31)
#define SYSCALL_LO 0

#define SYSCALL_HI_SET_LED (SYSCALL_HI | 10)
#define SYSCALL_LO_SET_LED (SYSCALL_LO | 10)

static void delay(int32_t count) {
	volatile int32_t c = count;
	while (c > 0) {
		c--;
	}
}

int32_t syscall_lo_handler(uint32_t syscall_nr, uint32_t arg1) {
	switch (syscall_nr) {
	case SYSCALL_LO_SET_LED:
		set_led(arg1);
		//delay(1000000);
		return 0;
	default:
		assert(1 == 2);
	}

	assert(1 == 2);
	return -1; // This should never run
}

int32_t syscall_hi_handler(uint32_t syscall_nr, uint32_t arg1) {
	switch (syscall_nr) {
	case SYSCALL_HI_SET_LED:
		set_led(arg1);
		//delay(500000);
		return 0;
	default:
		assert(1 == 2);
	}

	assert(1 == 2);
	return -1; // This should never run
}

int main(void)
{
	while (1) {
	}
}
