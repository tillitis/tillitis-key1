/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "../tk1/types.h"
#include "../tk1/led.h"
#include "../tk1/assert.h"

// Proof-of-concept firmware for handling syscalls.
// This is NOT a best-practice example of secure syscall implementation.

#define SYSCALL_SET_LED 10

int32_t syscall_handler(uint32_t syscall_nr, uint32_t arg1) {
	switch (syscall_nr) {
	case SYSCALL_SET_LED:
		set_led(arg1);
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
