# -*- coding: utf-8 -*-
#=======================================================================
#
# Copyright (C) 2023 Tillitis AB
# Written by Myrtle Shah <gatecat@ds0.me>
# SPDX-License-Identifier: GPL-2.0-only
#
# patch_uds_udi.py
# --------------
# Python program that patches the UDS and UDI implemented using
# named LUT4 instances to have unique initial values, not the generic
# values used during synthesis, p&r and mapping. This allows us to
# generate device unique bitstreams without running the complete flow.
#
# Both the UDI and UDS are using bit indexing from 32 LUTs for each
# word, i.e., the first word consists of bit 0 from each 32 LUTs and
# so on.
#
# The size requirements for the UDI and UDS are specified as 1 bit (8
# bytes of data) and 3 bits (32 bytes of data), respectively. The UDI
# does not occupy the entire LUT4 instance, and to conserve resources,
# the pattern of the UDI is repeated over the unused portion of the
# LUT4 instance. This eliminates the need to drive the three MSB pins
# while still achieving the correct output.
#
# In the case of UDS, a read-enable signal is present, and the most
# significant bit serves as the read-enable input. This requires the
# lower half of initialization bits to be forced to zero, ensuring
# that the memory outputs zero when the read-enable signal is
# inactive.
#
#
#=======================================================================

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
            # repeat so inputs above address have a don't care value
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
