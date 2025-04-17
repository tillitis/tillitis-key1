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
all: ccid.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

CCID_FMTFILES = \
	$(P)/ccid.c \
	$(P)/main.c \
	$(P)/syscall.h

CCID_OBJS = \
	$(P)/ccid.o \
	$(P)/main.o \
	$(P)/crt0.o \
	$(P)/syscall.o

ccid.elf: tkey-libs $(CCID_OBJS)
	$(CC) $(CFLAGS) $(CCID_OBJS) $(LDFLAGS) -o $@

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(CCID_FMTFILES)
	clang-format --verbose -i $(CCID_FMTFILES)

.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(CCID_FMTFILES)

.PHONY: clean
clean:
	rm -f ccid.bin ccid.elf $(CCID_OBJS)
