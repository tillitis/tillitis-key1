#######################################################

# Toolchain
CC = sdcc
OBJCOPY = objcopy
PACK_HEX = packihx
CHPROG = chprog

#######################################################

TARGET = usb_device

ROOT_DIR = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

XRAM_SIZE = 0x0400   # 1 KB on-chip xRAM
XRAM_LOC  = 0x0000   # xRAM area starts at address 0 in the External Data Address Space
CODE_SIZE = 0x3800   # 14 KB program storage area
FREQ_SYS  = 16000000 # 16 MHz system clock

EXTRA_FLAGS = -DBUILD_CODE

CFLAGS = \
         -V \
         -mmcs51 \
         --model-small \
         --xram-size $(XRAM_SIZE) \
         --xram-loc $(XRAM_LOC) \
         --code-size $(CODE_SIZE) \
         -I$(ROOT_DIR)include \
         -DFREQ_SYS=$(FREQ_SYS) \
         $(EXTRA_FLAGS)

LFLAGS = \
         $(CFLAGS)

C_FILES = \
          main.c \
          include/debug.c \
          include/print.c

# Create a .rel file for each .c file
RELS = $(C_FILES:.c=.rel)

%.rel : %.c
	$(CC) -c $(CFLAGS) $<

usb_strings.h: encode_usb_strings.py
	./encode_usb_strings.py

# Note: SDCC will dump all of the temporary files into this one,
# so strip the paths from RELS
# For now, get around this by stripping the paths off of the RELS list.

$(TARGET).ihx: $(RELS)
	$(CC) $(notdir $(RELS)) $(LFLAGS) -o $(TARGET).ihx

$(TARGET).hex: $(TARGET).ihx
	$(PACK_HEX) $(TARGET).ihx > $(TARGET).hex

$(TARGET).bin: $(TARGET).ihx
	$(OBJCOPY) -I ihex -O binary $(TARGET).ihx $(TARGET).bin

flash: $(TARGET).bin
	$(CHPROG) $(TARGET).bin

flash_patched: $(TARGET).bin
	./inject_serial_number.py -i $(TARGET).bin -o patched.bin
	$(CHPROG) patched.bin
	rm patched.bin

.DEFAULT_GOAL := all
all: $(TARGET).bin $(TARGET).hex

clean:
	rm -f \
	$(notdir $(RELS:.rel=.asm)) \
	$(notdir $(RELS:.rel=.lst)) \
	$(notdir $(RELS:.rel=.mem)) \
	$(notdir $(RELS:.rel=.rel)) \
	$(notdir $(RELS:.rel=.rst)) \
	$(notdir $(RELS:.rel=.sym)) \
	$(notdir $(RELS:.rel=.adb)) \
	$(TARGET).lk \
	$(TARGET).map \
	$(TARGET).mem \
	$(TARGET).ihx \
	$(TARGET).hex \
	$(TARGET).bin
