#ifndef __USB_STRINGS_H__
#define __USB_STRINGS_H__

#include "mem.h"

unsigned char FLASH ProdDesc[] = {  // "MTA1-USB-V1"
    24,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    'M', 0, 'T', 0, 'A', 0, '1', 0,
    '-', 0, 'U', 0, 'S', 0, 'B', 0,
    '-', 0, 'V', 0, '1', 0,
};

unsigned char FLASH ManufDesc[] = {  // "Tillitis"
    18,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    'T', 0, 'i', 0, 'l', 0, 'l', 0,
    'i', 0, 't', 0, 'i', 0, 's', 0,
};

unsigned char FLASH SerialDesc[] = {  // "68de5d27-e223-4874-bc76-a54d6e84068f"
    74,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    '6', 0, '8', 0, 'd', 0, 'e', 0,
    '5', 0, 'd', 0, '2', 0, '7', 0,
    '-', 0, 'e', 0, '2', 0, '2', 0,
    '3', 0, '-', 0, '4', 0, '8', 0,
    '7', 0, '4', 0, '-', 0, 'b', 0,
    'c', 0, '7', 0, '6', 0, '-', 0,
    'a', 0, '5', 0, '4', 0, 'd', 0,
    '6', 0, 'e', 0, '8', 0, '4', 0,
    '0', 0, '6', 0, '8', 0, 'f', 0,
};

unsigned char FLASH CdcCtrlInterfaceDesc[] = {  // "CDC-Ctrl"
    18,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    'C', 0, 'D', 0, 'C', 0, '-', 0,
    'C', 0, 't', 0, 'r', 0, 'l', 0,
};

unsigned char FLASH CdcDataInterfaceDesc[] = {  // "CDC-Data"
    18,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    'C', 0, 'D', 0, 'C', 0, '-', 0,
    'D', 0, 'a', 0, 't', 0, 'a', 0,
};

unsigned char FLASH FidoHidInterfaceDesc[] = {  // "FIDO-HID"
    18,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    'F', 0, 'I', 0, 'D', 0, 'O', 0,
    '-', 0, 'H', 0, 'I', 0, 'D', 0,
};

unsigned char FLASH TkeyCtrlInterfaceDesc[] = {  // "TKEY-Ctrl"
    20,    // Length of this descriptor (in bytes)
    0x03,  // Descriptor type (String)
    'T', 0, 'K', 0, 'E', 0, 'Y', 0,
    '-', 0, 'C', 0, 't', 0, 'r', 0,
    'l', 0,
};

#endif
