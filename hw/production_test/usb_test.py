#!/usr/bin/env python
"""IceFlasher, an iCE40 programming tool based on an RPi Pico"""

import struct
from typing import List, Any

import usb1  # type: ignore


# def processReceivedData(transfer):
#    #    print('got rx data',
#           transfer.getStatus(),
#           transfer.getActualLength())
#
#    if transfer.getStatus() != usb1.TRANSFER_COMPLETED:
#        # Transfer did not complete successfully, there is no
#        # data to read. This example does not resubmit transfers
#        # on errors. You may want to resubmit in some cases (timeout,
#        # ...).
#        return
#    data = transfer.getBuffer()[:transfer.getActualLength()]
#    # Process data...
#    # Resubmit transfer once data is processed.
# transfer.submit()


class IceFlasher:
    """ iCE40 programming tool based on an RPi Pico """

    COMMAND_PIN_DIRECTION = 0x30
    COMMAND_PULLUPS = 0x31
    COMMAND_PIN_VALUES = 0x32

    COMMAND_SPI_CONFIGURE = 0x40
    COMMAND_SPI_XFER = 0x41
    COMMAND_SPI_CLKOUT = 0x42

    COMMAND_ADC_READ = 0x50
    COMMAND_BOOTLOADER = 0xE0

    SPI_MAX_TRANSFER_SIZE = 2048 - 8

    def __init__(self) -> None:
        self.transfer_list: List[Any] = []

        # See: https://github.com/vpelletier/python-libusb1#usage
        self.context = usb1.USBContext()
        self.handle = self.context.openByVendorIDAndProductID(
            0xcafe,
            0x4010,
            skip_on_error=True,
        )

        if self.handle is None:
            # Device not present, or user is not allowed to access
            # device.
            raise ValueError('Device not found')

        # Check the device firmware version
        bcd_device = self.handle.getDevice().getbcdDevice()
        if bcd_device != 0x0200:
            raise ValueError(
                'Pico firmware version out of date- please upgrade')

        self.handle.claimInterface(0)

        self.cs_pin = -1

    def __del__(self) -> None:
        self.close()

    def close(self) -> None:
        """ Release the USB device handle """
        self._wait_async()

        if self.handle is not None:
            self.handle.close()
            self.handle = None
            self.context.close()
            self.context = None

    def _wait_async(self) -> None:
        # Wait until all submitted transfers can be cleared
        while any(transfer.isSubmitted()
                  for transfer in self.transfer_list):
            try:
                self.context.handleEvents()
            except usb1.USBErrorInterrupted:
                pass

            for transfer in reversed(self.transfer_list):
                if transfer.getStatus() == \
                        usb1.TRANSFER_COMPLETED:
                    self.transfer_list.remove(transfer)
                else:
                    print(
                        transfer.getStatus(),
                        usb1.TRANSFER_COMPLETED)

    def _write(
            self,
            request_id: int,
            data: bytes,
            nonblocking: bool = False) -> None:

        if nonblocking:
            transfer = self.handle.getTransfer()
            transfer.setControl(
                # usb1.ENDPOINT_OUT | usb1.TYPE_VENDOR |
                # usb1.RECIPIENT_DEVICE,  #request type
                0x40,
                request_id,  # request
                0,  # index
                0,
                data,  # data
                callback=None,  # callback functiopn
                user_data=None,  # userdata
                timeout=1000
            )
            transfer.submit()
            self.transfer_list.append(transfer)
        else:
            self.handle.controlWrite(
                0x40, request_id, 0, 0, data, timeout=100)

    def _read(self, request_id: int, length: int) -> bytes:
        # self._wait_async()
        return self.handle.controlRead(
            0xC0, request_id, 0, 0, length, timeout=100)

    def gpio_set_direction(self, pin: int, direction: bool) -> None:
        """Set the direction of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        value -- True: Set pin as output, False: set pin as input
        """
        msg = struct.pack('>II',
                          (1 << pin),
                          ((1 if direction else 0) << pin),
                          )

        self._write(self.COMMAND_PIN_DIRECTION, msg)

    def gpio_set_pulls(
            self,
            pin: int,
            pullup: bool,
            pulldown: bool) -> None:
        """Configure the pullup/down resistors for a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        pullup -- True: Enable pullup, False: Disable pullup
        pulldown -- True: Enable pulldown, False: Disable pulldown
        """
        msg = struct.pack('>III',
                          (1 << pin),
                          ((1 if pullup else 0) << pin),
                          ((1 if pulldown else 0) << pin),
                          )

        self._write(self.COMMAND_PULLUPS, msg)

    def gpio_put(self, pin: int, val: bool) -> None:
        """Set the output level of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        val -- True: High, False: Low
        """
        msg = struct.pack('>II',
                          1 << pin,
                          (1 if val else 0) << pin,
                          )

        self._write(self.COMMAND_PIN_VALUES, msg)

    def gpio_get_all(self) -> int:
        """Read the input levels of all GPIO pins"""
        msg_in = self._read(self.COMMAND_PIN_VALUES, 4)
        [gpio_states] = struct.unpack('>I', msg_in)

        return gpio_states

    def gpio_get(self, pin: int) -> bool:
        """Read the input level of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        """
        gpio_states = self.gpio_get_all()

        return ((gpio_states >> pin) & 0x01) == 0x01

    def spi_configure(
            self,
            sck_pin: int,
            cs_pin: int,
            mosi_pin: int,
            miso_pin: int,
            clock_speed: int) -> None:
        """Set the pins to use for SPI transfers

        Keyword arguments:
        sck_pin -- GPIO pin number to use as the SCK signal
        cs_pin -- GPIO pin number to use as the CS signal
        mosi_pin -- GPIO pin number to use as the MOSI signal
        miso_pin -- GPIO pin number to use as the MISO signal
        clock_speed -- SPI clock speed, in MHz
        """
        header = struct.pack('>BBBBB',
                             sck_pin,
                             cs_pin,
                             mosi_pin,
                             miso_pin,
                             clock_speed)
        msg = bytearray()
        msg.extend(header)

        self._write(self.COMMAND_SPI_CONFIGURE, msg)

        self.cs_pin = cs_pin

    def spi_write(
            self,
            buf: bytes,
            toggle_cs: bool = True) -> None:
        """Write data to the SPI port

        Keyword arguments:
        buf -- Byte buffer to send.
        toggle_cs -- (Optional) If true, toggle the CS line
        """
        self._spi_xfer(buf, toggle_cs, False)

    def spi_rxtx(
            self,
            buf: bytes,
            toggle_cs: bool = True) -> bytes:
        """Bitbang a SPI transfer

        Keyword arguments:
        buf -- Byte buffer to send.
        toggle_cs -- (Optional) If true, toggle the CS line
        """

        return self._spi_xfer(buf, toggle_cs, True)

    def _spi_xfer(
            self,
            buf: bytes,
            toggle_cs: bool,
            read_after_write: bool) -> bytes:

        ret = bytearray()

        if len(buf) <= self.SPI_MAX_TRANSFER_SIZE:
            return self._spi_xfer_inner(
                buf,
                toggle_cs,
                read_after_write)

        if toggle_cs:
            self.gpio_put(self.cs_pin, False)

        for i in range(0, len(buf), self.SPI_MAX_TRANSFER_SIZE):
            chunk = buf[i:i + self.SPI_MAX_TRANSFER_SIZE]
            ret.extend(
                self._spi_xfer_inner(
                    chunk,
                    False,
                    read_after_write))

        if toggle_cs:
            self.gpio_put(self.cs_pin, True)

        return bytes(ret)

    def _spi_xfer_inner(
            self,
            buf: bytes,
            toggle_cs: bool,
            read_after_write: bool) -> bytes:
        """Bitbang a SPI transfer using the specificed GPIO pins

        Keyword arguments:
        buf -- Byte buffer to send.
        toggle_cs -- (Optional) If true, toggle the CS line
        """

        if len(buf) > self.SPI_MAX_TRANSFER_SIZE:
            raise ValueError(
                'Message too large, '
                + f'size:{len(buf)} max:{self.SPI_MAX_TRANSFER_SIZE}')

        header = struct.pack('>BI', toggle_cs, len(buf))
        msg = bytearray()
        msg.extend(header)
        msg.extend(buf)

        self._write(self.COMMAND_SPI_XFER, msg)

        if not read_after_write:
            return bytes()

        msg_in = self._read(
            self.COMMAND_SPI_XFER,
            len(buf))

        return msg_in

    def spi_clk_out(self, byte_count: int) -> None:
        """Run the SPI clock without transferring data

        This function is useful for SPI devices that need a clock to
        advance their state machines.

        Keyword arguments:
        byte_count -- Number of bytes worth of clocks to send
        """

        header = struct.pack('>I',
                             byte_count)
        msg = bytearray()
        msg.extend(header)
        self._write(
            self.COMMAND_SPI_CLKOUT,
            msg)

    def adc_read_all(self) -> tuple[float, float, float]:
        """Read the voltage values of ADC 0, 1, and 2

        The firmware will read the values for each input multiple
        times, and return averaged values for each input.
        """
        msg_in = self._read(self.COMMAND_ADC_READ, 3 * 4)
        [ch0, ch1, ch2] = struct.unpack('>III', msg_in)

        return ch0 / 1000000, ch1 / 1000000, ch2 / 1000000

    def bootloader(self) -> None:
        """Reset the programmer to bootloader mode

        After the device is reset, it can be programmed using
        picotool, or by copying a file to the uf2 drive.
        """
        try:
            self._write(self.COMMAND_BOOTLOADER, bytes())
        except usb1.USBErrorIO:
            pass
