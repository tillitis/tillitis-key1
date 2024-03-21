/*
 * Tillitis TKey Memory Map
 *
 * Copyright (c) 2022, 2023, 2024 Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Note that this file is also included in at least qemu
 * (GPL-2.0-or-later) besides tillitis-key1 (GPL-2.0-only) and
 * tkey-libs (GPL-2.0-only) so it's licensed as GPL v2 or later.
 */

// clang-format off

#ifndef TKEY_TK1_MEM_H
#define TKEY_TK1_MEM_H

/*

  The canonical location of this file is in:

  https://github.com/tillitis/tillitis-key1

  /hw/application_fpga/fw/tk1_mem.h

  The contents are derived from the Verilog code. For use by QEMU model,
  firmware, and apps.

  Memory map

  Top level prefix, the first 2 bits in a 32-bit address:

  name	prefix	address length
  --------------------------------------------------------
  ROM		0b00	30 bit address
  RAM		0b01	30 bit address
  Reserved	0b10
  MMIO		0b11	6 bits for core select, 24 bits rest

  Address Prefix, the first 8 bits in a 32-bit address:

  name		prefix
  --------------------
  ROM		0x00
  RAM		0x40
  MMIO		0xc0
  MMIO TRNG	0xc0
  MMIO TIMER	0xc1
  MMIO UDS	0xc2
  MMIO UART	0xc3
  MMIO TOUCH	0xc4
  MMIO FW_RAM	0xd0
  MMIO QEMU	0xfe   Not used in real hardware
  MMIO TK1	0xff
 */

#define TK1_ROM_BASE 0x00000000
#define TK1_RAM_BASE 0x40000000
#define TK1_RAM_SIZE 0x20000

#define TK1_MMIO_BASE 0xc0000000
#define TK1_MMIO_SIZE 0x3fffffff

#define TK1_APP_MAX_SIZE 0x20000

#define TK1_MMIO_FW_RAM_BASE 0xd0000000
// FW_RAM is 2048 bytes
#define TK1_MMIO_FW_RAM_SIZE 0x800

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
#define TK1_MMIO_UDS_FIRST 0xc2000040
#define TK1_MMIO_UDS_LAST 0xc200005c

#define TK1_MMIO_UART_BASE 0xc3000000
#define TK1_MMIO_UART_BIT_RATE 0xc3000040
#define TK1_MMIO_UART_DATA_BITS 0xc3000044
#define TK1_MMIO_UART_STOP_BITS 0xc3000048
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

#define TK1_MMIO_TK1_SWITCH_APP 0xff000020

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

#define TK1_MMIO_TK1_BLAKE2S 0xff000040

#define TK1_MMIO_TK1_CDI_FIRST 0xff000080
#define TK1_MMIO_TK1_CDI_LAST 0xff00009c

#define TK1_MMIO_TK1_UDI_FIRST 0xff0000c0
#define TK1_MMIO_TK1_UDI_LAST 0xff0000c4

// Deprecated - use _ADDR_RAND instead
#define TK1_MMIO_TK1_RAM_ASLR 0xff000100
#define TK1_MMIO_TK1_RAM_ADDR_RAND 0xff000100
#define TK1_MMIO_TK1_RAM_SCRAMBLE 0xff000104

#define TK1_MMIO_TK1_CPU_MON_CTRL 0xff000180
#define TK1_MMIO_TK1_CPU_MON_FIRST 0xff000184
#define TK1_MMIO_TK1_CPU_MON_LAST 0xff000188
#endif
