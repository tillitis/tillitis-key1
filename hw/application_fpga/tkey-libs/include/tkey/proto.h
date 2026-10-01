// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>

#ifndef TKEY_PROTO_H
#define TKEY_PROTO_H

enum frame_domain {
	DST_HW_IFPGA = 0x00,
	DST_HW_AFPGA = 0x01,
	DST_FW = 0x02,
	DST_SW = 0x03
};

enum frame_cmdlen {
	LEN_1,
	LEN_4,
	LEN_32,
	LEN_128
};

#define CMDLEN_MAXBYTES 128

enum frame_status {
	FRAME_STATUS_OK,
	FRAME_STATUS_NOK
};

struct frame_header {
	uint8_t id;
	enum frame_domain f_domain;
	size_t len;
};

int frame_gen_hdr(uint8_t id, uint8_t frame_domain, uint8_t status,
		  size_t nbytes, uint8_t *header);
int frame_parse_hdr(uint8_t b, struct frame_header *hdr);
int frame_read(uint8_t *buf, size_t bufsize, struct frame_header *hdr);
int frame_write(uint8_t status, uint8_t id, enum frame_domain f_domain,
		uint8_t *buf, size_t nbytes);
#endif
