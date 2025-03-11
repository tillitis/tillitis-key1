/*
 * Tillitis TKey Memory Map
 *
 * SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Note that this file is also in tillitis-key1 and qemu
 * (GPL-2.0-or-later). Needs to stay in sync and have a compatible
 * license.
 */

// clang-format off

#ifndef TKEY_TK1_MEM_H
#define TKEY_TK1_MEM_H

/*
  The canonical location of this file is in:

    https://github.com/tillitis/tkey-libs

  Under:

    include/tkey/tk1_mem.h

  The contents are mostly derived from the Verilog code in

    https://github.com/tillitis/tillitis-key1

  Memory map

  Top level prefix, the first 2 bits in a 32-bit address:

  name	prefix	address length
  --------------------------------------------------------
  ROM		0b00	30 bit address
  RAM		0b01	30 bit address
  Reserved	0b10
  Cores		0b11	6 bits for core select, 24 bits rest

  Address Prefix, the first 8 bits in a 32-bit address:

  name		prefix
  --------------------
  ROM		0x00
  RAM		0x40
  TRNG		0xc0
  TIMER		0xc1
  UDS		0xc2
  UART		0xc3
  TOUCH		0xc4
  FW_RAM	0xd0
  QEMU		0xfe   Not used in real hardware
  TK1		0xff
 */

#define TK1_ROM_BASE 0x00000000
#define TK1_ROM_SIZE 0x2000

#define TK1_RAM_BASE 0x40000000
#define TK1_RAM_SIZE 0x20000

#define TK1_MMIO_BASE 0xc0000000
#define TK1_MMIO_SIZE 0x3fffffff

#define TK1_APP_MAX_SIZE 0x20000

#define TK1_MMIO_FW_RAM_BASE 0xd0000000
// FW_RAM is 4096 bytes
#define TK1_MMIO_FW_RAM_SIZE 0x1000

#define TK1_MMIO_TRNG_BASE 0xc0000000
#define TK1_MMIO_TRNG_STATUS 0xc0000024
#define TK1_MMIO_TRNG_STATUS_READY_BIT 0
#define TK1_MMIO_TRNG_ENTROPY 0xc0000080

#define TK1_MMIO_TIMER_BASE 0xc1000000
#define TK1_MMIO_TIMER_CTRL 0xc1000020
#define TK1_MMIO_TIMER_CTRL_START_BIT 0
#define TK1_MMIO_TIMER_CTRL_STOP_BIT 1

#define TK1_MMIO_TIMER_STATUS 0xc1000024
#define TK1_MMIO_TIMER_STATUS_RUNNING_BIT 0
#define TK1_MMIO_TIMER_PRESCALER 0xc1000028
#define TK1_MMIO_TIMER_TIMER 0xc100002c

#define TK1_MMIO_UDS_BASE 0xc2000000
#define TK1_MMIO_UDS_FIRST 0xc2000000
#define TK1_MMIO_UDS_LAST 0xc200001c

#define TK1_MMIO_UART_BASE 0xc3000000
#define TK1_MMIO_UART_RX_STATUS 0xc3000080
#define TK1_MMIO_UART_RX_DATA 0xc3000084
#define TK1_MMIO_UART_RX_BYTES 0xc3000088
#define TK1_MMIO_UART_TX_STATUS 0xc3000100
#define TK1_MMIO_UART_TX_DATA 0xc3000104

#define TK1_MMIO_TOUCH_BASE 0xc4000000
#define TK1_MMIO_TOUCH_STATUS 0xc4000024
#define TK1_MMIO_TOUCH_STATUS_EVENT_BIT 0

// This only exists in QEMU, not real hardware
#define TK1_MMIO_QEMU_BASE 0xfe000000
#define TK1_MMIO_QEMU_DEBUG 0xfe001000

#define TK1_MMIO_TK1_BASE 0xff000000

#define TK1_MMIO_TK1_NAME0 0xff000000
#define TK1_MMIO_TK1_NAME1 0xff000004
#define TK1_MMIO_TK1_VERSION 0xff000008

#define TK1_MMIO_TK1_LED 0xff000024
#define TK1_MMIO_TK1_LED_R_BIT 2
#define TK1_MMIO_TK1_LED_G_BIT 1
#define TK1_MMIO_TK1_LED_B_BIT 0

#define TK1_MMIO_TK1_GPIO 0xff000028
#define TK1_MMIO_TK1_GPIO1_BIT 0
#define TK1_MMIO_TK1_GPIO2_BIT 1
#define TK1_MMIO_TK1_GPIO3_BIT 2
#define TK1_MMIO_TK1_GPIO4_BIT 3

#define TK1_MMIO_TK1_APP_ADDR 0xff000030
#define TK1_MMIO_TK1_APP_SIZE 0xff000034

#define TK1_MMIO_TK1_CDI_FIRST 0xff000080
#define TK1_MMIO_TK1_CDI_LAST 0xff00009c

#define TK1_MMIO_TK1_UDI_FIRST 0xff0000c0
#define TK1_MMIO_TK1_UDI_LAST 0xff0000c4

// Deprecated - use _ADDR_RAND instead
#define TK1_MMIO_TK1_RAM_ASLR 0xff000100
#define TK1_MMIO_TK1_RAM_ADDR_RAND 0xff000100
// Deprecated - use _DATA_RAND instead
#define TK1_MMIO_TK1_RAM_SCRAMBLE 0xff000104
#define TK1_MMIO_TK1_RAM_DATA_RAND 0xff000104

#define TK1_MMIO_TK1_CPU_MON_CTRL 0xff000180
#define TK1_MMIO_TK1_CPU_MON_FIRST 0xff000184
#define TK1_MMIO_TK1_CPU_MON_LAST 0xff000188

#define TK1_MMIO_TK1_SYSTEM_RESET 0xff0001C0

#define TK1_MMIO_TK1_SPI_EN 0xff000200
#define TK1_MMIO_TK1_SPI_XFER 0xff000204
#define TK1_MMIO_TK1_SPI_DATA 0xff000208
#endif
