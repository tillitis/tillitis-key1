// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TKEY_APP_SYSCALL_H
#define TKEY_APP_SYSCALL_H

#include <stdint.h>

int syscall(uint32_t number, uint32_t arg1);

#endif
