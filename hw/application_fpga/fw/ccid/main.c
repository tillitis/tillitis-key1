/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

#include "../tk1/proto.h"

#include "ccid.h"

int main(void)
{
	uint8_t available = 0;
	uint8_t cmdbuf[BUFSIZE] = { 0 };
	enum ioend endpoint = IO_NONE;
	led_set(LED_BLUE);

	config_endpoints(IO_CCID | IO_DEBUG);

	while(1) {

		memset(cmdbuf, 0, BUFSIZE);

		// Wait for data
		if (readselect(IO_DEBUG | IO_CH552 | IO_CCID, &endpoint, &available) < 0) {
			assert(1 == 2);
		}

		if (endpoint == IO_CCID) {

			if (read(IO_CCID, cmdbuf, BUFSIZE, available) < 0) {
				assert(1 == 2);
			}
			handle_ccid(cmdbuf, available);

		}
		else if (endpoint == IO_DEBUG) {

			if (read(IO_DEBUG, cmdbuf, BUFSIZE, available) < 0) {
				assert(1 == 2);
			}

			switch (cmdbuf[0]) {
				case 'c': {
					// ccid
					if (cmdbuf[1] ==  'c' && \
						cmdbuf[2] ==  'i' && \
						cmdbuf[3] ==  'd') {
						write(IO_CCID, cmdbuf + 5, available - 5);
					}
					// cdc
					else if (cmdbuf[1] ==  'd' && \
							 cmdbuf[2] ==  'c') {
						write(IO_CDC, cmdbuf + 4, available - 4);
					}
					// ch552
					else if (cmdbuf[1] ==  'h' && \
							 cmdbuf[2] ==  '5' && \
							 cmdbuf[3] ==  '5' && \
							 cmdbuf[4] ==  '2') {
						write(IO_CH552, cmdbuf + 6, available - 6);
					}
				}
				break;

				case 'd': {
					// debug
					if (cmdbuf[1] ==  'e' && \
						cmdbuf[2] ==  'b' && \
						cmdbuf[3] ==  'u' && \
						cmdbuf[4] ==  'g') {
						write(IO_DEBUG, cmdbuf + 6, available - 6);
					}
				}
				break;

				case 'f': {
					// fido
					if (cmdbuf[1] ==  'i' && \
						cmdbuf[2] ==  'd' && \
						cmdbuf[3] ==  'o') {
						write(IO_FIDO, cmdbuf + 5, available - 5);
					}
				}
				break;

				case 'b': {
					// bad
					if (cmdbuf[1] ==  'a' && \
						cmdbuf[2] ==  'd') {
						write(0xFF, cmdbuf + 4, available - 4);
					}
				}
				break;

				default: {
				}
				break;
			}

		} else if (endpoint == IO_CH552) {

			if (read(IO_CH552, cmdbuf, BUFSIZE, available) < 0) {
				assert(1 == 2);
			}
			write(IO_DEBUG, cmdbuf, available);
		}
	}
}
