// SPDX-FileCopyrightText: 2026 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

// Non-returning application handoff.
// Before calling, set TK1_MMIO_TK1_APP_ADDR  to app start, TK1_RAM_BASE.
//
// - Enables syscalls
// - Clear firmware stack in FW_RAM (not .BSS, resetinfo or .data)
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

    // TK1_MMIO_TK1_APP_ADDR = 0xff000030.
    lui a0, 0xff000
    lw a0, 0x030(a0)
    jalr zero, 0(a0)
