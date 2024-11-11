// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "syscall.h"
#include "htif.h"
#include "mgmt_app.h"
#include "partition_table.h"
#include "preload_app.h"
#include "storage.h"

#include <stdint.h>

#define NO_FW_RAM

void inner_syscall(volatile syscall_t *ctx);

void syscall(syscall_t *ctx)
{

#ifdef NO_FW_RAM
	asm volatile("addi x5, sp, 0;"	  // Save current SP value in x5
		     "li sp, 0xd0000800;" // Change SP to top of FW_RAM
		     "addi sp, sp, -4;"	  // Adjust SP to make space
		     "sw x5, 0(sp);"	  // Store originally saved SP value
				     // in new stack
		     :			// No outputs
		     :			// No inputs
		     : "x5", "memory"); // Clobbers
#endif //NO_FW_RAM

	inner_syscall(ctx);

#ifdef NO_FW_RAM
	asm volatile("lui  t0, 0xd0000;"   // Load the upper 20 bits
		     "addi t0, t0, 0x7fc;" // Add the lower 12 bits for full
		     "lw t1, 0(t0);"  // Load the word at address in t0 to t1
		     "addi sp,t1, 0;" // Copy the value from t1 to sp
		     :		      // No outputs
		     :		      // No inputs
		     : "t0", "t1", "memory"); // Clobbers
#endif //NO_FW_RAM

	return;
}

void __attribute__((noinline)) inner_syscall(volatile syscall_t *ctx_local)
{
	partition_table_t part_table = {0x00};
	htif_putinthex((uint32_t)&part_table);
	htif_lf();

	part_table_read(&part_table);
	htif_hexdump(&part_table, sizeof(part_table));
	htif_lf();

	ctx_local->ret_value = -1;

	switch (ctx_local->syscall_no) {
	case ALLOC_AREA:
		ctx_local->ret_value = storage_allocate_area(&part_table);
		break;

	case DEALLOC_AREA:
		ctx_local->ret_value = storage_deallocate_area(&part_table);
		break;

	case READ_DATA:
		ctx_local->ret_value =
		    storage_read_data(&part_table, ctx_local->offset,
				      ctx_local->data, ctx_local->size);
		break;

	case WRITE_DATA:
		ctx_local->ret_value =
		    storage_write_data(&part_table, ctx_local->offset,
				       ctx_local->data, ctx_local->size);
		break;

	case ERASE_DATA:
		ctx_local->ret_value = storage_erase_sector(
		    &part_table, ctx_local->offset, ctx_local->size);
		break;

	case PRELOAD_STORE:
		ctx_local->ret_value =
		    preload_store(&part_table, ctx_local->offset,
				  ctx_local->data, ctx_local->size);
		break;

	case PRELOAD_STORE_FINALIZE:
		ctx_local->ret_value =
		    preload_store_finalize(&part_table, ctx_local->offset,
					   ctx_local->data, ctx_local->size);
		break;

	case PRELOAD_DELETE:
		ctx_local->ret_value = preload_delete(&part_table);
		break;

	case MGMT_APP_REGISTER:
		ctx_local->ret_value = mgmt_app_register(&part_table);
		break;

	case MGMT_APP_UNREGISTER:
		ctx_local->ret_value = mgmt_app_unregister(&part_table);
		break;

	default:
		/* return -1 */
		break;
	}

	return;
}
