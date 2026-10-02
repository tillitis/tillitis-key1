// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_KEYS_H
#define TKEY_KEYS_H

#include <stdbool.h>
#include <stdint.h>

bool keys_is_init(void);
void keys_generate(uint8_t *cdi_key, uint8_t *part_key);

#endif
