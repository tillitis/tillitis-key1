#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2021 Mullvad VPN AB <mullvad.se>
# SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

import uuid
import argparse
import encode_usb_strings


magic = encode_usb_strings.string_to_descriptor("68de5d27-e223-4874-bc76-a54d6e84068f")
random_uuid = str(uuid.uuid4())
random_uuid = random_uuid.replace("-", "_")
replacement = encode_usb_strings.string_to_descriptor(random_uuid)


parser = argparse.ArgumentParser(description='CH552 USB serial number injector. Replaces the default UUID with a randomly generated UUID4')
parser.add_argument('-i', required=True, help='input file')
parser.add_argument('-o', required=True, help='output file')
parser.add_argument('-v', required=False, action="store_true", help='verbose, print new uuid')
args = parser.parse_args()

f = bytearray(open(args.i, 'rb').read())

pos = f.find(magic)

if pos < 0:
    print('failed to find magic string')
    exit(1)

f[pos:(pos+len(magic))] = replacement

with open(args.o, 'wb') as of:
    of.write(f)
    
if args.v:
    print(f"{random_uuid}")
