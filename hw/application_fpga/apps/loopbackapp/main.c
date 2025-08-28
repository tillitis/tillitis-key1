// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>

#define BUFSIZE 256
#define HEADER_SIZE 2
#define HID_PACKET_SIZE 64
#define MAX_PAYLOAD_SIZE 64
#define SLEEPTIME 100000

void sleep(uint32_t n)
{
	for (volatile int i = 0; i < n; i++)
		;
}

int main(void)
{
	uint8_t available = 0;
	uint8_t cmdbuf[BUFSIZE] = {0};
	enum ioend endpoint = IO_NONE;
	bool waiting_for_more_data = false;
	uint8_t dest_endpoint = IO_NONE;
	uint8_t payload_length = 0;
	uint8_t payload_left = 0;
	uint8_t payloadbuf[MAX_PAYLOAD_SIZE] = {0};
	uint8_t first_packet_len = HID_PACKET_SIZE - HEADER_SIZE;
	uint8_t endpoints = IO_DEBUG;
	led_set(LED_BLUE);

	// Give time to CH552 to settle USB enumeration at power on
	sleep(SLEEPTIME * 5);

	// Setup available endpoints
	endpoints |= IO_CDC | IO_FIDO;
	config_endpoints(endpoints);

	while (1) {
		// Wait for data
		if (readselect(endpoints, &endpoint, &available) != 0) {
			assert(1 == 2);
		}

		switch (endpoint) {
		case IO_CDC:
			cmdbuf[0] = IO_CDC;
			cmdbuf[1] = available;
			if (read(IO_CDC, cmdbuf + 2, BUFSIZE - 2, available) <
			    0) {
				assert(1 == 2);
			}
			write(IO_DEBUG, cmdbuf, available + 2);
			memset(cmdbuf, 0, BUFSIZE);
			break;

		case IO_FIDO:
			cmdbuf[0] = IO_FIDO;
			cmdbuf[1] = available;
			if (read(IO_FIDO, cmdbuf + 2, BUFSIZE - 2, available) <
			    0) {
				assert(1 == 2);
			}
			write(IO_DEBUG, cmdbuf, available + 2);
			memset(cmdbuf, 0, BUFSIZE);
			break;

		case IO_CCID:
			cmdbuf[0] = IO_CCID;
			cmdbuf[1] = available;
			if (read(IO_CCID, cmdbuf + 2, BUFSIZE - 2, available) <
			    0) {
				assert(1 == 2);
			}
			write(IO_DEBUG, cmdbuf, available + 2);
			memset(cmdbuf, 0, BUFSIZE);
			break;

		case IO_DEBUG:
			if (available != HID_PACKET_SIZE) {
				led_set(LED_RED);
				while (1)
					;
			}

			if (read(IO_DEBUG, cmdbuf, BUFSIZE, available) < 0) {
				assert(1 == 2);
			}

			if (!waiting_for_more_data) {
				dest_endpoint = cmdbuf[0];
				payload_length = cmdbuf[1];

				// Check that destination endpoint is ok
				if (dest_endpoint != IO_CDC &&
				    dest_endpoint != IO_FIDO &&
				    dest_endpoint != IO_CCID) {
					led_set(LED_RED);
					while (1)
						;
				}

				// Check payload size
				if (payload_length > MAX_PAYLOAD_SIZE) {
					led_set(LED_RED);
					while (1)
						;
				}

				// Complete payload fits in this packet
				if (payload_length <= first_packet_len) {
					write(dest_endpoint, cmdbuf + 2,
					      payload_length);
					memset(cmdbuf, 0, BUFSIZE);
				} else { // More payload will come in next
					 // packet
					memcpy(payloadbuf, cmdbuf + 2,
					       first_packet_len);
					payload_left =
					    payload_length - first_packet_len;
					waiting_for_more_data = true;
				}
			} else {
				memcpy(payloadbuf + first_packet_len, cmdbuf,
				       payload_left);
				write(dest_endpoint, payloadbuf,
				      payload_length);
				memset(payloadbuf, 0, MAX_PAYLOAD_SIZE);
				memset(cmdbuf, 0, BUFSIZE);
				waiting_for_more_data = false;
			}
			break;

		default:
			break;
		}
	}
}
