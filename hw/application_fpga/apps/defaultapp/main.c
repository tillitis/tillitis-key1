// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <fw/tk1/reset.h>
#include <syscall.h>
#include <tkey/debug.h>
#include <tkey/led.h>
#include <tkey/syscall.h>

int main(void)
{
	struct reset rst = {0};

	led_set(LED_BLUE);

	rst.type = START_CLIENT;
	syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
}
