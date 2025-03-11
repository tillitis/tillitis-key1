OBJCOPY ?= llvm-objcopy

CC = clang

INCLUDE=include

# Set QEMU_DEBUG and TKEY_DEBUG below when compiling tkey-libs if you
# want debug prints from tkey-libs functions.
#
# - QEMU_DEBUG: the debug port on our qemu emulator
#
# - TKEY_DEBUG: The extra HID endpoint on a real TKey which you can
#   listen on for debug prints.
#
# NOTE WELL: If you just want debug prints on either of them in *your
# own device app* you just need to include tkey/debug.h and define
# either of them. You don't need to recompile tkey-libs.

CFLAGS = -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 \
	-mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common \
	-fno-builtin-printf -fno-builtin-putchar -nostdlib -mno-relax -flto \
	-Wall -Werror=implicit-function-declaration \
	-I $(INCLUDE) -I .

AS = clang
AR = llvm-ar
ASFLAGS = -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 \
	-mcmodel=medany -mno-relax

LDFLAGS=-T app.lds -L libcommon/ -lcommon -L libcrt0/ -lcrt0


.PHONY: all
all: libcrt0.a libcommon.a libmonocypher.a

IMAGE=ghcr.io/tillitis/tkey-builder:4

podman:
	podman run --rm --mount type=bind,source=$(CURDIR),target=/src \
	-w /src -it $(IMAGE) make -j

.PHONY: check
check:
	clang-tidy -header-filter=.* -checks=cert-* libcommon/*.c -- $(CFLAGS)

# C runtime library
libcrt0.a: libcrt0/crt0.o
	$(AR) -qc $@ libcrt0/crt0.o

# Common C functions
LIBOBJS=libcommon/assert.o libcommon/blake2s.o libcommon/led.o libcommon/lib.o \
	libcommon/proto.o libcommon/touch.o libcommon/io.o

libcommon.a: $(LIBOBJS)
	$(AR) -qc $@ $(LIBOBJS)
$(LIBOBJS): include/tkey/assert.h include/tkey/blake2s.h  include/tkey/led.h \
	include/tkey/lib.h include/tkey/proto.h include/tkey/tk1_mem.h \
	include/tkey/touch.h include/tkey/debug.h

# Monocypher
MONOOBJS=monocypher/monocypher.o monocypher/monocypher-ed25519.o
libmonocypher.a: $(MONOOBJS)
	$(AR) -qc $@ $(MONOOBJS)
$MONOOBJS: monocypher/monocypher-ed25519.h monocypher/monocypher.h

LIBS=libcrt0.a libcommon.a

.PHONY: clean
clean:
	rm -f $(LIBS) $(LIBOBJS) libcrt0/crt0.o
	rm -f libmonocypher.a $(MONOOBJS)

# Create compile_commands.json for clangd and LSP
.PHONY: clangd
clangd: compile_commands.json
compile_commands.json:
	$(MAKE) clean
	bear -- make all

# Uses ../.clang-format
FMTFILES=include/tkey/*.h libcommon/*.c
.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(FMTFILES)
	clang-format --verbose -i $(FMTFILES)
.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(FMTFILES)

.PHONY: update-mem-include
update-mem-include:
	cp -af ../tillitis-key1/hw/application_fpga/fw/tk1_mem.h \
	include/tkey/tk1_mem.h
	echo "Remember to update header include guard!"
