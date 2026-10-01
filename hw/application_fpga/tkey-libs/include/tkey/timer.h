// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_TIMER_H
#define TKEY_TIMER_H

// timer_wait() blocks for delay_s seconds before returning.
//
// This function uses the timer module and will override any existing timer
// configuration.
void timer_wait(int delay_s);

#endif
