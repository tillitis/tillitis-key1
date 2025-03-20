#!/usr/bin/env python3

import argparse

FLASH_SIZE_BYTES = 1 * 1024 * 1024
PRELOADED_APP_0_START = 0x30000


def run(output_path, preloaded_app_0_path):
    with (
        open(output_path, "wb") as output_file,
        open(preloaded_app_0_path, "rb") as preloaded_app_0_file,
    ):
        content = bytearray(b"\xFF") * FLASH_SIZE_BYTES
        preloaded_app = preloaded_app_0_file.read()
        content[
            PRELOADED_APP_0_START : PRELOADED_APP_0_START + len(preloaded_app)
        ] = preloaded_app
        output_file.write(content)


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("OUTPUT_PATH")
    arg_parser.add_argument("PRELOADED_APP_0_PATH")
    args = arg_parser.parse_args()

    run(args.OUTPUT_PATH, args.PRELOADED_APP_0_PATH)
