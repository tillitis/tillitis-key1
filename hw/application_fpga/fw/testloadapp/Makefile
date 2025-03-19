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
	-I $(LIBDIR)

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
all: testloadapp.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

TESTLOADAPP_FMTFILES = *.[ch]

TESTLOADAPP_OBJS = \
	$(P)/main.o \
	../testapp/syscall.o \

testloadapp.elf: tkey-libs $(TESTLOADAPP_OBJS)
	$(CC) $(CFLAGS) $(TESTLOADAPP_OBJS) $(LDFLAGS) -o $@

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(TESTLOADAPP_FMTFILES)
	clang-format --verbose -i $(TESTLOADAPP_FMTFILES)

.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(TESTLOADAPP_FMTFILES)

.PHONY: clean
clean:
	rm -f testloadapp.bin testloadapp.elf $(TESTLOADAPP_OBJS)
