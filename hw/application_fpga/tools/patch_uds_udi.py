# -*- coding: utf-8 -*-
#=======================================================================
#
# SPDX-FileCopyrightText: 2023 Tillitis AB <tillitis.se>
# Written by Myrtle Shah <gatecat@ds0.me>
# SPDX-License-Identifier: BSD-2-Clause
#
# Script to patch in a Unique Device Secret (UDS) and a Unique Device
# Identifier (UDI) from files into a bitstream.
#
# It's supposed to be run like this:
#
# nextpnr-ice40 --up5k --package sg48 --ignore-loops \
#    --json application_fpga_par.json --run patch_uds_udi.py
#
# with this environment:
#
# - UDS_HEX: path to the UDS file, typically the path to
#   ../data/uds.hex
# - UDI_HEX: path to the UDI file, typically the path to ../data/udi.hex
# - OUT_ASC: path to the ASC output that is then used by icebram and icepack.
#
# The script changes the UDS and UDI that are stored in named 4-bit
# LUT instances in the JSON file so we can generate device
# unique bitstreams without running the complete flow just to change
# UDS and UDI. Then we can just run the final bitstream generation
# from the ASC file.
#
# We represent our UDI and UDS values as a number of 32 bit words:
#
# - UDI: 2 words.
# - UDS: 8 words.
#
# We reserve 32 named 4-bit LUTs *each* to store the data: UDS in
# "uds_rom_idx" and UDI in "udi_rom_idx".
#
# The script repeats the value in the LUTs so we don't have to care
# about the value of the unused address bits.
#
# See documentation in their implementation in ../core/uds/README.md
# and ../core/tk1/README.md

import os

def parse_hex(file, length):
    data = []
    with open(file, "r") as f:
        for line in f:
            l = line.strip()
            if len(l) > 0:
                data.append(int(l, 16))
    assert len(data) == length, len(data)
    return data

def rewrite_lut(lut, idx, data, has_re=False):
    # each LUT provides one bit per 32-bit word out of 64/256 bits total
    new_init = 0
    for i, word in enumerate(data):
        if (word >> idx) & 0x1:
            # repeat so we don't have to care about inputs above
            # address
            repeat = (16 // len(data))
            for k in range(repeat):
                # UDS also has a read enable
                # LUT output is zero if this isn't asserted
                if has_re and k < (repeat // 2):
                    continue
                new_init |= (1 << (k * len(data) + i))
    lut.setParam("LUT_INIT", f"{new_init:016b}")

uds = parse_hex(os.environ["UDS_HEX"], 8)
udi = parse_hex(os.environ["UDI_HEX"], 2)

uds_lut_count = 0
udi_lut_count = 0

for cell_name, cell in ctx.cells:
    if "uds_rom_idx" in cell.attrs:
        index = int(cell.attrs["uds_rom_idx"], 2)
        rewrite_lut(cell, index, uds, True)
        uds_lut_count += 1
    if "udi_rom_idx" in cell.attrs:
        index = int(cell.attrs["udi_rom_idx"], 2)
        rewrite_lut(cell, index, udi, False)
        udi_lut_count += 1
assert uds_lut_count == 32, uds_lut_count
assert udi_lut_count == 32, udi_lut_count

write_bitstream(ctx, os.environ["OUT_ASC"])
