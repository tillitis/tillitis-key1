// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

// Non-returning application handoff.
// Before calling, set TK1_MMIO_TK1_APP_ADDR  to app start, TK1_RAM_BASE.
//
// - Enables syscalls
// - Clear firmware stack in FW_RAM (not .BSS, resetinfo or .data)
// - Clear CPU registers
// - Jump to address in TK1_MMIO_TK1_APP_ADDR, start of application

    .section .text
    .globl app_handoff
    .type app_handoff, @function

app_handoff:
    call syscall_enable

    la a0, _sstack
    la a1, _estack
clear_stack:
    sw zero, 0(a0)
    addi a0, a0, 4
    bltu a0, a1, clear_stack 

clear_registers:
    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10, 0
    li x11, 0
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    li x16, 0
    li x17, 0
    li x18, 0
    li x19, 0
    li x20, 0
    li x21, 0
    li x22, 0
    li x23, 0
    li x24, 0
    li x25, 0
    li x26, 0
    li x27, 0
    li x28, 0
    li x29, 0
    li x30, 0
    li x31, 0

    // TK1_MMIO_TK1_APP_ADDR = 0xff000030.
    lui a0, 0xff000
    lw a0, 0x030(a0)
    jalr zero, 0(a0)
