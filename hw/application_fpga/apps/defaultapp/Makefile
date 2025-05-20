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
all: defaultapp.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

.PHONY: tkey-libs
tkey-libs:
	make -C $(LIBDIR)

DEFAULTAPP_FMTFILES = *.[ch]

DEFAULTAPP_OBJS = \
	$(P)/main.o \
	../testapp/syscall.o \

defaultapp.elf: tkey-libs $(DEFAULTAPP_OBJS)
	$(CC) $(CFLAGS) $(DEFAULTAPP_OBJS) $(LDFLAGS) -o $@

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(DEFAULTAPP_FMTFILES)
	clang-format --verbose -i $(DEFAULT_FMTFILES)

.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(DEFAULT_FMTFILES)

.PHONY: clean
clean:
	rm -f defaultapp.* $(DEFAULTAPP_OBJS)
