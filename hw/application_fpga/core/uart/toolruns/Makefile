#===================================================================
#
# Makefile
# --------
# Makefile for building the UART core.
#
#
# Author: Joachim Strombergson
# Copyright (C) 2022 - Tillitis AB
# SPDX-License-Identifier: BSD-2-Clause
#
#===================================================================

TOP_SRC=../rtl/uart.v ../rtl/uart_core.v ../rtl/uart_fifo.v
TB_TOP_SRC =../tb/tb_uart.v

CC = iverilog
CC_FLAGS = -Wall

LINT = verilator
LINT_FLAGS = +1364-2005ext+ --lint-only  -Wall -Wno-fatal -Wno-DECLFILENAME


all: top.sim


top.sim: $(TB_TOP_SRC) $(TOP_SRC)
	$(CC) $(CC_FLAGS) -o top.sim $(TB_TOP_SRC) $(TOP_SRC)


sim-top: top.sim
	./top.sim


lint-top:  $(TOP_SRC)
	$(LINT) $(LINT_FLAGS) $(TOP_SRC)


clean:
	rm -f top.sim


help:
	@echo "Build system for simulation of UART core"
	@echo ""
	@echo "Supported targets:"
	@echo "------------------"
	@echo "top.sim:      Build top level simulation target."
	@echo "sim-top:      Run top level simulation."
	@echo "lint-top:     Lint top rtl source files."
	@echo "clean:        Delete all built files."

#===================================================================
# EOF Makefile
#===================================================================
