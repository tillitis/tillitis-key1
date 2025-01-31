P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
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
	-g

AS = clang

ASFLAGS = \
	-target riscv32-unknown-none-elf \
	-march=rv32iczmmul \
	-mabi=ilp32 \
	-mno-relax

LDFLAGS=-T $(P)/app.lds

.PHONY: all
all: testapp.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

TESTAPP_FMTFILES = \
	$(P)/main.c \

TESTAPP_OBJS = \
	$(P)/main.o \
	$(P)/crt0.o \
	$(P)/syscall.o \
	$(P)/../tk1/assert.o \
	$(P)/../tk1/led.o \
	$(P)/../tk1/lib.o \
	$(P)/../tk1/proto.o

testapp.elf: $(TESTAPP_OBJS)
	$(CC) $(CFLAGS) $(TESTAPP_OBJS) $(LDFLAGS) -o $@

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(TESTAPP_FMTFILES)
	clang-format --verbose -i $(TESTAPP_FMTFILES)

.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(TESTAPP_FMTFILES)

.PHONY: clean
clean:
	rm -f testapp.bin testapp.elf $(TESTAPP_OBJS)
