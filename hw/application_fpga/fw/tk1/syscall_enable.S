// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: GPL-2.0-only

#ifdef QEMU_SYSCALL

#define picorv32_maskirq_insn(...)

#else

#include "../tk1/picorv32/custom_ops.S"

#endif

	.section ".text"
	.globl syscall_enable


syscall_enable:
	// Enable syscall IRQ
	li t0, 0x7fffffff // IRQ31 mask
	picorv32_maskirq_insn(zero, t0) // Enable IRQs

	ret
