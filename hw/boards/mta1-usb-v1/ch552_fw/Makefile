TARGET = usb_device_cdc

CH554_SDCC=~/ch554_sdcc/
CHPROG=chprog


# Adjust the XRAM location and size to leave space for the USB DMA buffers
# Buffer layout in XRAM:
# 0x0000 Ep0Buffer[8]
# 0x0040 Ep1Buffer[8]
# 0x0080 EP2Buffer[2*64]
#
# This takes a total of 256bytes, so there are 768 bytes left.
XRAM_SIZE = 0x0300
XRAM_LOC = 0x0100

FREQ_SYS = 16000000

usb_strings.h: encode_usb_strings.py
	./encode_usb_strings.py

C_FILES = \
	main.c \
	include/debug.c

pre-flash:


flash_patched: usb_device_cdc.bin
	./inject_serial_number.py -i usb_device_cdc.bin -o patched.bin
	${CHPROG} patched.bin
	rm patched.bin

include Makefile.include
