// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/platform.h>
#include <tkey/tk1_mem.h>

// clang-format off
static volatile uint32_t *timer		  = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_status	  = (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
static volatile uint32_t *timer_ctrl	  = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
static volatile uint32_t *version	  = (volatile uint32_t *)TK1_MMIO_TK1_VERSION;
// clang-format on

void timer_wait(int delay_s)
{
	// Tick once every second
	*timer_prescaler = *version >= TKEY_VERSION_CASTOR
			       ? TKEY_CPU_FREQ_CASTOR_HZ
			       : TKEY_CPU_FREQ_BELLATRIX_HZ;
	*timer = delay_s;

	// Start timer
	*timer_ctrl |= (1 << TK1_MMIO_TIMER_CTRL_START_BIT);

	// Wait until timer hits 0.
	while (*timer_status != 0) {
	}

	// Stop timer
	*timer_ctrl |= (1 << TK1_MMIO_TIMER_CTRL_STOP_BIT);
}
