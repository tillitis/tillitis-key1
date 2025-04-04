#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2021 Mullvad VPN AB <mullvad.se>
# SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: GPL-2.0-only

import uuid
import argparse
import encode_usb_strings


magic = encode_usb_strings.string_to_descriptor("68de5d27-e223-4874-bc76-a54d6e84068f")
replacement = encode_usb_strings.string_to_descriptor(str(uuid.uuid4()))


parser = argparse.ArgumentParser(description='CH552 USB serial number injector. Replaces the default UUID with a randomly generated UUID4')
parser.add_argument('-i', required=True, help='input file')
parser.add_argument('-o', required=True, help='output file')
args = parser.parse_args()

f = bytearray(open(args.i, 'rb').read())

pos = f.find(magic)

if pos < 0:
    print('failed to find magic string')
    exit(1)

f[pos:(pos+len(magic))] = replacement

with open(args.o, 'wb') as of:
    of.write(f)
    
