// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#ifndef MEMCHECK_H
#define MEMCHECK_H

#include <stdbool.h>
#include <stddef.h>

bool in_app_ram(const void *p, size_t size);

#endif
