/*
 * Copyright (C) 2022 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "types.h"

#ifndef PROTO_H
#define PROTO_H

enum endpoints {
	DST_HW_IFPGA,
	DST_HW_AFPGA,
	DST_FW,
	DST_SW
};

enum cmdlen {
	LEN_1,
	LEN_4,
	LEN_32,
	LEN_128
};

#define CMDLEN_MAXBYTES 128

// clang-format off
enum fwcmd {
	FW_CMD_NAME_VERSION	= 0x01,
	FW_RSP_NAME_VERSION	= 0x02,
	FW_CMD_LOAD_APP_SIZE	= 0x03,
	FW_RSP_LOAD_APP_SIZE	= 0x04,
	FW_CMD_LOAD_APP_DATA	= 0x05,
	FW_RSP_LOAD_APP_DATA	= 0x06,
	FW_CMD_RUN_APP		= 0x07,
	FW_RSP_RUN_APP		= 0x08,
	FW_CMD_GET_APP_DIGEST	= 0x09,
	FW_RSP_GET_APP_DIGEST	= 0x10
};
// clang-format on

enum status {
	STATUS_OK,
	STATUS_BAD
};

struct frame_header {
	uint8_t id;
	enum endpoints endpoint;
	enum cmdlen len;
};

uint8_t genhdr(uint8_t id, uint8_t endpoint, uint8_t status, enum cmdlen len);
int parseframe(uint8_t b, struct frame_header *hdr);
void fwreply(struct frame_header hdr, enum fwcmd rspcode, uint8_t *buf);
void writebyte(uint8_t b);
void write(uint8_t *buf, size_t nbytes);
uint8_t readbyte();
uint8_t readbyte_ledflash(int ledvalue, int loopcount);
void read(uint8_t *buf, size_t nbytes);

#endif
