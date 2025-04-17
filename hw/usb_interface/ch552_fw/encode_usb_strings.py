#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2021 Mullvad VPN AB <mullvad.se>
# SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: GPL-2.0-only

def descriptor_to_string(descriptor):
    """ Convert a bytes object containing a USB string descriptor into a python string"""
    bLength = descriptor[0]
    if bLength != len(descriptor):
        raise Exception('Descriptor length mismatch, length_field:{} actual_length:{}'.format(
                bLength,
                len(descriptor)
                ))

    bDescriptorType = descriptor[1]
    if bDescriptorType != 0x03:
        raise Exception('Descriptor type mismatch, bDescriptorType:{02x} expected:0x03'.format(
            bDescriptorType
            ))

    return descriptor[2:].decode('utf-16', errors='strict')

def string_to_descriptor(string):
    """ Convert a python string into a bytes object containing a USB string descriptor"""
    descriptor = bytearray()
    descriptor.append(0x00) # Placeholder for length
    descriptor.append(0x03) # Descriptor type (String)
    descriptor.extend(string.encode('utf-16')[2:]) # crop the BOM
    descriptor[0] = len(descriptor) # Set length of this descriptor (in bytes)

    return bytes(descriptor)

def format_descriptor(name, value):
    descriptor = string_to_descriptor(value)
    formatted = [
        'unsigned char FLASH {}[] = {{  // "{}"'.format(name, value),  # Add string as a comment
        '    {},    // Length of this descriptor (in bytes)'.format(descriptor[0]),
        '    0x03,  // Descriptor type (String)'
    ]

    formatted.extend(
        [
            '    ' + ', '.join(
                ["'{}', 0".format(chr(b)) if b != 0x00 else "0x00" for b in descriptor[2 + i:2 + i + 8:2]]
            ) + ','
            for i in range(0, len(descriptor[2:]), 8)  # 8 bytes = 4 characters
        ]
    )
    formatted.append('};\n')
    return '\n'.join(formatted)

if __name__ == "__main__":
    strings = {
        "ProdDesc": "MTA1-USB-V1",
        "ManufDesc": "Tillitis",
        "SerialDesc": "68de5d27-e223-4874-bc76-a54d6e84068f",
        "CdcCtrlInterfaceDesc": "CDC-Ctrl",
        "CdcDataInterfaceDesc": "CDC-Data",
        "FidoInterfaceDesc": "FIDO",
        "CcidInterfaceDesc": "CCID",
        "DebugInterfaceDesc": "DEBUG"
    }

    with open('inc/usb_strings.h', 'w') as f:
        f.write('#ifndef __USB_STRINGS_H__\n')
        f.write('#define __USB_STRINGS_H__\n')
        f.write('\n')
        f.write('#include "mem.h"\n')
        f.write('\n')

        for name, value in strings.items():
            f.write(format_descriptor(name, value) + '\n')

        f.write('#endif\n')
