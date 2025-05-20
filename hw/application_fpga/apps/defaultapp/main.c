// Copyright (C) 2025 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <tkey/debug.h>
#include <tkey/led.h>

#include "../testapp/syscall.h"
#include "../tk1/reset.h"
#include "../tk1/syscall_num.h"

int main(void)
{
	struct reset rst = {0};

	led_set(LED_BLUE);

	rst.type = START_CLIENT;
	syscall(TK1_SYSCALL_RESET, (uint32_t)&rst, 0, 0);
}
