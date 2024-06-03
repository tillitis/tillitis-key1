#===================================================================
#
# Makefile
# --------
# Makefile for building the TK1 core simulation target.
#
#
# Author: Joachim Strombergson
# Copyright (C) 2023 - Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only
#
#===================================================================

TOP_SRC=../rtl/tk1.v
TB_TOP_SRC =../tb/tb_tk1.v ../tb/sb_rgba_drv.v ../tb/udi_rom_sim.v

CC = iverilog
CC_FLAGS = -Wall

LINT = verilator
LINT_FLAGS = +1364-2005ext+ --lint-only  -Wall -Wno-fatal -Wno-DECLFILENAME


all: top.sim


top.sim: $(TB_TOP_SRC) $(TOP_SRC)
	$(CC) $(CC_FLAGS) -o top.sim $(TB_TOP_SRC) $(TOP_SRC) -DUDI_HEX=\"../tb/udi.hex\"


sim-top: top.sim
	./top.sim


lint-top:  $(TOP_SRC)
	$(LINT) $(LINT_FLAGS) $(TOP_SRC)


clean:
	rm -f top.sim


help:
	@echo "Build system for simulation of TK1 core"
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
