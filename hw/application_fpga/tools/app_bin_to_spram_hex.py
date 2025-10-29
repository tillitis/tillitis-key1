#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

import argparse
import sys

arg_parser = argparse.ArgumentParser(
    description=(
        "Converts one TKey app binary into four files suitable to load into SPRAM"
        " during simulation."
    )
)

arg_parser.add_argument("app_bin")
arg_parser.add_argument("output_spram0_hex")
arg_parser.add_argument("output_spram1_hex")
arg_parser.add_argument("output_spram2_hex")
arg_parser.add_argument("output_spram3_hex")

args = arg_parser.parse_args()


def abort(msg: str, exitcode: int):
    sys.stderr.write(msg + "\n")
    sys.exit(exitcode)


with (
    open(args.app_bin, "rb") as app,
    open(args.output_spram0_hex, "w") as spram0,
    open(args.output_spram1_hex, "w") as spram1,
    open(args.output_spram2_hex, "w") as spram2,
    open(args.output_spram3_hex, "w") as spram3,
):

    # For debug
    # app = open("app.bin", "rb")
    # spram0 = open("output_spram0_hex", "w")
    # spram1 = open("output_spram1_hex", "w")
    # spram2 = open("output_spram2_hex", "w")
    # spram3 = open("output_spram3_hex", "w")

    SPRAM_SIZE_BYTES = int(256 * 1024 / 8)
    RAM_SIZE_BYTES = SPRAM_SIZE_BYTES * 4

    rows, cols = int(SPRAM_SIZE_BYTES / 2), 2
    ram0 = [[0] * cols for _ in range(rows)]
    ram1 = [[0] * cols for _ in range(rows)]
    ram2 = [[0] * cols for _ in range(rows)]
    ram3 = [[0] * cols for _ in range(rows)]

    # Input data is little endian
    input_data_le = app.read()

    if len(input_data_le) % 2 != 0:
        abort("Error: File size not a multiple of 2 bytes", -1)

    if len(input_data_le) > RAM_SIZE_BYTES:
        abort("Error: File does not fit in RAM", -1)

    # Zero-pad input to RAM size
    input_data_le += b"\x00" * (RAM_SIZE_BYTES - len(input_data_le))

    # Set ram_addr_rand and ram_data_rand to the values the simulated TRNG
    # will deliver.
    ram_addr_rand = 0xDEADBEEF
    ram_data_rand = 0xBD5B7DDE

    # Convert RAM
    cpu_addr_data_le = input_data_le[0:RAM_SIZE_BYTES]

    for cpu_addr in range(0, int(RAM_SIZE_BYTES / 4), 4):

        # Data is little endian
        word_data_le = cpu_addr_data_le[cpu_addr : cpu_addr + 4]

        # Get ram address
        ram_addr = cpu_addr >> 2

        # Convert ram_addr to little endian for calculation
        ram_addr_le = ram_addr.to_bytes(4, byteorder="little", signed=False)

        # Convert ram_data_rand to little endian for calculation
        ram_data_rand_le = ram_data_rand.to_bytes(4, byteorder="little", signed=False)

        # Concatenate two ram_addr and convert to little endian for calculation
        ram_addr_double = ((ram_addr << 16) & 0xFFFF0000) | (ram_addr & 0x0000FFFF)
        ram_addr_double_le = ram_addr_double.to_bytes(
            4, byteorder="little", signed=False
        )

        # XOR word_data_le, ram_data_rand_le and ram_addr_le
        #    Explanation:
        #    1. Zip the byte strings: zip(word_data_le, ram_data_rand_le, ram_addr_double_le) creates
        #       an iterator of tuples with corresponding elements from the byte strings.
        #    2. Apply XOR: Use a generator expression (a ^ b ^ c for a, b, c in ...) to XOR the values in each tuple.
        #    3. Convert back to bytes: The bytes constructor collects the XORed values into a new byte string.
        scrambled_word_data_le = bytes(
            a ^ b ^ c
            for a, b, c in zip(word_data_le, ram_data_rand_le, ram_addr_double_le)
        )

        # Convert ram_addr_rand to little endian for calculation
        ram_addr_rand_le = ram_addr_rand.to_bytes(4, byteorder="little", signed=False)

        # XOR ram_addr_le and ram_addr_rand_le
        scrambled_ram_addr_le = bytes(
            a ^ b for a, b in zip(ram_addr_le, ram_addr_rand_le)
        )

        # Get scrambled_ram_addr as int
        scrambled_ram_addr = int.from_bytes(
            scrambled_ram_addr_le, byteorder="little", signed=False
        )

        # In hardware the highest bits are discarded since the memory is only 128K bytes
        scrambled_ram_addr = scrambled_ram_addr & 0x7FFF

        # Check bit 14 for which pair of ram blocks the data should be stored in
        if not (scrambled_ram_addr & 0x4000):
            ram0[scrambled_ram_addr] = (
                scrambled_word_data_le[0],
                scrambled_word_data_le[1],
            )
            ram1[scrambled_ram_addr] = (
                scrambled_word_data_le[2],
                scrambled_word_data_le[3],
            )
        else:
            ram2[scrambled_ram_addr] = (
                scrambled_word_data_le[0],
                scrambled_word_data_le[1],
            )
            ram3[scrambled_ram_addr] = (
                scrambled_word_data_le[2],
                scrambled_word_data_le[3],
            )

    for line in range(0, rows, 1):
        spram0.write(f"{ram0[line][1]:02x}")
        spram0.write(f"{ram0[line][0]:02x}\n")
        spram1.write(f"{ram1[line][1]:02x}")
        spram1.write(f"{ram1[line][0]:02x}\n")
        spram2.write(f"{ram2[line][1]:02x}")
        spram2.write(f"{ram2[line][0]:02x}\n")
        spram3.write(f"{ram3[line][1]:02x}")
        spram3.write(f"{ram3[line][0]:02x}\n")
