#=======================================================================
#
# Makefile
# --------
# Makefile for building, simulating, running all application_fpga
# HW targets as well as its firmware.
#
#
# SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause
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
TARGET_FREQ ?= 24

# Size in 32-bit words, must be divisible by 256 (pairs of EBRs, because 16
# bits wide; an EBR is 128 32-bits words)
BRAM_FW_SIZE ?= 2048

PIN_FILE ?= application_fpga_tk1.pcf

SIZE ?= llvm-size
OBJCOPY ?= llvm-objcopy

CC = clang

LIBDIR ?= tkey-libs

# The preloaded app in slot 0. Must be defined. Firmware depends on
# it. Change this if you want another first preloaded app.
FLASH_APP_0 ?= $(P)/apps/defaultapp.bin
TKEYIMAGE_FLAGS = -app0 $(FLASH_APP_0)
FLASH_IMAGE_DEPS = $(FLASH_APP_0)

ifdef FLASH_APP_1
	TKEYIMAGE_FLAGS +=-app1 $(FLASH_APP_1)
	FLASH_IMAGE_DEPS += $(FLASH_APP_1)
endif

ifdef FLASH_APP_1_PUB
	TKEYIMAGE_FLAGS +=-app1pub $(FLASH_APP_1_PUB)
	FLASH_IMAGE_DEPS += $(FLASH_APP_1_PUB)
endif

ifdef FLASH_APP_1_SIG
	TKEYIMAGE_FLAGS +=-app1sig $(FLASH_APP_1_SIG)
	FLASH_IMAGE_DEPS += $(FLASH_APP_1_SIG)
endif

CFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-static \
	-std=gnu99 \
	-Os \
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
	-Wextra \
	-flto \
	-g \
	-I $(LIBDIR)/include \
	-I $(LIBDIR) \
	-I $(LIBDIR)/blake2s

AS = clang

ASFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mno-relax \
	-I $(LIBDIR)/include

ICE40_SIM_CELLS = $(shell yosys-config --datdir/ice40/cells_sim.v)


# FPGA specific source files.
FPGA_VERILOG_SRCS = \
	$(P)/rtl/application_fpga.v \
	$(P)/core/clk_reset_gen/rtl/clk_reset_gen.v \
	$(P)/core/trng/rtl/trng.v

# Testbench simulation specific source files.
SIM_VERILOG_SRCS = \
	$(P)/tb/tb_application_fpga_sim.v \
	$(P)/tb/application_fpga_sim.v \
	$(P)/tb/reset_gen_sim.v \
	$(P)/tb/trng_sim.v

# Verilator simulation specific source files.
VERILATOR_VERILOG_SRCS = \
	$(P)/tb/application_fpga_sim.v \
	$(P)/tb/reset_gen_sim.v \
	$(P)/tb/trng_sim.v

# Common verilog source files.
VERILOG_SRCS = \
	$(P)/core/ram/rtl/ram.v \
	$(P)/core/rom/rtl/rom.v \
	$(P)/core/fw_ram/rtl/fw_ram.v \
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
	$(P)/core/uart/rtl/uart.v

# PicoRV32 verilog source file
PICORV32_SRCS = \
	$(P)/core/picorv32/rtl/picorv32.v


# Mention mgmt_app_digest.h explicitly since it needs to be generated.
FIRMWARE_DEPS = \
	$(P)/fw/tk1/*.h \
	$(P)/fw/tk1/mgmt_app_digest.h

FIRMWARE_OBJS = \
	$(P)/fw/tk1/main.o \
	$(P)/fw/tk1/start.o \
	$(P)/fw/tk1/proto.o \
	$(P)/fw/tk1/syscall_enable.o \
	$(P)/fw/tk1/syscall_handler.o \
	$(P)/fw/tk1/spi.o \
	$(P)/fw/tk1/flash.o \
	$(P)/fw/tk1/storage.o \
	$(P)/fw/tk1/partition_table.o \
	$(P)/fw/tk1/auth_app.o \
	$(P)/fw/tk1/rng.o \
	$(P)/fw/tk1/reset.o \
	$(P)/fw/tk1/preload_app.o \
	$(P)/fw/tk1/mgmt_app.o \
	$(P)/fw/tk1/memcheck.o \

CHECK_SOURCES = \
	$(P)/fw/tk1/*.[ch]

TESTFW_OBJS = \
	$(P)/fw/testfw/main.o \
	$(P)/fw/testfw/start.o

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
	@test $$(( \
		$$($(SIZE) -A $< | grep text | awk 'NR==1{print $$2}') + \
		$$($(SIZE) -A $< | grep text | awk 'NR==2{print $$2}') \
		)) -le $$(( 32 / 8 * $(BRAM_FW_SIZE) )) \
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

# -------------------------------------------------------------------
# Build tools
# -------------------------------------------------------------------

# .PHONY to let go build handle deps and rebuilds
.PHONY: tools/tkeyimage/tkeyimage
tools/tkeyimage/tkeyimage:
	go build -C $(P)/tools/tkeyimage

.PHONY: tools/b2s/b2s
tools/b2s/b2s:
	go build -C $(P)/tools/b2s

#-------------------------------------------------------------------
# Firmware generation.
# Included in the bitstream.
#-------------------------------------------------------------------
LDFLAGS = \
	-T $(P)/fw/tk1/firmware.lds \
	-Wl,--cref,-M \
	-L $(LIBDIR) -lcommon -lblake2s

QEMU_LDFLAGS = \
	-T $(P)/fw/tk1/qemu_firmware.lds \
	-Wl,--cref,-M \
	-L $(LIBDIR) -lcommon -lblake2s

# Common libraries the firmware and testfw depend on. See
# https://github.com/tillitis/tkey-libs/
.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

# Device app used as preloaded app
$(FLASH_APP_0):
	make -C apps

# Build a complete filesystem image
flash_image.bin: tools/tkeyimage/tkeyimage $(FLASH_IMAGE_DEPS)
	./tools/tkeyimage/tkeyimage -f $(TKEYIMAGE_FLAGS) -o $@

$(FIRMWARE_OBJS): $(FIRMWARE_DEPS)
$(TESTFW_OBJS): $(FIRMWARE_DEPS)

#firmware.elf: CFLAGS += -DTKEY_DEBUG
firmware.elf: tkey-libs $(FIRMWARE_OBJS) $(P)/fw/tk1/firmware.lds $(P)/fw/tk1/mgmt_app_digest.h
	$(CC) $(CFLAGS) $(FIRMWARE_OBJS) $(LDFLAGS) -o $@ > $(basename $@).map

simfirmware.elf: CFLAGS += -DSIMULATION
simfirmware.elf: $(FIRMWARE_OBJS) $(P)/fw/tk1/firmware.lds
	$(CC) $(CFLAGS) $(FIRMWARE_OBJS) $(LDFLAGS) -o $@ > $(basename $@).map

qemu_firmware.elf: CFLAGS += -DQEMU_DEBUG
qemu_firmware.elf: ASFLAGS += -DQEMU_DEBUG
qemu_firmware.elf: CFLAGS += -DQEMU_SYSCALL
qemu_firmware.elf: ASFLAGS += -DQEMU_SYSCALL
qemu_firmware.elf: tkey-libs $(FIRMWARE_OBJS) $(P)/fw/tk1/qemu_firmware.lds $(P)/fw/tk1/mgmt_app_digest.h
	$(CC) $(CFLAGS) $(FIRMWARE_OBJS) $(QEMU_LDFLAGS) -o $@ > $(basename $@).map

# Generate the file containing the digest for the obligatory default
# app in slot 0.
$(P)/fw/tk1/mgmt_app_digest.h: $(FLASH_APP_0) tools/b2s/b2s
	./tools/b2s/b2s -c -m $(FLASH_APP_0) > $@

# Create compile_commands.json for clangd and LSP
.PHONY: clangd
clangd: compile_commands.json
compile_commands.json:
	$(MAKE) clean
	bear -- make qemu_firmware.elf

.PHONY: check
check:
	clang-tidy -header-filter=.* -checks=cert-*,clang-analyzer-* $(CHECK_SOURCES) -- $(CFLAGS)

.PHONY: splint
splint:
	splint \
		+unixlib \
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
		+gnuextensions \
		-fixedformalarray \
		-mustfreeonly \
		-I $(LIBDIR)/include \
		-I $(LIBDIR) \
		-I $(LIBDIR)/blake2s \
		$(CHECK_SOURCES)

testfw.elf: tkey-libs $(TESTFW_OBJS) $(P)/fw/tk1/firmware.lds
	$(CC) $(CFLAGS) $(TESTFW_OBJS) $(LDFLAGS) -o $@ > $(basename $@).map

# Generate a fake BRAM file that will be filled in later after place-n-route
bram_fw.hex:
	$(ICESTORM_PATH)icebram -v -g 32 $(BRAM_FW_SIZE) > $@

firmware.hex: firmware.bin firmware_size_mismatch
	python3 $(P)/tools/makehex.py $< $(BRAM_FW_SIZE) > $@
simfirmware.hex: simfirmware.bin simfirmware_size_mismatch
	python3 $(P)/tools/makehex.py $< $(BRAM_FW_SIZE) > $@
testfw.hex: testfw.bin testfw_size_mismatch
	python3 $(P)/tools/makehex.py $< $(BRAM_FW_SIZE) > $@

.PHONY: check-binary-hashes
check-binary-hashes:
	sha512sum firmware.bin
	sha256sum application_fpga.bin
	sha512sum -c firmware.bin.sha512
	sha256sum -c application_fpga.bin.sha256

%.bin: %.elf
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
	-DNO_ICE40_DEFAULT_ASSIGNMENTS \
	-Wno-GENUNNAMED

lint: $(FPGA_VERILOG_SRCS) \
		$(SIM_VERILOG_SRCS) \
		$(VERILOG_SRCS) \
		$(PICORV32_SRCS)
	$(LINT) $(LINT_FLAGS) \
	-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
	-DFIRMWARE_HEX=\"$(P)/firmware.hex\" \
	-DUDS_HEX=\"$(P)/data/uds.hex\" \
	-DUDI_HEX=\"$(P)/data/udi.hex\" \
	--top-module application_fpga \
	config.vlt $^ $(ICE40_SIM_CELLS) \
	>lint_issues.txt 2>&1 \
	&& { rm -f lint_issues.txt; exit 0; } \
	|| {   cat lint_issues.txt; exit 1; }
.PHONY: lint

#-------------------------------------------------------------------
# Source formatting.
#-------------------------------------------------------------------
FORMAT = verible-verilog-format

FORMAT_FLAGS = \
	--indentation_spaces=2 \
	--wrap_end_else_clauses=true \
	--inplace

CHECK_FORMAT_FLAGS = \
	--indentation_spaces=2 \
	--wrap_end_else_clauses=true \
	--inplace \
	--verify

fmt: $(FPGA_VERILOG_SRCS) $(SIM_VERILOG_SRCS) $(VERILATOR_VERILOG_SRCS) $(VERILOG_SRCS)
	$(FORMAT) $(FORMAT_FLAGS) $^
.PHONY: fmt

# Temporary fix using grep, since the verible with --verify flag only returns
# error if the last file is malformatted.
checkfmt: $(FPGA_VERILOG_SRCS) $(SIM_VERILOG_SRCS) $(VERILATOR_VERILOG_SRCS) $(VERILOG_SRCS)
	make -C fw/tk1 checkfmt
	make -C fw/testfw checkfmt
	make -C apps checkfmt
	$(FORMAT) $(CHECK_FORMAT_FLAGS) $^ 2>&1 | \
		grep "Needs formatting" && exit 1 || true
.PHONY: checkfmt

#-------------------------------------------------------------------
# Build Verilator compiled simulation for the design.
#-------------------------------------------------------------------
verilator: $(VERILATOR_VERILOG_SRCS) $(VERILOG_SRCS) $(PICORV32_SRCS) \
		firmware.hex \
		$(P)/tb/application_fpga_verilator.cc
	verilator \
		--timescale 1ns/1ns \
		-DNO_ICE40_DEFAULT_ASSIGNMENTS \
		-Wall \
		-Wno-COMBDLY \
		-Wno-lint \
		-Wno-UNOPTFLAT \
		-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		-DFIRMWARE_HEX=\"$(P)/firmware.hex\" \
		-DUDS_HEX=\"$(P)/data/uds.hex\" \
		-DUDI_HEX=\"$(P)/data/udi.hex\" \
		--cc \
		--exe \
		--Mdir verilated \
		--top-module application_fpga_sim \
		$(filter %.v, $^) $(ICE40_SIM_CELLS) \
		$(filter %.cc, $^)
	make -C verilated -f Vapplication_fpga_sim.mk
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

synth.json: $(FPGA_VERILOG_SRCS) $(VERILOG_SRCS) $(PICORV32_SRCS) bram_fw.hex
	$(YOSYS_PATH)yosys \
		-v3 \
		-l synth.txt \
		$(YOSYS_FLAG) \
		-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		-DFIRMWARE_HEX=\"$(P)/bram_fw.hex\" \
		-p 'synth_ice40 -abc2 -device u -dff -dsp -top application_fpga -json $@' \
		-p 'write_verilog -attr2comment synth.v' \
		$(filter %.v, $^)

application_fpga_par.json: synth.json $(P)/data/$(PIN_FILE)
	$(NEXTPNR_PATH)nextpnr-ice40 \
		-l application_fpga_par.txt \
		--seed 2945058561865725735 \
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
# Build testbench simulation for the design
#-------------------------------------------------------------------
tb_application_fpga: $(SIM_VERILOG_SRCS) \
			$(VERILOG_SRCS) \
			$(PICORV32_SRCS) \
			simfirmware.hex
	python3 ./tools/app_bin_to_spram_hex.py \
			./tb/app.bin \
			./tb/output_spram0.hex \
			./tb/output_spram1.hex \
			./tb/output_spram2.hex \
			./tb/output_spram3.hex \
			|| { echo -e "\n -- Put your app.bin to simulate in the \"tb\" directory\n"; false; }
	verilator \
		-j $(shell nproc --ignore=1) \
		--binary \
		--cc \
		--exe \
		--Mdir tb_verilated \
		--trace-fst \
		--trace-structs \
		--top-module tb_application_fpga_sim \
		--timescale 1ns/1ns \
		--timing \
		-Wno-WIDTHEXPAND \
		-Wno-UNOPTFLAT \
		-DNO_ICE40_DEFAULT_ASSIGNMENTS \
		-DAPP_SIZE=$(shell ls -l tb/app.bin| awk '{print $$5}') \
		-DBRAM_FW_SIZE=$(BRAM_FW_SIZE) \
		-DFIRMWARE_HEX=\"$(P)/simfirmware.hex\" \
		-DUDS_HEX=\"$(P)/data/uds.hex\" \
		-DUDI_HEX=\"$(P)/data/udi.hex\" \
		$(filter %.v, $^) $(ICE40_SIM_CELLS)
	make -C tb_verilated -f Vtb_application_fpga_sim.mk
	./tb_verilated/Vtb_application_fpga_sim \
		&& { echo -e "\n -- Wave simulation saved to tb_application_fpga_sim.fst\n"; true; }

#-------------------------------------------------------------------
# FPGA device programming.
#-------------------------------------------------------------------

prog_flash: check-hardware application_fpga.bin flash_image.bin
	tillitis-iceprog flash_image.bin
	tillitis-iceprog application_fpga.bin
.PHONY: prog_flash

prog_flash_bs: check-hardware application_fpga.bin
	tillitis-iceprog application_fpga.bin
.PHONY: prog_flash_bs

prog_flash_testfw: check-hardware application_fpga_testfw.bin
	tillitis-iceprog application_fpga_testfw.bin
.PHONY: prog_flash_testfw

check-hardware:
	@tillitis-iceprog -t >/dev/null 2>&1 || \
	{ echo "Programmer not plugged in or not accessible"; false; }
	@if tillitis-iceprog -t 2>&1 | grep -qi "^flash.id:\( 0x\(00\|ff\)\)\{4\}"; then \
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
clean: clean_sim clean_fw clean_tb
	rm -f bram_fw.hex
	rm -f synth.{v,json,txt} application_fpga.{asc,bin} application_fpga_testfw.bin
	rm -f application_fpga_par.{json,txt}
	rm -f lint_issues.txt
	rm -f tools/tpt/*.hex
	rm -rf tools/tpt/__pycache__
	make -C apps clean
	rm -f fw/tk1/mgmt_app_digest.h
	rm -f flash_image.bin

.PHONY: clean

clean_fw:
	rm -f firmware.{elf,map,bin,hex}
	rm -f $(FIRMWARE_OBJS)
	rm -f testfw.{elf,map,bin,hex}
	rm -f $(TESTFW_OBJS)
	rm -f qemu_firmware.{elf,map,bin,hex}
	make -C tkey-libs clean
.PHONY: clean_fw

clean_sim:
	rm -f simfirmware.{elf,map,bin,hex}
	rm -f tb_application_fpga_sim.fst
	rm -f tb_application_fpga_sim.fst.hier
	rm -f tb/output_spram*.hex
	rm -rf tb_verilated
	rm -rf verilated
.PHONY: clean_sim

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
	@echo "all                  Build all targets."
	@echo "check                Run static analysis on firmware."
	@echo "splint               Run splint static analysis on firmware."
	@echo "firmware.elf         Build firmware ELF file."
	@echo "firmware.hex         Build firmware converted to hex, to be included in bitstream."
	@echo "bram_fw.hex          Build a fake BRAM file that will be filled in later after place-n-route."
	@echo "verilator            Build Verilator simulation program"
	@echo "tb_application_fpga  Build testbench simulation for the design"
	@echo "lint                 Run lint on Verilog source files."
	@echo "tb                   Run all testbenches"
	@echo "prog_flash           Program device flash with FGPA bitstream (including firmware), partition table, and defaultapp.bin (using the RPi Pico-based programmer)."
	@echo "prog_flash_bs        Program device flash with FGPA bitstream including firmware (using the RPi Pico-based programmer)."
	@echo "prog_flash_testfw    Program device flash as above, but with testfw."
	@echo "clean                Delete all generated files."
	@echo "clean_fw             Delete only generated files for firmware. Useful for fw devs."
	@echo "clean_tb             Delete only generated files for testbenches."

#=======================================================================
# EOF Makefile
#=======================================================================
