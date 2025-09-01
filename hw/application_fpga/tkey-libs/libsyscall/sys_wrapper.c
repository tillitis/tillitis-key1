// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <tkey/syscall.h>

// Reset the TKey. Leave the reset type (enum reset_start) in rst as
// well as an optional app_digest, forcing firmware to only allow that
// specific app digest, as well as some data to leave to the next app
// in chain in next_app_data. Send the length of the next_app_data in len.
//
// The TKey is reset and firmware starts again.
//
// Returns non-zero on error.
int sys_reset(struct reset *rst, size_t len)
{
	return syscall(TK1_SYSCALL_RESET, (uint32_t)rst, len, 0);
}

// Fills in data left from previous app in the chain into
// `next_app_data`. Buffer needs to be large enough to receive
// RESET_DATA_SIZE bytes.
//
// Returns 0 on success.
int sys_reset_data(uint8_t next_app_data[RESET_DATA_SIZE])
{
	return syscall(TK1_SYSCALL_GET_APP_DATA, (uint32_t)next_app_data, 0, 0);
}

// Allocate a flash area for the current app. Must be done before sys_write()
// or sys_read(). If the current app already has an area allocated no new
// area will be allocated.
//
// Returns 0 on success.
int sys_alloc(void)
{
	return syscall(TK1_SYSCALL_ALLOC_AREA, 0, 0, 0);
}

// Free an already allocated flash area for the current app.
//
// Returns 0 on success.
int sys_dealloc(void)
{
	return syscall(TK1_SYSCALL_DEALLOC_AREA, 0, 0, 0);
}

// Write data in `buf` to the app's flash area at byte `offset` within
// the area.
//
// Up to storage area size bytes can be written at once and `offset` must be a
// multiple of 256 bytes.
//
// Returns 0 on success.
int sys_write(uint32_t offset, void *buf, size_t len)
{
	return syscall(TK1_SYSCALL_WRITE_DATA, offset, (uint32_t)buf, len);
}

// Read `len` bytes into `buf` at byte `offset` from the app's flash
// area.
//
// Returns 0 on success.
int sys_read(uint32_t offset, void *buf, size_t len)
{
	return syscall(TK1_SYSCALL_READ_DATA, offset, (uint32_t)buf, len);
}

// Erase `len` bytes from `offset` within the area.
//
// Both `len` and  `offset` must be a multiple of 4096 bytes.
//
// Returns 0 on success.
int sys_erase(uint32_t offset, size_t len)
{
	return syscall(TK1_SYSCALL_ERASE_DATA, offset, len, 0);
}

// Returns the TKey Vendor and Product ID.
int sys_get_vidpid(void)
{
	return syscall(TK1_SYSCALL_GET_VIDPID, 0, 0, 0);
}

// Delete the app in flash slot 1. Only available for the verified
// management app.
//
// Returns 0 on success.
int sys_preload_delete(void)
{
	return syscall(TK1_SYSCALL_PRELOAD_DELETE, 0, 0, 0);
}

// Store an app, or possibly just a block of an app, from the `app`
// buffer in flash slot 1 at byte `offset`.
//
// If you can't fit your entire app in the buffer, call
// `sys_preload_store` many times as you receive the binary from the
// client. Returns 0 on success.
//
// Up to preloaded app area size bytes can be written at once and `offset` must
// be a multiple of 256 bytes.
//
// Only available for the verified management app.
//
// Returns 0 on success.
int sys_preload_store(uint32_t offset, void *app, size_t len)
{
	return syscall(TK1_SYSCALL_PRELOAD_STORE, offset, (uint32_t)app, len);
}

// Finalize storing of an app where the complete binary size is `len`
// in flash slot 1. Returns 0 on success. Only available for the
// verified management app.
//
// Compute a BLAKE2s hash digest over the entire binary. Pass the
// result in `app_digest`.
//
// Sign `app_digest` with your Ed25519 private key and pass the
// resulting signature in `app_signature`.
//
// Returns 0 on success.
int sys_preload_store_fin(size_t len, uint8_t digest[32], uint8_t signature[64])
{
	return syscall(TK1_SYSCALL_PRELOAD_STORE_FIN, len, (uint32_t)digest,
		       (uint32_t)signature);
}

// Copies the digest and signature of app in flash slot 1 to
// `app_digest` and `app_signature`. Returns 0 on success. Only
// available for the verified management app.
//
// Returns 0 on success.
int sys_get_digsig(uint8_t digest[32], uint8_t signature[64])
{
	return syscall(TK1_SYSCALL_PRELOAD_GET_DIGSIG, (uint32_t)digest,
		       (uint32_t)signature, 0);
}

// Returns filesystem status. Non-zero when problems have been
// detected, so far only that the first copy of the partition table
// didn't pass checks.
int sys_status(void)
{
	return syscall(TK1_SYSCALL_STATUS, 0, 0, 0);
}
