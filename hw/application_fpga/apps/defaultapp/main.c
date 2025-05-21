// Copyright (C) 2025 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <fw/tk1/reset.h>
#include <fw/tk1/syscall_num.h>
#include <syscall.h>
#include <tkey/debug.h>
#include <tkey/led.h>

int main(void)
{
	struct reset rst = {0};

	led_set(LED_BLUE);

	rst.type = START_CLIENT;
	syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
}
