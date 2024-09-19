// Copyright (C) 2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include "syscall.h"
#include "mgmt_app.h"
#include "partition_table.h"
#include "preload_app.h"
#include "storage.h"

#include <stdint.h>

int syscall(syscall_t *ctx)
{

	partition_table_t part_table;
	part_table_read(&part_table);

	switch (ctx->syscall_no) {
	case ALLOC_AREA:
		return storage_allocate_area(&part_table);
		break;

	case DEALLOC_AREA:
		return storage_deallocate_area(&part_table);
		break;

	case READ_DATA:
		return storage_read_data(&part_table, ctx->offset, ctx->data,
					 ctx->size);
		break;

	case WRITE_DATA:
		return storage_write_data(&part_table, ctx->offset, ctx->data,
					  ctx->size);
		break;

	case PRELOAD_STORE:
		return preload_store(&part_table, ctx->offset, ctx->data,
				     ctx->size);
		break;

	case PRELOAD_STORE_FINALIZE:
		return preload_store_finalize(&part_table, ctx->offset,
					      ctx->data, ctx->size);
		break;

	case PRELOAD_DELETE:
		return preload_delete(&part_table);
		break;

	case MGMT_APP_REGISTER:
		return mgmt_app_register(&part_table);
		break;

	case MGMT_APP_UNREGISTER:
		return mgmt_app_unregister(&part_table);
		break;

	default:
		/* return -1 */
		break;
	}
	return -1;
}
