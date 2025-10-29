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
# SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause
#
#=======================================================================

import os
import sys
import argparse
from secrets import token_bytes
from binascii import unhexlify
from hkdf import Hkdf

VID_MAX = 2**16 - 1
PID_MAX = 2**6 - 1
REV_MAX = 2**6 - 1
SERIAL_MAX_EXPR = "(2**32-1)"
SERIAL_MAX = eval(SERIAL_MAX_EXPR)

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
    outpath = os.path.abspath("uds.hex")
    if verbose:
        print("Writing %s" % (outpath))

    with open (outpath, 'w', encoding = 'utf-8') as uds_file:
        for i in range(8):
            uds_file.write(uds[(i*8) : (i*8 + 8)]+"\n")


#-------------------------------------------------------------------
#-------------------------------------------------------------------
def gen_udi(verbose, pid, vid, rev, serial):
    if verbose:
        print("Generating UDI")

    if vid == None:
        vid = int(input("Enter Vendor ID (0 -- %d): " % VID_MAX))

    if pid == None:
        pid = int(input("Enter Product ID (0 -- %d): " % PID_MAX))

    if rev == None:
        rev = int(input("Enter Product Revision (0 -- %d): " % REV_MAX))

    if serial == None:
        serial = int(input("Enter serial number (0 -- %d %s): " % (SERIAL_MAX, SERIAL_MAX_EXPR)))

    assert vid <= VID_MAX
    assert pid <= PID_MAX
    assert rev <= REV_MAX
    assert serial <= SERIAL_MAX

    udi_hex = "0%04x%03x%08x" % (vid, pid << 6 | rev, serial)

    if verbose:
        print("\nGenerated UDI:")
        print(udi_hex, "\n")

    return udi_hex


#-------------------------------------------------------------------
#-------------------------------------------------------------------
def save_udi(verbose, udi):
    outpath = os.path.abspath("udi.hex")
    if verbose:
        print("Writing %s" % (outpath))

    with open (outpath, 'w', encoding = 'utf-8') as udi_file:
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
    parser.add_argument("--vid", help="Vendor ID (0 -- %d)" % VID_MAX, type=int)
    parser.add_argument("--pid", help="Product ID (0 -- %d" % PID_MAX, type=int)
    parser.add_argument("--rev", help="Product Revision (0 -- %d)" % REV_MAX, type=int)
    parser.add_argument("--serial", help="Serial number (0 -- %d %s)" % (SERIAL_MAX, SERIAL_MAX_EXPR), type=int)
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
