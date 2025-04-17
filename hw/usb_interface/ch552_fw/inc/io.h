// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: MIT

#ifndef __IO_H__
#define __IO_H__

enum ioend {
    IO_NONE = 0x00,   // No endpoint
    IO_UART = 0x01,   // Only destination, raw UART access
    IO_QEMU = 0x02,   // Only destination, QEMU debug port
    IO_CH552 = 0x04,  // Internal CH552 control port
    IO_CDC = 0x08,    // CDC "serial" port
    IO_FIDO = 0x10,   // FIDO security token port
    IO_CCID = 0x20,   // CCID "smart card" port
    IO_DEBUG = 0x40,  // Debug port over USB HID
};

enum ch552cmd {
    SET_ENDPOINTS = 0x01, // Config USB endpoints on the CH552
    CH552_CMD_MAX,
};

#endif
