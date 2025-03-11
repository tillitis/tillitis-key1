// SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/blake2s.h>
#include <tkey/tk1_mem.h>

int blake2s(void *out, unsigned long outlen, const void *key,
	    unsigned long keylen, const void *in, unsigned long inlen,
	    blake2s_ctx *ctx)
{
	// Not implemented.
	return -1;
}
