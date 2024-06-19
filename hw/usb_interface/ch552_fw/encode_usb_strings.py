#!/usr/bin/env python3

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
    descriptor.append(0x00) # placeholder for length
    descriptor.append(0x03)
    descriptor.extend(string.encode('utf-16')[2:]) # crop the BOM
    descriptor[0] = len(descriptor)

    return bytes(descriptor)


if __name__ == "__main__":
    manufacturer = 'Tillitis'
    product = 'MTA1-USB-V1'
    serial = "68de5d27-e223-4874-bc76-a54d6e84068f"

    with open('usb_strings.h', 'w') as f:
        f.write('#ifndef USB_STRINGS\n')
        f.write('#define USB_STRINGS\n')
    
        f.write('unsigned char __code Prod_Des[]={{  // "{}"\n'.format(product))
        f.write('    ')
        f.write(', '.join(['0x{:02x}'.format(i) for i in string_to_descriptor(product)]))
        f.write('\n};\n\n')
        
        f.write('unsigned char __code Manuf_Des[]={{  // "{}"\n'.format(manufacturer))
        f.write('    ')
        f.write(', '.join(['0x{:02x}'.format(i) for i in string_to_descriptor(manufacturer)]))
        f.write('\n};\n\n')
    
        f.write('unsigned char __code SerDes[]={{  // "{}"\n'.format(serial))
        f.write('    ')
        f.write(', '.join(['0x{:02x}'.format(i) for i in string_to_descriptor(serial)]))
        f.write('\n};\n\n')
    
    
        f.write('#endif\n')
