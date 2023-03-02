#!/usr/bin/env python

import sys

#
# bistream to NVCM command conversion is based on majbthrd's work in
# https://github.com/YosysHQ/icestorm/pull/272
#


def icebin2nvcm(bitstream: bytes) -> list[str]:
    # ensure that the file starts with the correct bistream preamble
    for origin in range(0, len(bitstream)):
        if bitstream[origin:origin + 4] == bytes.fromhex('7EAA997E'):
            break

    if origin == len(bitstream):
        raise Exception("Preamble not found")

    print("Found preamable at %08x" % (origin), file=sys.stderr)

    # there might be stuff in the header with vendor tools,
    # but not usually in icepack produced output, so ignore it for now

    # todo: what is the correct size?
    print(len(bitstream))

    rows = []

    rows.append('06')

    for pos in range(origin, len(bitstream), 8):
        row = bitstream[pos:pos + 8]

        # pad out to 8-bytes
        row += b'\0' * (8 - len(row))

        if row == bytes(8):
            # skip any all-zero entries in the bistream
            continue

        # NVCM addressing is very weird
        addr = pos - origin
        nvcm_addr = int(addr / 328) * 4096 + (addr % 328)

        row_str = "02%06x%s" % (nvcm_addr, row.hex())
        row_str = ' '.join([row_str[i:i + 2]
                            for i in range(0, len(row_str), 2)]) + ' '

        rows.append(row_str)

    rows.append('04')

    return rows


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()

    parser.add_argument('infile',
                        type=str,
                        help='input bin file')

    parser.add_argument('outfile',
                        type=str,
                        help='output nvcm file')

    args = parser.parse_args()

    with open(args.infile, 'rb') as f_in:
        bitstream = f_in.read()

    cmds = icebin2nvcm(bitstream)

    with open(args.outfile, 'w') as f_out:
        for cmd in cmds:
            f_out.write(cmd)
            f_out.write('\n')
