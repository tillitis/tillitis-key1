#===================================================================
#
# Makefile
# --------
# Makefile for linting the PicoRV32 core.
#
#
# Author: Joachim Strombergson
# Copyright (C) 2024 - Tillitis AB
# SPDX-License-Identifier: BSD-2-Clause
#
#===================================================================

TOP_SRC=../rtl/picorv32.v

LINT = verilator
LINT_FLAGS = +1364-2005ext+ --lint-only -Wall -Wwarn-lint -Wno-fatal -Wno-DECLFILENAME --timescale 1ns/1ns

all:	lint

lint:  $(TOP_SRC)
	$(LINT) $(LINT_FLAGS) $^ --top-module picorv32 > lint_issues.txt 2>&1


clean:
	rm lint_issues.txt

help:
	@echo "Build system for the PicoRV32"
	@echo ""
	@echo "Supported targets:"
	@echo "------------------"
	@echo "lint:  Lint PicoRV32 source files."
	@echo "clean: Delete all generated files."

#===================================================================
# EOF Makefile
#===================================================================
