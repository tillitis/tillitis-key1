P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
LIBDIR ?= $(P)/../
OBJCOPY ?= llvm-objcopy
CC = clang

# If you want debug_puts() etcetera to output something on our QEMU
# debug port, use -DQEMU_DEBUG below, or -DTKEY_DEBUG to use Tkeys USB debug pipe
CFLAGS = -g -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 -mcmodel=medany \
   -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf \
   -fno-builtin-putchar -nostdlib -mno-relax -flto \
   -Wall -Werror=implicit-function-declaration \
   -I $(LIBDIR)/include -I $(LIBDIR)
   # -DQEMU_DEBUG -DTKEY_DEBUG

INCLUDE=$(LIBDIR)/include

LDFLAGS=-T $(LIBDIR)/app.lds -Wl,--gc-sections,--print-gc-sections -L $(LIBDIR) -lcommon -lcrt0

.PHONY: all
all: blue.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

BLUEOBJS=blue.o
blue.elf: blue.o
	$(CC) $(CFLAGS) $(BLUEOBJS) $(LDFLAGS) -I $(LIBDIR) -o $@

.PHONY: clean
clean:
	rm -f blue.bin blue.elf $(BLUEOBJS)
