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
"""NVCM programming tool for iCE40 FPGAs"""

import os
import sys
import struct
from time import sleep
from usb_test import IceFlasher
from icebin2nvcm import icebin2nvcm


class Nvcm():
    """NVCM programming interface for ICE40 FPGAs"""
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

    banks = {
        'nvcm': 0x00,
        'trim': 0x10,
        'sig': 0x20
    }

    def __init__(
            self,
            pins: dict,
            spi_speed: int = 12,
            debug: bool = False) -> None:
        self.pins = pins
        self.debug = debug

        self.flasher = IceFlasher()

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
        """Disable power to the DUT"""
        self.flasher.gpio_put(self.pins['5v_en'], True)

    def power_off(self) -> None:
        """Enable power to the DUT"""
        self.flasher.gpio_put(self.pins['5v_en'], False)

    def enable(self, cs: bool, reset: bool = True) -> None:
        """Set the CS and Reset pin states"""
        self.flasher.gpio_put(self.pins['ss'], cs)
        self.flasher.gpio_put(self.pins['crst'], reset)

    def writehex(self, s: str, toggle_cs: bool = True) -> None:
        """Write SPI data to the target device

        Keyword arguments:
        s -- data to send (formatted as a string of hex data)
        toggle_cs -- If true, automatically lower the CS pin before
                     transmit, and raise it after transmit
        """
        if self.debug and not s == "0500":
            print("TX", s)
        data = bytes.fromhex(s)

        self.flasher.spi_write(data, toggle_cs)

    def sendhex(self, s: str, toggle_cs: bool = True) -> bytes:
        """Perform a full-duplex write/read on the target device

        Keyword arguments:
        s -- data to send (formatted as a string of hex data)
        toggle_cs -- If true, automatically lower the CS pin before
                     transmit, and raise it after transmit
        """
        if self.debug and not s == "0500":
            print("TX", s)
        x = bytes.fromhex(s)

        b = self.flasher.spi_rxtx(x, toggle_cs)

        if self.debug and not s == "0500":
            print("RX", b.hex())
        return b

    def delay(self, count: int) -> None:
        """'Delay' by sending clocks with CS de-asserted

        Keyword arguments:
        count -- Number of bytes to clock
        """

        self.flasher.spi_clk_out(count)

    def init(self) -> None:
        """Reboot the part and enter SPI command mode"""
        if self.debug:
            print("init")
        self.enable(cs=True, reset=True)
        self.enable(cs=True, reset=False)
        self.enable(cs=False, reset=False)
        self.enable(cs=False, reset=True)
        sleep(0.1)
        self.enable(cs=True, reset=True)

    def status_wait(self) -> None:
        """Wait for the status register to clear"""
        for i in range(0, 1000):
            self.delay(1250)
            ret = self.sendhex("0500")
            status = struct.unpack('>H', ret)[0]

            if (status & 0x00c1) == 0:
                return

        raise ValueError("status failed to clear")

    def command(self, cmd: str) -> None:
        """Send a command to the NVCM state machine"""
        self.writehex(cmd)
        self.status_wait()
        self.delay(2)

    def pgm_enable(self) -> None:
        """Enable program mode"""
        self.command("06")

    def pgm_disable(self) -> None:
        """Disable program mode"""
        self.command("04")

    def enable_access(self) -> None:
        """Send the 'access NVCM' instruction"""
        self.command("7eaa997e010e")

    def read_bytes(
            self,
            cmd: int,
            address: int,
            length: int = 8) -> bytes:
        """Read NVCM memory and return as a byte array

        Known read commands are:
        0x03: Read NVCM bank
        0x84: Read RF

        Keyword arguments:
        cmd -- Read command
        address -- NVCM memory address to read from
        length -- Number of bytes to read
        """

        msg = ''
        msg += ("%02x%06x" % (cmd, address))
        msg += ("00" * 9)  # dummy bytes
        msg += ("00" * length)  # read
        ret = self.sendhex(msg)

        return ret[4 + 9:]

    def read_int(
            self,
            cmd: int,
            address: int) -> int:
        """Read NVCM memory and return as an integer

        Read commands are documented in read_bytes

        Keyword arguments:
        cmd -- Read command
        address -- NVCM memory address to read from
        """

        val = self.read_bytes(cmd, address, 8)
        return struct.unpack('>Q', val)[0]

    def write(self, cmd: int, address: int, data: str) -> None:
        """Write data to the NVCM memory

        Keyword arguments:
        cmd -- Write command
        address -- NVCM memory address to write to
        length -- Number of bytes to write
        """
        self.writehex("%02x%06x" % (cmd, address) + data)

        try:
            self.status_wait()
        except Exception as exc:
            raise Exception(
                "WRITE FAILED: cmd=%02x address=%06x data=%s" %
                (cmd, address, data)) from exc

        self.delay(2)

    def bank_select(self, bank: str) -> None:
        """ Select the active NVCM bank to target

        Keyword arguments:
        bank -- NVCM bank: nvcm, trim, or sig
        """

        self.write(0x83, 0x000025, f"{self.banks[bank]:02x}")

    def read_trim(self) -> int:
        """Read the RF trim register"""
        self.enable_access()

        # ! Shift in READ_RF(0x84) instruction;
        # SDR 104 TDI(0x00000000000000000004000021);
        val = self.read_int(0x84, 0x000020)
        self.delay(2)

        # print("FSM Trim Register %x" % (x))

        self.bank_select('nvcm')
        return val

    def write_trim(self, data: str) -> None:
        """Write to the RF trim register

        Keyword arguments:
        data -- Hex-formatted string, should be 8 bytes of data
        """
        # ! Setup Programming Parameter in Trim Registers;
        # ! Shift in Trim setup-NVCM instruction;
        # TRIMInstruction[1] = 0x000000430F4FA80004000041;
        self.write(0x82, 0x000020, data)

    def nvcm_enable(self) -> None:
        """Enable NVCM interface by sending knock command"""
        if self.debug:
            print("enable")
        self.enable_access()

        # ! Setup Reading Parameter in Trim Registers;
        # ! Shift in Trim setup-NVCM instruction;
        # TRIMInstruction[1] = 0x000000230000000004000041;
        if self.debug:
            print("setup_nvcm")
        self.write_trim("00000000c4000000")

    def enable_trim(self) -> None:
        """Enable NVCM write commands"""
        # ! Setup Programming Parameter in Trim Registers;
        # ! Shift in Trim setup-NVCM instruction;
        # TRIMInstruction[1] = 0x000000430F4FA80004000041;
        self.write_trim("0015f2f0c2000000")

    def trim_blank_check(self) -> None:
        """Check that the NVCM trim parameters are blank"""

        print("NVCM Trim_Parameter_OTP blank check")

        self.bank_select('trim')

        ret = self.read_bytes(0x03, 0x000020, 1)[0]
        self.bank_select('nvcm')

        if ret != 0x00:
            raise ValueError(
                'NVCM Trim_Parameter_OTP Block not blank. ' +
                f'(read: 0x{ret:%02x})')

    def blank_check(self, total_fuse: int) -> None:
        """Check if sub-section of the NVCM memory is blank

        To check all of the memory, first determine how much NVCM
        memory your part actually has, or at least the size of the
        file that you plan to write to it.

        Keyword arguments:
        total_fuse -- Number of fuse bytes to read before stopping
        """
        self.bank_select('nvcm')

        status = True
        print("NVCM main memory blank check")
        contents = self.read_bytes(0x03, 0x000000, total_fuse)

        for index in range(0, total_fuse):
            val = contents[index]
            if self.debug:
                print(f"{index:08x}: {val:02x}")
            if val != 0:
                print(
                    f"{index:08x}: NVCM Memory Block is not blank.",
                    file=sys.stderr)
                status = False

        self.bank_select('nvcm')
        if not status:
            raise ValueError("NVCM Main Memory not blank")

    def program(self, rows: list[str]) -> None:
        """Program the memory by running an NVCM command sequence

        Keyword arguments:
        rows -- List of NVCM commands to run, formatted as hex
                strings
        """
        print("NVCM Program main memory")

        self.bank_select('nvcm')

        self.enable_trim()

        self.pgm_enable()

        i = 0
        for row in rows:
            # print('data for row:',i, row)
            if i % (1024 * 8) == 0:
                print("%6d / %6d bytes" % (i, len(rows) * 8))
            i += 8
            try:
                self.command(row)
            except Exception as exc:
                raise Exception(
                    "programming failed, row:{row}"
                ) from exc

        self.pgm_disable()

    def write_trim_pages(self, lock_bits: str) -> None:
        """Write to the trim pages

        The trim pages can be written multiple times. Known usages
        are to configure the device for NVCM boot, and to secure
        the device by disabling the NVCM interface.

        Keyword arguments:
        lock_bits -- Mas of bits to set in the trim pages
        """
        self.bank_select('nvcm')

        self.enable_trim()

        self.bank_select('trim')

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
        self.write(0x02, 0x000020, lock_bits)
        self.write(0x02, 0x000060, lock_bits)
        self.write(0x02, 0x0000a0, lock_bits)
        self.write(0x02, 0x0000e0, lock_bits)

        self.pgm_disable()

        # verify a read back
        val = self.read_int(0x03, 0x000020)

        self.bank_select('nvcm')

        lock_bits_int = int(lock_bits, 16)
        if val & lock_bits_int != lock_bits_int:
            raise ValueError(
                "Failed to write trim lock bits: " +
                f"{val:016x} != expected {lock_bits_int:016x}"
            )

        print(f"New state {val:016x}")

    def trim_secure(self) -> None:
        """Disable NVCM readout by programming the security bits

        Use with caution- the device will no longer respond to NVCM
        commands after this command runs.
        """
        print("NVCM Secure")
        trim = self.read_trim()
        if (trim >> 60) & 0x3 != 0:
            print(
                "NVCM already secure? trim=%016x" %
                (trim), file=sys.stderr)

        self.write_trim_pages("3000000100000000")

    def trim_program(self) -> None:
        """Configure the device to boot from NVCM (?)

        Use with caution- the device will no longer boot from
        external SPI flash after this command runs.
        """
        print("NVCM Program Trim_Parameter_OTP")
        self.write_trim_pages("0015f2f1c4000000")

    def info(self) -> None:
        """ Print the contents of the configuration registers """
        self.bank_select('sig')
        sig1 = self.read_int(0x03, 0x000000)

        self.bank_select('sig')
        sig2 = self.read_int(0x03, 0x000008)

        # have to switch back to nvcm bank before switching to trim?
        self.bank_select('nvcm')
        trim = self.read_trim()

#        self.bank_select('nvcm')

        self.bank_select('trim')
        trim0 = self.read_int(0x03, 0x000020)

        self.bank_select('trim')
        trim1 = self.read_int(0x03, 0x000060)

        self.bank_select('trim')
        trim2 = self.read_int(0x03, 0x0000a0)

        self.bank_select('trim')
        trim3 = self.read_int(0x03, 0x0000e0)

        self.bank_select('nvcm')

        secured = (trim >> 60) & 0x3
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

        self.bank_select('nvcm')

        contents = bytearray()

        for offset in range(0, length, 8):
            if offset % (1024 * 8) == 0:
                print("%6d / %6d bytes" % (offset, length))

            nvcm_addr = int(offset / 328) * 4096 + (offset % 328)
            contents += self.read_bytes(0x03, nvcm_addr, 8)
            self.delay(2)

        return bytes(contents)

    def read_file(self, filename: str, length: int) -> None:
        """ Read the contents of the NVCM to a file

        Keyword arguments:
        filename -- File to write to, or '-' to write to stdout
        length -- Number of bytes to read from NVCM
        """

        contents = bytearray()

        # prepend a header to the file, to identify it as an FPGA
        # bitstream
        contents += bytes([0xff, 0x00, 0x00, 0xff])

        contents += self.read_nvcm(length)

        if filename == '-':
            with os.fdopen(sys.stdout.fileno(),
                           "wb",
                           closefd=False) as out_file:
                out_file.write(contents)
                out_file.flush()
        else:
            with open(filename, "wb") as out_file:
                out_file.write(contents)
                out_file.flush()

    def verify(self, filename: str) -> None:
        """ Verify that the contents of the NVCM match a file

        Keyword arguments:
        filename -- File to compare
        """
        with open(filename, "rb") as verify_file:
            compare = verify_file.read()

        assert len(compare) > 0

        contents = bytearray()
        contents += bytes([0xff, 0x00, 0x00, 0xff])
        contents += self.read_nvcm(len(compare))

        # We might have read more than needed because of read
        # boundaries
        if len(contents) > len(compare):
            contents = contents[:len(compare)]

        assert compare == contents
        print('Verification complete, NVCM contents match file')


def sleep_flash(pins: dict) -> None:
    """ Put the SPI bootloader flash in deep sleep mode

    Keyword arguments:
    pins -- Dictionary of pins to use for SPI interface
    """
    flasher = IceFlasher()

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

    flasher.spi_write(bytes([0xAB]))

    # Confirm we can talk to flash
    data = flasher.spi_rxtx(bytes([0x9f, 0, 0]))

    print('flash ID while awake:', ' '.join(
        [f'{b:02x}' for b in data]))
    assert data == bytes([0xff, 0xef, 0x40])

    # Test that the flash will ignore a sleep command that doesn't
    # start on the first byte
    flasher.spi_write(bytes([0, 0xb9]))

    # Confirm we can talk to flash
    data = flasher.spi_rxtx(bytes([0x9f, 0, 0]))

    print('flash ID while awake:', ' '.join(
        [f'{b:02x}' for b in data]))
    assert data == bytes([0xff, 0xef, 0x40])

    # put the flash to sleep
    flasher.spi_write(bytes([0xb9]))

    # Confirm flash is asleep
    data = flasher.spi_rxtx(bytes([0x9f, 0, 0]))

    print('flash ID while asleep:', ' '.join(
        [f'{b:02x}' for b in data]))
    assert data == bytes([0xff, 0xff, 0xff])


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()

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

    parser.add_argument(
        '--speed',
        dest='spi_speed',
        type=int,
        default=15,
        help='SPI clock speed, in MHz')

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
        sys.exit(1)

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

    nvcm = Nvcm(
        tp1_pins,
        spi_speed=args.spi_speed,
        debug=args.verbose)
    nvcm.power_on()

    # # Turn on ICE40 in CRAM boot mode
    nvcm.init()
    nvcm.nvcm_enable()

    if args.read_info:
        nvcm.info()

    if args.write_file:
        with open(args.write_file, "rb") as in_file:
            bitstream = in_file.read()
        print(f"read {len(bitstream)} bytes")
        cmds = icebin2nvcm(bitstream)

        if not args.ignore_blank:
            nvcm.trim_blank_check()
            # how much should we check?
            nvcm.blank_check(0x100)

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
        nvcm.enable(cs=True, reset=False)
        sleep(0.5)
        nvcm.enable(cs=True, reset=True)
