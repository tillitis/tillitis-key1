#===================================================================
#
# Makefile
# --------
# Makefile for building the TK1 core simulation target.
#
#
# Author: Joachim Strombergson
# Copyright (C) 2023 - Tillitis AB
# SPDX-License-Identifier: BSD-2-Clause
#
#===================================================================

SPI_SRC=../rtl/tk1_spi_master.v
TB_SPI_SRC =../tb/tb_tk1_spi_master.v
MEM_MODEL_SRC =../tb/W25Q80DL.v

TOP_SRC=../rtl/tk1.v $(SPI_SRC)
TB_TOP_SRC =../tb/tb_tk1.v ../tb/sb_rgba_drv_sim.v ../tb/udi_rom_sim.v
LINT_SRC=$(TOP_SRC) ../tb/sb_rgba_drv_sim.v ../tb/udi_rom_sim.v

CC = iverilog
CC_FLAGS = -Wall

LINT = verilator
LINT_FLAGS = +1364-2005ext+ --lint-only  -Wall -Wno-fatal -Wno-DECLFILENAME


all: MEM.TXT spi.sim top.sim


MEM.TXT:
	../tools/mem_gen.py > MEM.TXT


spi.sim: $(TB_SPI_SRC) $(SPI_SRC) $(MEM_MODEL_SRC)
	$(CC) $(CC_FLAGS) -o spi.sim $^


top.sim: $(TB_TOP_SRC) $(TOP_SRC)
	$(CC) $(CC_FLAGS) -o top.sim $^ -DUDI_HEX=\"../tb/udi.hex\"


sim-spi: spi.sim
	./spi.sim


sim-top: top.sim
	./top.sim


lint-top:  $(LINT_SRC)
	$(LINT) $(LINT_FLAGS) $^ -DUDI_HEX=\"../tb/udi.hex\"


clean:
	rm -f spi.sim
	rm -f top.sim
	rm -f MEM.TXT

help:
	@echo "Build system for simulation of TK1 core"
	@echo ""
	@echo "Supported targets:"
	@echo "------------------"
	@echo "spi.sim:      Build SPI simulation target."
	@echo "sim-spi:      Run SPI simulation."
	@echo "top.sim:      Build top level simulation target."
	@echo "sim-top:      Run top level simulation."
	@echo "lint-top:     Lint top rtl source files."
	@echo "clean:        Delete all built files."

#===================================================================
# EOF Makefile
#===================================================================
