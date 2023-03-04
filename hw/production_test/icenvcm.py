#!/usr/bin/env python
#
#  Copyright (C) 2021
#
#  * Trammell Hudson <hudson@trmm.net>
#  * Matthew Mets https://github.com/cibomahto
#  * Peter Lawrence https://github.com/majbthrd
#
#  Permission to use, copy, modify, and/or distribute this software
#  for any purpose with or without fee is hereby granted, provided
#  that the above copyright notice and this permission notice
#  appear in all copies.
#
#  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
#  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
#  AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
#  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
#  NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
#  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

import usb_test
from icebin2nvcm import icebin2nvcm
import sys
from time import sleep
import os


class Nvcm():
    # todo: add expected bitstream sizes
    id_table = {
        0x06: "ICE40LP8K / ICE40HX8K",
        0x07: "ICE40LP4K / ICE40HX4K",
        0x08: "ICE40LP1K / ICE40HX1K",
        0x09: "ICE40LP384",
        0x0E: "ICE40LP1K_SWG16",
        0x0F: "ICE40LP640_SWG16",
        0x10: "ICE5LP1K",
        0x11: "ICE5LP2K",
        0x12: "ICE5LP4K",
        0x14: "ICE40UL1K",
        0x15: "ICE40UL640",
        0x20: "ICE40UP5K",
        0x21: "ICE40UP3K",
    }

    def __init__(
            self,
            pins: dict,
            spi_speed: int = 12,
            debug: bool = False) -> None:
        self.pins = pins
        self.debug = debug

        self.flasher = usb_test.ice40_flasher()

        self.flasher.gpio_put(self.pins['5v_en'], False)
        self.flasher.gpio_put(self.pins['crst'], False)

        # Configure pins for talking to ice40
        self.flasher.gpio_set_direction(pins['ss'], True)
        self.flasher.gpio_set_direction(pins['mosi'], True)
        self.flasher.gpio_set_direction(pins['sck'], True)
        self.flasher.gpio_set_direction(pins['miso'], False)
        self.flasher.gpio_set_direction(pins['5v_en'], True)
        self.flasher.gpio_set_direction(pins['crst'], True)
        self.flasher.gpio_set_direction(pins['cdne'], False)

        self.flasher.spi_pins_set(
            pins['sck'],
            pins['ss'],
            pins['mosi'],
            pins['miso'],
            spi_speed
        )

    def power_on(self) -> None:
        self.flasher.gpio_put(self.pins['5v_en'], True)

    def power_off(self) -> None:
        self.flasher.gpio_put(self.pins['5v_en'], False)

    def enable(self, cs: bool, reset: bool = True) -> None:
        # gpio.write(cs << cs_pin | reset << reset_pin)
        self.flasher.gpio_put(self.pins['ss'], cs)
        self.flasher.gpio_put(self.pins['crst'], reset)

    def sendhex_cs(self, s: str, toggle_cs: bool = True) -> bytes:
        if self.debug and not s == "0500":
            print("TX", s)
        x = bytes.fromhex(s)

        # b = dev.exchange(x, duplex=True, readlen=len(x))
        b = self.flasher.spi_bitbang(x, toggle_cs)

        if self.debug and not s == "0500":
            print("RX", b.hex())
        return b

    def delay(self, count: int) -> None:
        # run the clock with no CS asserted
        # dev.exchange(b'\x00', duplex=True, readlen=count)
        self.sendhex_cs('00' * count, False)

    def tck(self, count: int) -> None:
        self.delay((count >> 3) * 2)

    def init(self) -> None:
        if self.debug:
            print("init")
        self.enable(True, True)
        self.enable(True, False)  # reset high
        sleep(0.15)

        self.enable(False, False)  # enable and reset high
        sleep(0.12)
        self.enable(False, True)  # enable low, reset high
        sleep(0.12)
        self.enable(True, True)  # enable and reset low
        sleep(0.12)

    def status_wait(self, count: int = 1000) -> None:
        for i in range(0, count):
            self.tck(5000)
            ret = self.sendhex_cs("0500")
            x = int.from_bytes(ret, byteorder='big')

            # print("x=%04x" %(x))

            if (x & 0x00c1) == 0:
                return

        raise Exception("status failed to clear")

    def command(self, cmd: str) -> None:
        self.sendhex_cs(cmd)
        self.status_wait()
        self.tck(8)

    def pgm_enable(self) -> None:
        self.command("06")

    def pgm_disable(self) -> None:
        self.command("04")

    def enable_access(self) -> None:
        # ! Shift in Access-NVCM instruction;
        # SMCInstruction[1] = 0x70807E99557E;
        self.command("7eaa997e010e")

    def read_bytes(
            self,
            address: int,
            length: int = 8,
            cmd: int = 0x03) -> bytes:
        """Returns a byte array of the contents"""
        msg = ''
        msg += ("%02x%06x" % (cmd, address))
        msg += ("00" * 9)  # dummy bytes
        msg += ("00" * length)  # read
        ret = self.sendhex_cs(msg)

        return ret[4 + 9:]

    def read(
            self,
            address: int,
            length: int = 8,
            cmd: int = 0x03) -> int:
        """Returns a big integer"""

        val = self.read_bytes(address, length, cmd)
        x = 0
        for i in range(0, length):
            x = x << 8 | val[i]
        return x

    def write(self, address: int, data: str, cmd: int = 0x02) -> None:
        self.sendhex_cs("%02x%06x" % (cmd, address) + data)

        try:
            self.status_wait()
        except Exception as e:
            raise Exception(
                "WRITE FAILED: cmd=%02x address=%06x data=%s" %
                (cmd, address, data))

        self.tck(8)

    def bank_select(self, bank: int) -> None:
        self.write(cmd=0x83, address=0x000025, data="%02x" % (bank))

    def select_nvcm(self) -> None:
        # ! Shift in Restore Access-NVCM instruction;
        # SDR 40 TDI(0x00A40000C1);
        self.bank_select(0x00)

    def select_trim(self) -> None:
        # ! Shift in Trim setup-NVCM instruction;
        # SDR 40 TDI(0x08A40000C1);
        self.bank_select(0x10)

    def select_sig(self) -> None:
        # ! Shift in Access Silicon Signature instruction;
        # IDInstruction[1] = 0x04A40000C1;
        # SDR 40 TDI(IDInstruction[1]);
        self.bank_select(0x20)

    def read_trim(self) -> int:
        # ! Shift in Access-NVCM instruction;
        # SMCInstruction[1] = 0x70807E99557E;
        self.enable_access()

        # ! Shift in READ_RF(0x84) instruction;
        # SDR 104 TDI(0x00000000000000000004000021);
        x = self.read(cmd=0x84, address=0x000020, length=8)
        self.tck(8)

        # print("FSM Trim Register %x" % (x))

        self.select_nvcm()
        return x

    def write_trim(self, data: str) -> None:
        # ! Setup Programming Parameter in Trim Registers;
        # ! Shift in Trim setup-NVCM instruction;
        # TRIMInstruction[1] = 0x000000430F4FA80004000041;
        self.write(cmd=0x82, address=0x000020, data=data)

    def nvcm_enable(self) -> None:
        if self.debug:
            print("enable")
        # ! Shift in Access-NVCM instruction;
        # SMCInstruction[1] = 0x70807E99557E;
        self.enable_access()

        # ! Setup Reading Parameter in Trim Registers;
        # ! Shift in Trim setup-NVCM instruction;
        # TRIMInstruction[1] = 0x000000230000000004000041;
        if self.debug:
            print("setup_nvcm")
        self.write_trim("00000000c4000000")

    def enable_trim(self) -> None:
        # ! Setup Programming Parameter in Trim Registers;
        # ! Shift in Trim setup-NVCM instruction;
        # TRIMInstruction[1] = 0x000000430F4FA80004000041;
        self.write_trim("0015f2f0c2000000")

    def trim_blank_check(self) -> bool:
        print("NVCM Trim_Parameter_OTP blank check")

        self.select_trim()

        x = self.read(0x000020, 1)
        self.select_nvcm()

        if x != 0:
            print(
                "NVCM Trim_Parameter_OTP Block is not blank. (%02x)" %
                x)
            return False
        return True

    def blank_check(self, total_fuse: int) -> bool:
        self.select_nvcm()

        status = True
        print("NVCM main memory blank check")
        contents = self.read_bytes(0x000000, total_fuse)

        for i in range(0, total_fuse):
            x = contents[i]
            if self.debug:
                print("%08x: %02x" % (i, x))
            if x != 0:
                print(
                    "%08x: NVCM Main Memory Block is not blank." %
                    (i), file=sys.stderr)
                status = False

        self.select_nvcm()
        return status

    def program(self, rows: list[str]) -> None:
        print("NVCM Program main memory")

        self.select_nvcm()

        self.enable_trim()

        self.pgm_enable()

        status = True

        i = 0
        for row in rows:
            # print('data for row:',i, row)
            if i % 1024 == 0:
                print("%6d / %6d bytes" % (i, len(rows) * 8))
            i += 8
            try:
                self.command(row)
            except Exception as e:
                raise Exception("programming failed, row:", row)

        self.pgm_disable()

    def write_trim_pages(self, lock_bits: str) -> None:
        self.select_nvcm()

        self.enable_trim()

        self.select_trim()

        self.pgm_enable()

        # ! Program Security Bit row 1;
        # ! Shift in PAGEPGM instruction;
        # SDR 96 TDI(0x000000008000000C04000040);
        # ! Program Security Bit row 2;
        # SDR 96 TDI(0x000000008000000C06000040);
        # ! Program Security Bit row 3;
        # SDR 96 TDI(0x000000008000000C05000040);
        # ! Program Security Bit row 4;
        # SDR 96 TDI(0x00000000800000C07000040);
        self.write(0x000020, lock_bits)
        self.write(0x000060, lock_bits)
        self.write(0x0000a0, lock_bits)
        self.write(0x0000e0, lock_bits)

        self.pgm_disable()

        # verify a read back
        x = self.read(0x000020, 8)

        self.select_nvcm()

        lock_bits_int = int(lock_bits, 16)
        if x & lock_bits_int != lock_bits_int:
            raise Exception(
                "Failed to write trim lock bits: " +
                "%016x != expected %016x" %
                (x, lock_bits_int))

        print("New state %016x" % (x))

    def trim_secure(self) -> None:
        print("NVCM Secure")
        trim = self.read_trim()
        if (trim >> 60) & 0x3 != 0:
            print(
                "NVCM already secure? trim=%016x" %
                (trim), file=sys.stderr)

        self.write_trim_pages("3000000100000000")

    def trim_program(self) -> None:
        print("NVCM Program Trim_Parameter_OTP")
        self.write_trim_pages("0015f2f1c4000000")

    def info(self) -> None:
        """ Print the contents of the configuration registers """
        self.select_sig()
        sig1 = self.read(0x000000, 8)

        self.select_sig()
        sig2 = self.read(0x000008, 8)

        # have to switch back to nvcm bank before switching to trim?
        self.select_nvcm()
        trim = self.read_trim()

#        self.select_nvcm()

        self.select_trim()
        trim0 = self.read(0x000020, 8)

        self.select_trim()
        trim1 = self.read(0x000060, 8)

        self.select_trim()
        trim2 = self.read(0x0000a0, 8)

        self.select_trim()
        trim3 = self.read(0x0000e0, 8)

        self.select_nvcm()

        secured = ((trim >> 60) & 0x3)
        device_id = (sig1 >> 56) & 0xFF

        print("Device: %s (%02x) secure=%d" % (
            self.id_table.get(device_id, "Unknown"),
            device_id,
            secured
        ))
        print("Sig  0: %016x" % (sig1))
        print("Sig  1: %016x" % (sig2))

        print("TrimRF: %016x" % (trim))
        print("Trim 0: %016x" % (trim0))
        print("Trim 1: %016x" % (trim1))
        print("Trim 2: %016x" % (trim2))
        print("Trim 3: %016x" % (trim3))

    def read_nvcm(self, length: int) -> bytes:
        """ Read out the contents of the NVCM fuses

        Keyword arguments:
        length: Length of data to read
        """

        self.select_nvcm()

        contents = bytearray()

        for offset in range(0, length, 8):
            if offset % 1024 == 0:
                print("%6d / %6d bytes" % (offset, length))

            nvcm_addr = int(offset / 328) * 4096 + (offset % 328)
            contents += self.read_bytes(nvcm_addr, 8)
            self.tck(8)

        return bytes(contents)

    def read_file(self, filename: str, length: int) -> None:
        """ Read the contents of the NVCM to a file

        Keyword arguments:
        filename -- File to write to, or '-' to write to stdout
        """

        contents = bytearray()

        # prepend a header to the file, to identify it as an FPGA
        # bitstream
        contents += bytes([0xff, 0x00, 0x00, 0xff])

        contents += self.read_nvcm(length)

        if filename == '-':
            with os.fdopen(sys.stdout.fileno(),
                           "wb",
                           closefd=False) as f:
                f.write(contents)
                f.flush()
        else:
            with open(filename, "wb") as f:
                f.write(contents)
                f.flush()

    def verify(self, filename: str) -> None:
        """ Verify that the contents of the NVCM match a file

        Keyword arguments:
        filename -- File to compare
        """
        with open(filename, "rb") as f:
            compare = f.read()

        assert (len(compare) > 0)

        contents = bytearray()
        contents += bytes([0xff, 0x00, 0x00, 0xff])
        contents += self.read_nvcm(len(compare))

        # We might have read more than needed because of read
        # boundaries
        if len(contents) > len(compare):
            contents = contents[:len(compare)]

        assert (compare == contents)


def sleep_flash(pins: dict) -> None:
    """ Put the SPI bootloader flash in deep sleep mode"""
    flasher = usb_test.ice40_flasher()

    # Disable board power
    flasher.gpio_put(pins['5v_en'], False)
    flasher.gpio_set_direction(pins['5v_en'], True)

    # Pull CRST low to prevent FPGA from starting
    flasher.gpio_set_direction(pins['crst'], True)
    flasher.gpio_put(pins['crst'], False)

    # Enable board power
    flasher.gpio_put(pins['5v_en'], True)

    # Configure pins for talking to flash
    flasher.gpio_set_direction(pins['ss'], True)
    flasher.gpio_set_direction(pins['mosi'], False)
    flasher.gpio_set_direction(pins['sck'], True)
    flasher.gpio_set_direction(pins['miso'], True)

    flasher.spi_pins_set(
        pins['sck'],
        pins['ss'],
        pins['miso'],
        pins['mosi'],
        12
    )

    flasher.spi_bitbang(bytes([0xAB]))

    # Confirm we can talk to flash
    data = flasher.spi_bitbang(bytes([0x9f, 0, 0]))

    print('flash ID while awake:', ' '.join(
        ['{:02x}'.format(b) for b in data]))
    assert (data == bytes([0xff, 0xef, 0x40]))

    # Test that the flash will ignore a sleep command that doesn't
    # start on the first byte
    flasher.spi_bitbang(bytes([0, 0xb9]))

    # Confirm we can talk to flash
    data = flasher.spi_bitbang(bytes([0x9f, 0, 0]))

    print('flash ID while awake:', ' '.join(
        ['{:02x}'.format(b) for b in data]))
    assert (data == bytes([0xff, 0xef, 0x40]))

    # put the flash to sleep
    flasher.spi_bitbang(bytes([0xb9]))

    # Confirm flash is asleep
    data = flasher.spi_bitbang(bytes([0x9f, 0, 0]))

    print('flash ID while asleep:', ' '.join(
        ['{:02x}'.format(b) for b in data]))
    assert (data == bytes([0xff, 0xff, 0xff]))


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()

    parser.add_argument('--port',
                        type=str,
                        default='ftdi://::/1',
                        help='FTDI port of the form ftdi://::/1')

    parser.add_argument(
        '-v',
        '--verbose',
        dest='verbose',
        action='store_true',
        help='Show debug information and serial read/writes')

    parser.add_argument(
        '-f',
        '--sleep_flash',
        dest='sleep_flash',
        action='store_true',
        help='Put an attached SPI flash chip in deep sleep')

    parser.add_argument(
        '-b',
        '--boot',
        dest='do_boot',
        action='store_true',
        help='Deassert the reset line to allow the FPGA to boot')

    parser.add_argument('-i', '--info',
                        dest='read_info',
                        action='store_true',
                        help='Read chip ID, trim and other info')

    parser.add_argument('--read',
                        dest='read_file',
                        type=str,
                        default=None,
                        help='Read contents of NVCM')

    parser.add_argument('--verify',
                        dest='verify_file',
                        type=str,
                        default=None,
                        help='Verify the contents of NVCM')

    parser.add_argument(
        '--write',
        dest='write_file',
        type=str,
        default=None,
        help='bitstream file to write to NVCM ' +
        '(warning: not reversable!)')

    parser.add_argument('--ignore-blank',
                        dest='ignore_blank',
                        action='store_true',
                        help='Proceed even if the chip is not blank')

    parser.add_argument(
        '--secure',
        dest='set_secure',
        action='store_true',
        help='Set security bits to prevent modification ' +
        '(warning: not reversable!)')

    parser.add_argument(
        '--my-design-is-good-enough',
        dest='good_enough',
        action='store_true',
        help='Enable the dangerous commands --write and --secure')

    args = parser.parse_args()

    if not args.good_enough \
            and (args.write_file or args.set_secure):
        print(
            "Are you sure your design is good enough?",
            file=sys.stderr)
        exit(1)

    tp1_pins = {
        '5v_en': 7,
        'sck': 10,
        'mosi': 11,
        'ss': 12,
        'miso': 13,
        'crst': 14,
        'cdne': 15
    }

    if args.sleep_flash:
        sleep_flash(tp1_pins)

    nvcm = Nvcm(tp1_pins, debug=args.verbose)
    nvcm.power_on()

    # # Turn on ICE40 in CRAM boot mode
    nvcm.init()
    nvcm.nvcm_enable()

    if args.read_info:
        nvcm.info()

    if args.write_file:
        with open(args.write_file, "rb") as f:
            bitstream = f.read()
        print("read %d bytes" % (len(bitstream)))
        cmds = icebin2nvcm(bitstream)
        if not cmds:
            exit(1)

        if not args.ignore_blank:
            nvcm.trim_blank_check() or exit(1)
            # how much should we check?
            nvcm.blank_check(0x100) or exit(1)

        # this is it!
        nvcm.program(cmds)

        # update the trim to boot from nvcm
        nvcm.trim_program()

    if args.read_file:
        # read back after writing to the NVCM
        nvcm.read_file(args.read_file, 104090)

    if args.verify_file:
        # read back after writing to the NVCM
        nvcm.verify(args.verify_file)

    if args.set_secure:
        nvcm.trim_secure()

    if args.do_boot:
        # hold reset low for half a second
        nvcm.enable(True, False)
        sleep(0.5)
        nvcm.enable(True, True)
