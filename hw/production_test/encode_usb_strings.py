#!/usr/bin/env python
"""Convert python strings to UTF16 USB descriptors"""


def descriptor_to_string(descriptor: bytes) -> str:
    """ Convert a USB string descriptor into a python string

    Keyword arguments:
    descriptor -- UTF-16 formatted USB descriptor string
    """
    b_length = descriptor[0]
    if b_length != len(descriptor):
        raise ValueError(
            'Length mismatch, ' +
            f'length_field:{b_length} length:{len(descriptor)}')

    b_descriptor_type = descriptor[1]
    if b_descriptor_type != 0x03:
        raise ValueError(
            f'Type mismatch, bDescriptorType:{b_descriptor_type:02x}'
            + 'expected:0x03')

    return descriptor[2:].decode('utf-16', errors='strict')


def string_to_descriptor(string: str) -> bytes:
    """ Convert a python string into a USB string descriptor

    Keyword arguments:
    string: String to convert
    """
    descriptor = bytearray()
    descriptor.append(0x00)  # placeholder for length
    descriptor.append(0x03)
    descriptor.extend(string.encode('utf-16')[2:])  # crop the BOM
    descriptor[0] = len(descriptor)

    return bytes(descriptor)


if __name__ == "__main__":
    MANUFACTURER = 'Mullvad'
    PRODUCT = 'MTA1-USB-V1'
    SERIAL = "68de5d27-e223-4874-bc76-a54d6e84068f"

    # serial = bytes([
    #    0x14,0x03,
    #    0x32,0x00,0x30,0x00,0x31,0x00,0x37,0x00,0x2D,0x00,
    #    0x32,0x00,0x2D,0x00,
    #    0x32,0x00,0x35,0x00
    #    ])
    # print(descriptor_to_string(serial))

    # sample_product = bytes([
    #    0x14,0x03,
    #    0x43,0x00,0x48,0x00,0x35,0x00,0x35,0x00,0x34,0x00,0x5F,0x00,
    #    0x43,0x00,0x44,0x00,0x43,0x00
    #    ])
    # print(descriptor_to_string(sample_product))
    # rt = string_to_descriptor(descriptor_to_string(sample_product))
    # print(descriptor_to_string(rt))
    #
    # print(['{:02x} '.format(i) for i in sample_product])
    # print(['{:02x} '.format(i) for i in rt])

    # sample_mfr = bytes([
    #    0x0A,0x03,
    #    0x5F,0x6c,0xCF,0x82,0x81,0x6c,0x52,0x60,
    #    ])
    # print(descriptor_to_string(sample_mfr))
    # rt = string_to_descriptor(descriptor_to_string(sample_mfr))
    # print(descriptor_to_string(rt))
    #
    # print(['{:02x} '.format(i) for i in sample_mfr])
    # print(['{:02x} '.format(i) for i in rt])

    with open('usb_strings.h', 'w', encoding='utf-8') as f:
        f.write('#ifndef USB_STRINGS\n')
        f.write('#define USB_STRINGS\n')

        f.write(
            f'unsigned char __code Prod_Des[]={{  // "{PRODUCT}"\n')
        f.write('    ')
        f.write(', '.join([f'0x{i:02x}'
                for i in string_to_descriptor(PRODUCT)]))
        f.write('\n};\n')

        f.write(
            'unsigned char __code Manuf_Des[]={  ' +
            f'// "{MANUFACTURER}"\n')
        f.write('    ')
        f.write(', '.join([f'0x{i:02x}'
                for i in string_to_descriptor(MANUFACTURER)]))
        f.write('\n};\n')

        f.write(
            f'unsigned char __code SerDes[]={{  // "{SERIAL}"\n')
        f.write('    ')
        f.write(', '.join([f'0x{i:02x}'
                for i in string_to_descriptor(SERIAL)]))
        f.write('\n};\n')

        f.write('#endif\n')
