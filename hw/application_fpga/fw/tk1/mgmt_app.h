// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef MGMT_APP_H
#define MGMT_APP_H

#include <stdbool.h>
#include <stdint.h>

int mgmt_app_init(uint8_t app_digest[32]);
bool mgmt_app_authenticate(void);
uint8_t *mgmt_app_allowed_digest(void);

#endif
