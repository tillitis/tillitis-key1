#=======================================================================
#
# Makefile
# --------
# Makefile for building, simulating, running all application_fpga
# HW targets as well as its firmware.
#
#
# Copyright (C) 2022-2024 - Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only
#
#
# Please note: When creating a new cores and adding more testbenches,
# please update the tb target below to include it as well.
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

# FPGA target frequency. Should be in sync with the clock frequency
# given by the parameters to the PLL in rtl/clk_reset_gen.v
TARGET_FREQ ?= 21

# Size in 32-bit words, must be divisible by 256 (pairs of EBRs, because 16
# bits wide; an EBR is 128 32-bits words)
BRAM_FW_SIZE ?= 1536

PIN_FILE ?= application_fpga_tk1.pcf

SIZE ?= llvm-size
OBJCOPY ?= llvm-objcopy

CC = clang

CFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-static \
	-std=gnu99 \
	-O2 \
	-ffast-math \
	-fno-common \
	-fno-builtin-printf \
	-fno-builtin-putchar \
	-fno-builtin-memcpy \
	-nostdlib \
	-mno-relax \
	-Wall \
	-Wpedantic \
	-Wno-language-extension-token \
	-flto \
	-g

AS = clang

ASFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mno-relax

ICE40_SIM_CELLS = $(shell yosys-config --datdir/ice40/cells_sim.v)


# FPGA specific source files.
FPGA_SRC = \
	$(P)/rtl/application_fpga.v \
	$(P)/core/clk_reset_gen/rtl/clk_reset_gen.v

# Verilator simulation specific source files.
VERILATOR_FPGA_SRC = \
	$(P)/tb/application_fpga_vsim.v \
	$(P)/tb/reset_gen_vsim.v

# Common verilog source files.
VERILOG_SRCS = \
	$(P)/core/ram/rtl/ram.v \
	$(P)/core/rom/rtl/rom.v \
	$(P)/core/fw_ram/rtl/fw_ram.v \
	$(P)/core/picorv32/rtl/picorv32.v \
	$(P)/core/timer/rtl/timer_core.v \
	$(P)/core/timer/rtl/timer.v \
	$(P)/core/uds/rtl/uds.v \
	$(P)/core/uds/rtl/uds_rom.v \
	$(P)/core/touch_sense/rtl/touch_sense.v \
	$(P)/core/tk1/rtl/tk1.v \
	$(P)/core/tk1/rtl/tk1_spi_master.v \
	$(P)/core/tk1/rtl/udi_rom.v \
	$(P)/core/uart/rtl/uart_core.v \
	$(P)/core/uart/rtl/uart_fifo.v \
	$(P)/core/uart/rtl/uart.v \
	$(P)/core/trng/rtl/rosc.v

FIRMWARE_DEPS = \
	$(P)/fw/tk1_mem.h \
	$(P)/fw/tk1/types.h \
	$(P)/fw/tk1/lib.h \
	$(P)/fw/tk1/proto.h \
	$(P)/fw/tk1/assert.h \
	$(P)/fw/tk1/led.h

FIRMWARE_OBJS = \
	$(P)/fw/tk1/main.o \
	$(P)/fw/tk1/start.o \
	$(P)/fw/tk1/proto.o \
	$(P)/fw/tk1/lib.o \
	$(P)/fw/tk1/assert.o \
	$(P)/fw/tk1/led.o \
	$(P)/fw/tk1/blake2s/blake2s.o

FIRMWARE_SOURCES = \
	$(P)/fw/tk1/main.c \
	$(P)/fw/tk1/proto.c \
	$(P)/fw/tk1/lib.c \
	$(P)/fw/tk1/assert.c \
	$(P)/fw/tk1/led.c \
	$(P)/fw/tk1/blake2s/blake2s.c

TESTFW_OBJS = \
	$(P)/fw/testfw/main.o \
	$(P)/fw/testfw/start.o \
	$(P)/fw/tk1/proto.o \
	$(P)/fw/tk1/lib.o \
	$(P)/fw/tk1/blake2s/blake2s.o

#-------------------------------------------------------------------
# All: Complete build of HW and FW.
#-------------------------------------------------------------------
all: application_fpga.bin
.PHONY: all

#-------------------------------------------------------------------
# The size_mismatch target make sure that we don't end up with an
# incorrect BRAM_FW_SIZE
# -------------------------------------------------------------------
%_size_mismatch: %.elf phony_explicit
	@test $$($(SIZE) $< | awk 'NR==2{print $$4}') -le $$(( 32 / 8 * $(BRAM_FW_SIZE) )) \
	|| { printf "The 'BRAM_FW_SIZE' variable needs to be increased\n"; \
	[[ $< =~ testfw ]] && printf "Note that testfw fits if built with -Os\n"; \
	false; }

# can't make implicit rule .PHONY
phony_explicit:
.PHONY: phony_explicit

#-------------------------------------------------------------------
# Personalization of the TKey
#-------------------------------------------------------------------

secret:
	cd data;../tools/tpt/tpt.py
.PHONY: secret

#-------------------------------------------------------------------
# Firmware generation.
# Included in the bitstream.
#-------------------------------------------------------------------
LDFLAGS = -T $(P)/fw/tk1/firmware.lds

$(FIRMWARE_OBJS): $(FIRMWARE_DEPS)
$(TESTFW_OBJS): $(FIRMWARE_DEPS)

firmware.elf: $(FIRMWARE_OBJS) $(P)/fw/tk1/firmware.lds
	$(CC) $(CFLAGS) $(FIRMWARE_OBJS) $(LDFLAGS) -o $@

qemu_firmware.elf: CFLAGS += -DQEMU_CONSOLE
qemu_firmware.elf: firmware.elf
	mv firmware.elf qemu_firmware.elf

.PHONY: check
check:
	clang-tidy -header-filter=.* -checks=cert-* $(FIRMWARE_SOURCES) -- $(CFLAGS)

.PHONY: splint
splint:
	splint \
		-nolib \
		-predboolint \
		+boolint \
		-nullpass \
		-unrecog \
		-infloops \
		-initallelements \
		-type \
		-unreachable \
		-unqualifiedtrans \
		-fullinitblock \
		$(FIRMWARE_SOURCES)

testfw.elf: $(TESTFW_OBJS) $(P)/fw/tk1/firmware.lds
	$(CC) $(CFLAGS) $(TESTFW_OBJS) $(LDFLAGS) -o $@

# Generate a fake BRAM file that will be filled in later after place-n-route
bram_fw.hex:
	$(ICESTORM_PATH)icebram -v -g 32 $(BRAM_FW_SIZE) > $@

firmware.hex: firmware.bin firmware_size_mismatch
	python3 $(P)/tools/makehex/makehex.py $< $(BRAM_FW_SIZE) > $@
testfw.hex: testfw.bin testfw_size_mismatch
	python3 $(P)/tools/makehex/makehex.py $< $(BRAM_FW_SIZE) > $@

.PHONY: check-binary-hashes
check-binary-hashes:
	sha512sum firmware.bin
	sha256sum application_fpga.bin
	sha512sum -c firmware.bin.sha512
	sha256sum -c application_fpga.bin.sha256

%.bin: %.elf
	$(SIZE) $<
	@test "$$($(SIZE) $< | awk 'NR==2{print $$2, $$3}')" = "0 0" \
	|| { printf "Non-empty data or bss section!\n"; false; }
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $< $@
	chmod -x $@

#-------------------------------------------------------------------
# Source linting.
#-------------------------------------------------------------------
LINT = verilator
# For Verilator 5.019 -Wno-GENUNNAMED needs to be added to LINT_FLAGS for the
# cell library.
LINT_FLAGS = \
	+1364-2005ext+ \
	--lint-only \
	-Wall \
	-Wno-DECLFILENAME \
	-Wno-WIDTHEXPAND \
	-Wno-UNOPTFLAT \
	--timescale 1ns/1ns \
	-DNO_ICE40_DEFAULT_ASSIGNMENTS

lint: $(FPGA_SRC) $(VERILOG_SRCS) $(ICE40_SIM_CELLS)
	$(LINT) $(LINT_FLAGS) \
	-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
	-DFIRMWARE_HEX=\"$(P)/firmware.hex\" \
	-DUDS_HEX=\"$(P)/data/uds.hex\" \
	-DUDI_HEX=\"$(P)/data/udi.hex\" \
	--top-module application_fpga \
	config.vlt $^ \
	>lint_issues.txt 2>&1 \
	&& { rm -f lint_issues.txt; exit 0; } \
	|| {   cat lint_issues.txt; exit 1; }
.PHONY: lint

#-------------------------------------------------------------------
# Build Verilator compiled simulation for the design.
#-------------------------------------------------------------------
verilator: $(VERILATOR_FPGA_SRC) $(VERILOG_SRCS) firmware.hex $(ICE40_SIM_CELLS) \
		$(P)/tb/application_fpga_verilator.cc
	verilator \
		--timescale 1ns/1ns \
		-DNO_ICE40_DEFAULT_ASSIGNMENTS \
		-Wall \
		-Wno-COMBDLY \
		-Wno-lint \
		-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		-DFIRMWARE_HEX=\"$(P)/firmware.hex\" \
		-DUDS_HEX=\"$(P)/data/uds.hex\" \
		-DUDI_HEX=\"$(P)/data/udi.hex\" \
		--cc \
		--exe \
		--Mdir verilated \
		--top-module application_fpga \
		$(filter %.v, $^) \
		$(filter %.cc, $^)
	make -C verilated -f Vapplication_fpga.mk
.PHONY: verilator

#-------------------------------------------------------------------
# Run all testbenches
#-------------------------------------------------------------------
tb:
	make -C core/timer/toolruns sim-top
	make -C core/tk1/toolruns sim-top
	make -C core/touch_sense/toolruns sim-top
	make -C core/trng/toolruns sim-top
	make -C core/uart/toolruns sim-top
	make -C core/uds/toolruns sim-top

.PHONY: tb

#-------------------------------------------------------------------
# Main FPGA build flow.
# Synthesis. Place & Route. Bitstream generation.
#-------------------------------------------------------------------

YOSYS_FLAG ?=

synth.json: $(FPGA_SRC) $(VERILOG_SRCS) bram_fw.hex $(P)/data/uds.hex $(P)/data/udi.hex
	$(YOSYS_PATH)yosys \
		-v3 \
		-l synth.txt \
		$(YOSYS_FLAG) \
		-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		-DFIRMWARE_HEX=\"$(P)/bram_fw.hex\" \
		-p 'synth_ice40 -dsp -top application_fpga -json $@; write_verilog -attr2comment synth.v' \
		$(filter %.v, $^)

application_fpga_par.json: synth.json $(P)/data/$(PIN_FILE)
	$(NEXTPNR_PATH)nextpnr-ice40 \
		-l application_fpga_par.txt \
		--freq $(TARGET_FREQ) \
		--ignore-loops \
		--up5k \
		--package sg48 \
		--json $< \
		--pcf $(P)/data/$(PIN_FILE) \
		--write $@ \
		&& {                                  exit 0; } \
		|| { rm -f application_fpga_par.json; exit 1; }

application_fpga.asc: application_fpga_par.json $(P)/data/uds.hex $(P)/data/udi.hex
	UDS_HEX="$(P)/data/uds.hex" \
	UDI_HEX="$(P)/data/udi.hex" \
	OUT_ASC=$@ \
	$(NEXTPNR_PATH)nextpnr-ice40 \
		--up5k \
		--package sg48 \
		--ignore-loops \
		--json $< \
		--run tools/patch_uds_udi.py

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
	iverilog \
		-o $@ \
		-s tb_application_fpga synth.v $(P)/tb/tb_application_fpga.v \
		-DNO_ICE40_DEFAULT_ASSIGNMENTS \
		$(ICE40_SIM_CELLS)
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

prog_flash: check-hardware application_fpga.bin
	sudo tillitis-iceprog application_fpga.bin
.PHONY: prog_flash

prog_flash_testfw: check-hardware application_fpga_testfw.bin
	sudo tillitis-iceprog application_fpga_testfw.bin
.PHONY: prog_flash_testfw

check-hardware:
	@sudo tillitis-iceprog -t >/dev/null 2>&1 || \
	{ echo "Programmer not plugged in or not accessible"; false; }
	@if sudo tillitis-iceprog -t 2>&1 | grep -qi "^flash.id:\( 0x\(00\|ff\)\)\{4\}"; then \
	echo "No USB stick in the programmer?"; false; else true; fi
.PHONY: check-hardware

#-------------------------------------------------------------------
# Post build analysis.
#-------------------------------------------------------------------
timing: application_fpga.asc $(P)/data/$(PIN_FILE)
	$(ICESTORM_PATH)icetime -c 18 -tmd up5k -P sg48 -p $(P)/data/$(PIN_FILE) -t $<

view: tb_application_fpga_vcd
	gtkwave $< application_fpga.gtkw


#-------------------------------------------------------------------
# Cleanup.
#-------------------------------------------------------------------
clean: clean_fw clean_tb
	rm -f bram_fw.hex
	rm -f synth.{v,json,txt} route.v application_fpga.{asc,bin,vcd} application_fpga_testfw.bin
	rm -f tb_application_fpga.vvp synth_tb.vvp route_tb.vvp
	rm -f application_fpga_par.{json,txt}
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
	rm -f qemu_firmware.elf
.PHONY: clean_fw

clean_tb:
	make -C core/timer/toolruns clean
	make -C core/tk1/toolruns clean
	make -C core/touch_sense/toolruns clean
	make -C core/trng/toolruns clean
	make -C core/uart/toolruns clean
	make -C core/uds/toolruns clean
.PHONY: clean_tb

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
	@echo "check              Run static analysis on firmware."
	@echo "splint             Run splint static analysis on firmware."
	@echo "firmware.elf       Build firmware ELF file."
	@echo "firmware.hex       Build firmware converted to hex, to be included in bitstream."
	@echo "bram_fw.hex        Build a fake BRAM file that will be filled in later after place-n-route."
	@echo "verilator          Build Verilator simulation program"
	@echo "lint               Run lint on Verilog source files."
	@echo "tb                 Run all testbenches"
	@echo "prog_flash         Program device flash with FGPA bitstream including firmware (using the RPi Pico-based programmer)."
	@echo "prog_flash_testfw  Program device flash as above, but with testfw."
	@echo "clean              Delete all generated files."
	@echo "clean_fw           Delete only generated files for firmware. Useful for fw devs."
	@echo "clean_tb           Delete only generated files for testbenches."

#=======================================================================
# EOF Makefile
#=======================================================================
