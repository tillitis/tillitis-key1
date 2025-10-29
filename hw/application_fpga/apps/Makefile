# SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
LIBDIR ?= ../tkey-libs
OBJCOPY ?= llvm-objcopy
CC = clang
CFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mcmodel=medany \
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
	-Werror \
	-flto \
	-g \
	-I $(LIBDIR)/include \
	-I $(LIBDIR) \
	-I include \
	-I ../

AS = clang

ASFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mno-relax

LDFLAGS = \
	-T $(LIBDIR)/app.lds \
	-L $(LIBDIR) -lcrt0 -lcommon -lmonocypher -lblake2s

.PHONY: all
all: defaultapp.bin loopbackapp.bin reset_test.bin testapp.bin testloadapp.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

OBJS=syscall.o

# syscall.o: syscall.S
# 	$(CC) $(CFLAGS) $(DEFAULTAPP_OBJS) $(LDFLAGS) -o $@

# defaultapp
DEFAULTAPP_FMTFILES = *.[ch]

DEFAULTAPP_OBJS = \
	$(P)/defaultapp/main.o

defaultapp.elf: tkey-libs $(OBJS) $(DEFAULTAPP_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(DEFAULTAPP_OBJS) $(LDFLAGS) -o $@

# loopbackapp

LOOPBACKAPP_OBJS = \
	$(P)/loopbackapp/main.o

loopbackapp.elf: tkey-libs $(LOOPBACKAPP_OBJS)
	$(CC) $(CFLAGS) $(LOOPBACKAPP_OBJS) $(LDFLAGS) -o $@

# reset_test

RESET_TEST_FMTFILES = *.[ch]

RESET_TEST_OBJS = \
	$(P)/reset_test/main.o

reset_test.elf: tkey-libs $(RESET_TEST_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(RESET_TEST_OBJS) $(LDFLAGS) -o $@

# testapp

TESTAPP_OBJS = \
	$(P)/testapp/main.o

testapp.elf: tkey-libs $(TESTAPP_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(TESTAPP_OBJS) $(LDFLAGS) -o $@

# testloadapp

TESTLOADAPP_OBJS = \
	$(P)/testloadapp/main.o

testloadapp.elf: tkey-libs $(TESTLOADAPP_OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(TESTLOADAPP_OBJS) $(LDFLAGS) -o $@

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 defaultapp/*.[ch]
	clang-format --verbose -i defaultapp/*.[ch]

	clang-format --dry-run --ferror-limit=0 loopbackapp/*.[ch]
	clang-format --verbose -i loopbackapp/*.[ch]

	clang-format --dry-run --ferror-limit=0 reset_test/*.[ch]
	clang-format --verbose -i reset_test/*.[ch]

	clang-format --dry-run --ferror-limit=0 testapp/*.[ch]
	clang-format --verbose -i testapp/*.[ch]

	clang-format --dry-run --ferror-limit=0 testloadapp/*.[ch]
	clang-format --verbose -i testloadapp/*.[ch]

.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 defaultapp/*.[ch]

	clang-format --dry-run --ferror-limit=0 loopbackapp/*.[ch]

	clang-format --dry-run --ferror-limit=0 reset_test/*.[ch]

	clang-format --dry-run --ferror-limit=0 testapp/*.[ch]

	clang-format --dry-run --ferror-limit=0 testloadapp/*.[ch]

.PHONY: clean
clean:
	rm -f *.elf *.bin $(OBJS) $(DEFAULTAPP_OBJS) $(LOOPBACKAPP_OBJS) \
	$(RESET_TEST_OBJS) $(TESTAPP_OBJS) $(TESTLOADAPP_OBJS)

