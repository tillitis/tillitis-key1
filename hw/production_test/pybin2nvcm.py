#!/usr/bin/env python
""" bistream to NVCM command conversion is based on majbthrd's work
in https://github.com/YosysHQ/icestorm/pull/272
"""


def pybin2nvcm(bitstream: bytes) -> list[str]:
    """Convert an ice40 bitstream into an NVCM program

    The NVCM format is a set of commands that are run against the
    NVCM state machine, which instruct the state machine to write
    the bitstream into the NVCM. It's somewhat convoluted!

    Keyword arguments:
    bitstream -- Bitstream to convert into NVCM format
    """

    # ensure that the file starts with the correct bistream preamble
    for origin in range(0, len(bitstream)):
        if bitstream[origin:origin + 4] == bytes.fromhex('7EAA997E'):
            break

    if origin == len(bitstream):
        raise ValueError('Preamble not found')

    print(f'Found preamable at {origin:08x}')

    # there might be stuff in the header with vendor tools,
    # but not usually in icepack produced output, so ignore it for now

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

        row_str = f'02{nvcm_addr:06x}{row.hex()}'
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
        data = f_in.read()

    cmds = pybin2nvcm(data)

    with open(args.outfile, 'w', encoding='utf-8') as f_out:
        for cmd in cmds:
            f_out.write(cmd)
            f_out.write('\n')
