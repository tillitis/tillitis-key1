P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
LIBDIR ?= ../../tkey-libs
OBJCOPY ?= llvm-objcopy
CC = clang
CFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mcmodel=medany \
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
	-Werror \
	-flto \
	-g \
	-I $(LIBDIR)/include \
	-I $(LIBDIR) \
	-DTKEY_DEBUG

AS = clang

ASFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mno-relax

LDFLAGS = \
	-T $(P)/app.lds \
	-L $(LIBDIR) -lcommon

.PHONY: all
all: reset_test.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

RESET_TEST_FMTFILES = \
	$(P)/main.c \
	$(P)/syscall.h

RESET_TEST_OBJS = \
	$(P)/main.o \
	$(P)/crt0.o \
	$(P)/syscall.o

reset_test.elf: tkey-libs $(RESET_TEST_OBJS)
	$(CC) $(CFLAGS) $(RESET_TEST_OBJS) $(LDFLAGS) -o $@

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(RESET_TEST_FMTFILES)
	clang-format --verbose -i $(RESET_TEST_FMTFILES)

.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(RESET_TEST_FMTFILES)

.PHONY: clean
clean:
	rm -f reset_test.bin reset_test.elf $(RESET_TEST_OBJS)
