#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#=======================================================================
#
# tpt.py
# ------
# TillitisKey Provisioning Tool.
#
# The tool use HKDF (RFC5869) to generate the UDS.
#
# Copyright (C) 2022 - Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only
#
#=======================================================================

import sys
import argparse
from secrets import token_bytes
from binascii import unhexlify
from hkdf import Hkdf


#-------------------------------------------------------------------
#-------------------------------------------------------------------
def gen_uds(verbose, arg_ent):
    if verbose:
        print("\nGenerating UDS")

    if arg_ent == None:
        ent = str.encode(input("Enter additional entropy: "))
    else:
        ent = str.encode(arg_ent)

    ikm = token_bytes(256)
    my_hkdf = Hkdf(ent, ikm)
    uds = my_hkdf.expand(b"TillitisKey UDS", 32)
    uds_hex = uds.hex()

    if verbose:
        print("\nGenerated UDS:")
        print(uds_hex, "\n")

    return uds_hex


#-------------------------------------------------------------------
#-------------------------------------------------------------------
def save_uds(verbose, uds):
    if verbose:
        print("Writing uds.hex")

    with open ("uds.hex", 'w', encoding = 'utf-8') as uds_file:
        for i in range(8):
            uds_file.write(uds[(i*8) : (i*8 + 8)]+"\n")


#-------------------------------------------------------------------
#-------------------------------------------------------------------
def gen_udi(verbose, pid, vid, rev, serial):
    if verbose:
        print("Generating UDI")

    if vid == None:
        vid = int(input("Enter Vendor ID (0 - 65535): "))

    if pid == None:
        pid = int(input("Enter Product ID (0 - 255): "))

    if rev == None:
        rev = int(input("Enter revision (0 - 15): "))

    if serial == None:
        serial = int(input("Enter serial number (0 - (2**32 -1)): "))

    assert vid < 65536
    assert pid < 256
    assert rev < 16
    assert serial < 2**32

    udi_hex = ("0%04x%02x%1x%08x" % (vid, pid, rev, serial))

    if verbose:
        print("\nGenerated UDI:")
        print(udi_hex, "\n")

    return udi_hex


#-------------------------------------------------------------------
#-------------------------------------------------------------------
def save_udi(verbose, udi):
    if verbose:
        print("Writing udi.hex")

    with open ("udi.hex", 'w', encoding = 'utf-8') as udi_file:
            udi_file.write(udi[0 : 8] + "\n")
            udi_file.write(udi[8 : 16] + "\n")

def enc_str(b):
    return bytestring.decode(sys.getfilesystemencoding())

#-------------------------------------------------------------------
#-------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", help="Verbose operation", action="store_true")
    parser.add_argument("--ent", help="User supplied entropy", type=str)
    parser.add_argument("--vid", help="Vendor id (0 - 65535)",  type=int, default=0)
    parser.add_argument("--pid", help="Product id (0 - 2555", type=int, default=0)
    parser.add_argument("--rev", help="Revision number (0 - 15)", type=int, default=0)
    parser.add_argument("--serial", help="Serial number (0 - (2**31 - 1))", type=int, default=0)
    args = parser.parse_args()

    if args.verbose:
        print("TillitisKey Provisining Tool (TPT)")

    uds = gen_uds(args.verbose, args.ent)
    save_uds(args.verbose, uds)

    udi = gen_udi(args.verbose, args.pid, args.vid, args.rev, args.serial)
    save_udi(args.verbose, udi)


#-------------------------------------------------------------------
#-------------------------------------------------------------------
if __name__=="__main__":
    sys.exit(main())
