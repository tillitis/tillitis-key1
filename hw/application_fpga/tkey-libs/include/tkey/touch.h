// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_TOUCH_H
#define TKEY_TOUCH_H

#include <stdbool.h>
#include <stdint.h>
#include <tkey/tk1_mem.h>

// touchwait() waits for a touch event while blinking color on the
// status LED. timeout_s is the timeout in seconds.
//
// If a touch event occurs it returns true. If the timeout expires it
// returns false.
//
// This function uses the timer module and will override any existing timer
// configuration.
bool touch_wait(int color, int timeout_s);

#endif
