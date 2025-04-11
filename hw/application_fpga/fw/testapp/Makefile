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
	-I $(LIBDIR)

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
all: testapp.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

TESTAPP_FMTFILES = *.[ch]

TESTAPP_OBJS = \
	$(P)/main.o \
	$(P)/crt0.o \
	$(P)/syscall.o

testapp.elf: tkey-libs $(TESTAPP_OBJS)
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
