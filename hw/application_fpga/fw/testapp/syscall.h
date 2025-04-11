// SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>

#ifndef TKEY_APP_SYSCALL_H
#define TKEY_APP_SYSCALL_H

int syscall(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif
