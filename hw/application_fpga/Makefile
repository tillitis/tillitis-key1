#=======================================================================
#
# Makefile
# --------
# Makefile for building, simulating, running all application_fpga
# HW targets as well as its firmware.
#
#
# Copyright (C) 2022 - Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only
#
#=======================================================================

#-------------------------------------------------------------------
# Defines.
#-------------------------------------------------------------------
SHELL := /bin/bash
CUR_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
P := $(CUR_DIR)

YOSYS_PATH ?=
NEXTPNR_PATH ?=
ICESTORM_PATH ?=

# Size in 32-bit words, must be divisible by 256 (pairs of EBRs, because 16
# bits wide; an EBR is 128 32-bits words)
BRAM_FW_SIZE ?= 1536

PIN_FILE ?= application_fpga_mta1_usb_v1.pcf

SIZE ?= llvm-size-14
OBJCOPY ?= llvm-objcopy-14

CC = clang-14

CFLAGS = -target riscv32-unknown-none-elf -march=rv32imc -mabi=ilp32 \
   -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf \
   -fno-builtin-putchar -nostdlib -mno-relax -Wall -flto -DNOCONSOLE

AS = clang-14
ASFLAGS = -target riscv32-unknown-none-elf -march=rv32imc -mabi=ilp32 -mno-relax

ICE40_SIM_CELLS = $(shell yosys-config --datdir/ice40/cells_sim.v)


# FPGA specific Verilog source files.
FPGA_SRC = $(P)/rtl/application_fpga.v \
	   $(P)/rtl/clk_reset_gen.v

# Verilator simulation  specific Verilog source files.
VERILATOR_FPGA_SRC = $(P)/tb/application_fpga_vsim.v \
		     $(P)/tb/reset_gen_vsim.v

# Common verilog source files.
VERILOG_SRCS = \
	$(P)/rtl/ram.v \
	$(P)/rtl/rom.v \
	$(P)/core/picorv32/rtl/picorv32.v \
	$(P)/core/timer/rtl/timer_core.v \
	$(P)/core/timer/rtl/timer.v \
	$(P)/core/uds/rtl/uds.v \
	$(P)/core/touch_sense/rtl/touch_sense.v \
	$(P)/core/mta1/rtl/mta1.v \
	$(P)/core/uart/rtl/uart_core.v \
	$(P)/core/uart/rtl/uart_fifo.v \
	$(P)/core/uart/rtl/uart.v \
	$(P)/core/trng/rtl/rosc.v

FIRMWARE_DEPS = \
	$(P)/fw/mta1_mkdf_mem.h \
	$(P)/fw/mta1_mkdf/types.h \
	$(P)/fw/mta1_mkdf/lib.h \
	$(P)/fw/mta1_mkdf/proto.h

FIRMWARE_OBJS = \
	$(P)/fw/mta1_mkdf/main.o \
	$(P)/fw/mta1_mkdf/start.o \
	$(P)/fw/mta1_mkdf/proto.o \
	$(P)/fw/mta1_mkdf/lib.o \
	$(P)/fw/mta1_mkdf/blake2s/blake2s.o

TESTFW_OBJS = \
	$(P)/fw/testfw/main.o \
	$(P)/fw/mta1_mkdf/start.o \
	$(P)/fw/mta1_mkdf/proto.o \
	$(P)/fw/mta1_mkdf/lib.o

#-------------------------------------------------------------------
# All: Complete build of HW and FW.
#-------------------------------------------------------------------
all: application_fpga.bin
.PHONY: all

#-------------------------------------------------------------------
# The size_mismatch target make sure that we don't end up with an
# incorrect BRAM_FW_SIZE
# -------------------------------------------------------------------
size_mismatch: firmware.elf
	@test $$($(SIZE) $< | awk 'NR==2{print $$4}') -le $$(( 32 / 8 * $(BRAM_FW_SIZE) )) || \
	(echo "The 'BRAM_FW_SIZE' variable needs to be increased" && false)
.PHONY: size_mismatch


#-------------------------------------------------------------------
# Firmware generation.
# Included in the bitstream.
#-------------------------------------------------------------------
LDFLAGS=-T $(P)/fw/mta1_mkdf/firmware.lds

$(FIRMWARE_OBJS): $(FIRMWARE_DEPS)
$(TESTFW_OBJS): $(FIRMWARE_DEPS)

firmware.elf: $(FIRMWARE_OBJS) $(P)/fw/mta1_mkdf/firmware.lds
	$(CC) $(CFLAGS) $(FIRMWARE_OBJS) $(LDFLAGS) -o $@

testfw.elf: $(TESTFW_OBJS) $(P)/fw/mta1_mkdf/firmware.lds
	$(CC) $(CFLAGS) $(TESTFW_OBJS) $(LDFLAGS) -o $@

# Generate a fake BRAM file that will be filled in later after place-n-route
bram_fw.hex:
	$(ICESTORM_PATH)icebram -v -g 32 $(BRAM_FW_SIZE) > $@

firmware.hex: firmware.bin size_mismatch
	python3 $(P)/tools/makehex/makehex.py $< $(BRAM_FW_SIZE) > $@
testfw.hex: testfw.bin size_mismatch
	python3 $(P)/tools/makehex/makehex.py $< $(BRAM_FW_SIZE) > $@

%.bin: %.elf
	$(SIZE) $<
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $< $@
	chmod -x $@


#-------------------------------------------------------------------
# Source linting.
#-------------------------------------------------------------------
LINT=verilator
LINT_FLAGS = +1364-2001ext+ --lint-only  -Wall -Wno-fatal -Wno-DECLFILENAME \
	--timescale 1ns/1ns -DNO_ICE40_DEFAULT_ASSIGNMENTS

lint: $(FPGA_SRC) $(VERILOG_SRCS) $(ICE40_SIM_CELLS)
	$(LINT) $(LINT_FLAGS) \
	-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
	-DFIRMWARE_HEX=\"$(P)/firmware.hex\" \
	-DUDS_HEX=\"$(P)/data/uds.hex\" \
	-DUDI_HEX=\"$(P)/data/udi.hex\" \
	--top-module application_fpga $^ &> lint_issues.txt
.PHONY: lint


#-------------------------------------------------------------------
# Build Verilator compiled simulation for the design.
#-------------------------------------------------------------------
verilator: $(VERILATOR_FPGA_SRC) $(VERILOG_SRCS) firmware.hex $(ICE40_SIM_CELLS) \
		$(P)/tb/application_fpga_verilator.cc
	verilator --timescale 1ns/1ns -DNO_ICE40_DEFAULT_ASSIGNMENTS \
                  -Wall -Wno-COMBDLY -Wno-lint \
		  -DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		  -DFIRMWARE_HEX=\"$(P)/firmware.hex\" \
		  -DUDS_HEX=\"$(P)/data/uds.hex\" \
		  -DUDI_HEX=\"$(P)/data/udi.hex\" \
		  --cc --exe --Mdir verilated --top-module application_fpga \
		$(filter %.v, $^) $(filter %.cc, $^)
	make -C verilated -f Vapplication_fpga.mk
.PHONY: verilator


#-------------------------------------------------------------------
# Main FPGA build flow.
# Synthesis. Place & Route. Bitstream generation.
#-------------------------------------------------------------------
synth.json: $(FPGA_SRC) $(VERILOG_SRCS) bram_fw.hex $(P)/data/uds.hex $(P)/data/udi.hex
	$(YOSYS_PATH)yosys -v3 -l synth.log -DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		-DFIRMWARE_HEX=\"$(P)/bram_fw.hex\" \
		-DUDS_HEX=\"$(P)/data/uds.hex\" \
		-DUDI_HEX=\"$(P)/data/udi.hex\" \
		-p 'synth_ice40 -dsp -top application_fpga -json $@; write_verilog -attr2comment synth.v' \
		$(filter %.v, $^)

application_fpga.asc: synth.json $(P)/data/$(PIN_FILE)
	$(NEXTPNR_PATH)nextpnr-ice40  --ignore-loops --up5k --package sg48 --json $< \
		--pcf $(P)/data/$(PIN_FILE) --asc $@

application_fpga.bin: application_fpga.asc bram_fw.hex firmware.hex
	$(ICESTORM_PATH)icebram -v bram_fw.hex firmware.hex < $< > $<.tmp
	$(ICESTORM_PATH)icepack $<.tmp $@
	@-$(RM) $<.tmp

application_fpga_testfw.bin: application_fpga.asc bram_fw.hex testfw.hex
	$(ICESTORM_PATH)icebram -v bram_fw.hex testfw.hex < $< > $<.tmp
	$(ICESTORM_PATH)icepack $<.tmp $@
	@-$(RM) $<.tmp

#-------------------------------------------------------------------
# post-synthesis functional simulation.
#-------------------------------------------------------------------
synth_tb.vvp: $(P)/tb/tb_application_fpga.v synth.json
	iverilog -o $@ -s tb_application_fpga synth.v $(P)/tb/tb_application_fpga.v \
		-DNO_ICE40_DEFAULT_ASSIGNMENTS $(ICE40_SIM_CELLS)
	chmod -x $@

synth_sim: synth_tb.vvp
	vvp -N $<
.PHONY: synth_sim

synth_sim_vcd: synth_tb.vvp
	vvp -N $< +vcd
.PHONY: synth_sim_vcd

#-------------------------------------------------------------------
# post-place and route functional simulation.
#-------------------------------------------------------------------
route.v: application_fpga.asc  $(P)/data/$(PIN_FILE)
	icebox_vlog -L -n application_fpga -sp  $(P)/data/$(PIN_FILE) $< > $@

route_tb.vvp: route.v tb/tb_application_fpga.v
	iverilog -o $@ -s tb_application_fpga $^ $(ICE40_SIM_CELLS)
	chmod -x $@

route_sim: route_tb.vvp
	vvp -N $<
.PHONY: route_sim

route_sim_vcd: route_tb.vvp
	vvp -N $< +vcd
.PHONY: route_sim_vcd

#-------------------------------------------------------------------
# FPGA device programming.
#-------------------------------------------------------------------

prog_flash: application_fpga.bin
	sudo tillitis-iceprog $<
.PHONY: prog_flash

prog_flash_testfw: application_fpga_testfw.bin
	sudo tillitis-iceprog $<
.PHONY: prog_flash_testfw

#-------------------------------------------------------------------
# Post build analysis.
#-------------------------------------------------------------------
timing: application_fpga.asc $(P)/data/$(PIN_FILE)
	$(ICESTORM_PATH)icetime -c 12 -tmd up5k -P sg48 -p $(P)/data/$(PIN_FILE) -t $<

view: tb_application_fpga_vcd
	gtkwave $< application_fpga.gtkw


#-------------------------------------------------------------------
# Cleanup.
#-------------------------------------------------------------------
clean: clean_fw
	rm -f bram_fw.hex
	rm -f synth.{log,v,json} route.v application_fpga.{asc,bin,vcd} application_fpga_testfw.bin
	rm -f tb_application_fpga.vvp synth_tb.vvp route_tb.vvp
	rm -f *.vcd
	rm -f lint_issues.txt
	rm -rf verilated
	rm -f tools/tpt/*.hex
	rm -rf tools/tpt/__pycache__
.PHONY: clean

clean_fw:
	rm -f firmware.{elf,elf.map,bin,hex}
	rm -f $(FIRMWARE_OBJS)
	rm -f testfw.{elf,elf.map,bin,hex}
	rm -f $(TESTFW_OBJS)
.PHONY: clean_fw

#-------------------------------------------------------------------
# Display info about targets.
#-------------------------------------------------------------------
help:
	@echo ""
	@echo "Build system for application_fpga FPGA design and firmware."
	@echo ""
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all                Build all targets."
	@echo "firmware.elf       Build firmware ELF file."
	@echo "firmware.hex       Build firmware converted to hex, to be included in bitstream."
	@echo "bram_fw.hex        Build a fake BRAM file that will be filled in later after place-n-route."
	@echo "verilator          Build Verilator simulation program"
	@echo "lint               Run lint on Verilog source files."
	@echo "prog_flash         Program device flash with FGPA bitstream including firmware (using the RPi Pico-based programmer)."
	@echo "prog_flash_testfw  Program device flash as above, but with testfw."
	@echo "clean              Delete all generated files."
	@echo "clean_fw           Delete only generated files for firmware. Useful for fw devs."

#=======================================================================
# EOF Makefile
#=======================================================================
