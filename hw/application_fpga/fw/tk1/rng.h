// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only
#ifndef RNG_H
#define RNG_H

#include <stdint.h>

uint32_t rng_get_word(void);
uint32_t rng_xorwow(uint32_t state, uint32_t acc);

#endif
