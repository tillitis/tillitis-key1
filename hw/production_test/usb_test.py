#!/usr/bin/env python
from time import sleep

# pyusb
import usb.core  # type: ignore
import usb.util  # type: ignore

# libusb
import usb1  # type: ignore

import struct
from typing import List, Any

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


class ice40_flasher:
    FLASHER_REQUEST_LED_SET = 0x00,
    FLASHER_REQUEST_PIN_DIRECTION_SET = 0x10
    FLASHER_REQUEST_PULLUPS_SET = 0x12
    FLASHER_REQUEST_PIN_VALUES_SET = 0x20
    FLASHER_REQUEST_PIN_VALUES_GET = 0x30
    FLASHER_REQUEST_SPI_BITBANG_CS = 0x41
    FLASHER_REQUEST_SPI_BITBANG_NO_CS = 0x42
    FLASHER_REQUEST_SPI_PINS_SET = 0x43
    FLASHER_REQUEST_SPI_CLKOUT = 0x44
    FLASHER_REQUEST_ADC_READ = 0x50
    FLASHER_REQUEST_BOOTLOADER = 0xFF

    SPI_MAX_TRANSFER_SIZE = (2048 - 8)

    def __init__(self) -> None:
        self.backend = 'libusb'

        if self.backend == 'pyusb':
            # See:
            # https://github.com/pyusb/pyusb/blob/master/docs/tutorial.rst
            self.dev = usb.core.find(
                idVendor=0xcafe, idProduct=0x4010)

            if self.dev is None:
                raise ValueError('Device not found')

            self.dev.set_configuration()

        elif self.backend == 'libusb':
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
            self.handle.claimInterface(0)

            self.transfer_list: List[Any] = []

    def _wait_async(self) -> None:
        if self.backend == 'libusb':
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

    def _write(self, request_id: int, data: bytes,
               nonblocking: bool = False) -> None:
        if self.backend == 'pyusb':
            self.dev.ctrl_transfer(0x40, request_id, 0, 0, data)

        elif self.backend == 'libusb':
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
                self.handle.controlWrite(0x40, request_id, 0, 0, data)

    def _read(self, request_id: int, length: int) -> bytes:
        if self.backend == 'pyusb':
            # ctrl_transfer(self, bmRequestType, bRequest, wValue=0,
            #  wIndex=0, data_or_wLength = None, timeout = None):
            # Request type:
            # bit 7: direction 0:host to device (OUT),
            #                 1: device to host (IN)
            # bits 5-6: type: 0:standard 1:class 2:vendor 3:reserved
            # bits 0-4: recipient: 0:device 1:interface 2:endpoint
            # 3:other
            ret = self.dev.ctrl_transfer(
                0xC0, request_id, 0, 0, length)

        elif self.backend == 'libusb':
            self._wait_async()
            ret = self.handle.controlRead(
                0xC0, request_id, 0, 0, length)

        return ret

    def gpio_set_direction(self, pin: int, direction: bool) -> None:
        """Set the direction of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        direction -- True: Set pin as output, False: set pin as input
        """
        msg = struct.pack('>II',
                          (1 << pin),
                          ((1 if direction else 0) << pin),
                          )

        self._write(self.FLASHER_REQUEST_PIN_DIRECTION_SET, msg)

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

        self._write(self.FLASHER_REQUEST_PULLUPS_SET, msg)

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

        self._write(self.FLASHER_REQUEST_PIN_VALUES_SET, msg)

    def gpio_get_all(self) -> int:
        """Read the input levels of all GPIO pins"""
        msg_in = self._read(self.FLASHER_REQUEST_PIN_VALUES_GET, 4)
        [gpio_states] = struct.unpack('>I', msg_in)

        return gpio_states

    def gpio_get(self, pin: int) -> bool:
        """Read the input level of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        """
        gpio_states = self.gpio_get_all()

        return ((gpio_states >> pin) & 0x01) == 0x01

    def spi_pins_set(
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
        """
        header = struct.pack('>BBBBB',
                             sck_pin,
                             cs_pin,
                             mosi_pin,
                             miso_pin,
                             clock_speed)
        msg = bytearray()
        msg.extend(header)

        self._write(self.FLASHER_REQUEST_SPI_PINS_SET, msg)

    def spi_write(
            self,
            buf: bytes,
            toggle_cs: bool = True) -> bytes:
        """Write data to the SPI port

        Keyword arguments:
        buf -- Byte buffer to send.
        toggle_cs: (Optional) If true, toggle the CS line
        """
        max_chunk_size = self.SPI_MAX_TRANSFER_SIZE
        for i in range(0, len(buf), max_chunk_size):
            chunk = buf[i:i + max_chunk_size]
            self._spi_bitbang_inner(
                buf=chunk,
                toggle_cs=toggle_cs,
                read_after_write=False)

    def spi_rxtx(
            self,
            buf: bytes,
            toggle_cs: bool = True) -> bytes:
        """Bitbang a SPI transfer

        Keyword arguments:
        buf -- Byte buffer to send.
        toggle_cs: (Optional) If true, toggle the CS line
        """

        ret = bytearray()

        max_chunk_size = self.SPI_MAX_TRANSFER_SIZE
        for i in range(0, len(buf), max_chunk_size):
            chunk = buf[i:i + max_chunk_size]
            ret.extend(
                self._spi_bitbang_inner(
                    buf=chunk,
                    toggle_cs=toggle_cs))

        return bytes(ret)

    def _spi_bitbang_inner(
            self,
            buf: bytes,
            toggle_cs: bool = True,
            read_after_write: bool = True) -> bytes:
        """Bitbang a SPI transfer using the specificed GPIO pins

        Keyword arguments:
        buf -- Byte buffer to send.
        toggle_cs: (Optional) If true, toggle the CS line
        """

        if len(buf) > self.SPI_MAX_TRANSFER_SIZE:
            raise Exception(
                'Message too large, size:{:} max:{:}'.format(
                    len(buf), self.SPI_MAX_TRANSFER_SIZE))

        header = struct.pack('>I', len(buf))
        msg = bytearray()
        msg.extend(header)
        msg.extend(buf)

        if toggle_cs:
            cmd = self.FLASHER_REQUEST_SPI_BITBANG_CS
        else:
            cmd = self.FLASHER_REQUEST_SPI_BITBANG_NO_CS

        self._write(cmd, msg, nonblocking=True)

        if not read_after_write:
            return bytes()

        msg_in = self._read(
            self.FLASHER_REQUEST_SPI_BITBANG_CS,
            len(buf))

        return msg_in

    def spi_clk_out(self, byte_count: int) -> None:
        header = struct.pack('>I',
                             byte_count)
        msg = bytearray()
        msg.extend(header)
        self._write(
            self.FLASHER_REQUEST_SPI_CLKOUT,
            msg,
            nonblocking=True)

    def adc_read_all(self) -> tuple[float, float, float]:
        """Read the voltage values of ADC 0, 1, and 2

        The firmware will read the values for each input multiple
        times, and return averaged values for each input.
        """
        msg_in = self._read(self.FLASHER_REQUEST_ADC_READ, 3 * 4)
        [ch0, ch1, ch2] = struct.unpack('>III', msg_in)

        return ch0 / 1000000, ch1 / 1000000, ch2 / 1000000

    def bootloader(self) -> None:
        """Reset the programmer to bootloader mode

        After the device is reset, it can be programmed using
        picotool, or by copying a file to the uf2 drive.
        """
        try:
            self._write(self.FLASHER_REQUEST_BOOTLOADER, bytes())
        except usb.core.USBError:
            # We expect the device to disappear immediately, so mask
            # the resulting error
            pass


if __name__ == '__main__':
    flasher = ice40_flasher()

    flasher.spi_pins_set(1, 2, 3, 4, 15)
