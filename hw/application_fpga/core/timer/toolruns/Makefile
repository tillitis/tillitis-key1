#===================================================================
#
# Makefile
# --------
# Makefile for building the timer core and top simulations.
#
#
# Author: Joachim Strombergson
# Copyright (C) 2022 - Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only
#
#===================================================================

CORE_SRC=../rtl/timer_core.v
TB_CORE_SRC =../tb/tb_timer_core.v

TOP_SRC=../rtl/timer.v $(CORE_SRC)
TB_TOP_SRC =../tb/tb_timer.v

CC = iverilog
CC_FLAGS = -Wall

LINT = verilator
LINT_FLAGS = +1364-2001ext+ --lint-only  -Wall -Wno-fatal -Wno-DECLFILENAME


all: top.sim core.sim


top.sim: $(TB_TOP_SRC) $(TOP_SRC)
	$(CC) $(CC_FLAGS) -o top.sim $(TB_TOP_SRC) $(TOP_SRC)


core.sim: $(TB_CORE_SRC) $(CORE_SRC)
	$(CC) $(CC_FLAGS) -o core.sim $(TB_CORE_SRC) $(CORE_SRC)


sim-top: top.sim
	./top.sim


sim-core: core.sim
	./core.sim


lint-core:  $(CORE_SRC)
	$(LINT) $(LINT_FLAGS) $(CORE_SRC)


lint-top:  $(TOP_SRC)
	$(LINT) $(LINT_FLAGS) $(TOP_SRC)


clean:
	rm -f top.sim
	rm -f core.sim


help:
	@echo "Build system for simulation of timer core"
	@echo ""
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all:          Build all simulation targets."
	@echo "top.sim:      Build top level simulation target."
	@echo "core.sim:     Build core level simulation target."
	@echo "sim-top:      Run top level simulation."
	@echo "sim-core:     Run core level simulation."
	@echo "lint-core:    Lint core rtl source files."
	@echo "lint-core:    Lint top rtl source files."
	@echo "clean:        Delete all built files."

#===================================================================
# EOF Makefile
#===================================================================
