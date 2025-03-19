#######################################################

# Toolchain
CC = sdcc
OBJCOPY = objcopy
PACK_HEX = packihx
CHPROG = chprog

#######################################################

TARGET = usb_device

OUT_DIR = _out

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
         -Iinc \
         -DFREQ_SYS=$(FREQ_SYS) \
         $(EXTRA_FLAGS)

LFLAGS = \
         $(CFLAGS)

C_FILES = \
          src/main.c \
          src/debug.c \
          src/print.c

# Create a .rel file for each .c file in $(OUT_DIR)/
RELS = $(patsubst %.c,$(OUT_DIR)/%.rel,$(C_FILES))

OUT_SUBDIRS = $(sort $(dir $(RELS)))

# Ensure out directory exists
$(OUT_DIR):
	mkdir -p $(OUT_SUBDIRS)

$(OUT_DIR)/%.rel: %.c | $(OUT_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

usb_strings.h: encode_usb_strings.py
	./encode_usb_strings.py

# Compile the final ihx file
$(TARGET).ihx: $(RELS)
	$(CC) $(RELS) $(LFLAGS) -o $(TARGET).ihx

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
	rm -rf $(OUT_DIR) \
	$(TARGET).lk \
	$(TARGET).map \
	$(TARGET).mem \
	$(TARGET).ihx \
	$(TARGET).hex \
	$(TARGET).bin
