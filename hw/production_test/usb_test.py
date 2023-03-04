#!/usr/bin/env python
import usb.core  # type: ignore
import usb.util  # type: ignore
import struct


class ice40_flasher:
    FLASHER_REQUEST_LED_SET = 0x00,
    FLASHER_REQUEST_PIN_DIRECTION_SET = 0x10
    FLASHER_REQUEST_PULLUPS_SET = 0x12
    FLASHER_REQUEST_PIN_VALUES_SET = 0x20
    FLASHER_REQUEST_PIN_VALUES_GET = 0x30
    FLASHER_REQUEST_SPI_BITBANG_CS = 0x41
    FLASHER_REQUEST_SPI_BITBANG_NO_CS = 0x42
    FLASHER_REQUEST_SPI_PINS_SET = 0x43
    FLASHER_REQUEST_ADC_READ = 0x50
    FLASHER_REQUEST_BOOTLOADER = 0xFF

    def __init__(self) -> None:
        # See:
        # https://github.com/pyusb/pyusb/blob/master/docs/tutorial.rst
        self.dev = usb.core.find(idVendor=0xcafe, idProduct=0x4010)

        if self.dev is None:
            raise ValueError('Device not found')

        self.dev.set_configuration()

    def _write(self, request_id: int, data: bytes) -> None:
        self.dev.ctrl_transfer(0x40, request_id, 0, 0, data)

    def _write_bulk(self, request_id: int, data: bytes) -> None:
        msg = bytearray()
        msg.append(request_id)
        msg.extend(data)
        self.dev.write(0x01, data)

    def _read(self, request_id: int, length: int) -> bytes:
        # ctrl_transfer(self, bmRequestType, bRequest, wValue=0,
        #   wIndex=0, data_or_wLength = None, timeout = None):
        # Request type:
        # bit 7: direction 0:host to device (OUT),
        #                  1: device to host (IN)
        # bits 5-6: type: 0:standard 1:class 2:vendor 3:reserved
        # bits 0-4: recipient: 0:device 1:interface 2:endpoint 3:other
        ret = self.dev.ctrl_transfer(0xC0, request_id, 0, 0, length)
        return ret

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

# self._write_bulk(self.FLASHER_REQUEST_PIN_DIRECTION_SET, msg)
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

    def spi_bitbang(
            self,
            buf: bytes,
            toggle_cs: bool = True) -> bytes:
        """Bitbang a SPI transfer

        Keyword arguments:
        buf -- Byte buffer to send. If the bit_count is smaller than
               the buffer size, some data will not be sent.
        toggle_cs: (Optional) If true, toggle the CS line
        """

        ret = bytearray()

        max_chunk_size = (2048 - 8)
        for i in range(0, len(buf), max_chunk_size):
            chunk = buf[i:i + max_chunk_size]
            ret.extend(
                self.spi_bitbang_inner(
                    buf=chunk,
                    toggle_cs=toggle_cs))

        return bytes(ret)

    def spi_bitbang_inner(
            self,
            buf: bytes,
            bit_count: int = -1,
            toggle_cs: bool = True) -> bytes:
        """Bitbang a SPI transfer using the specificed GPIO pins

        Keyword arguments:
        buf -- Byte buffer to send. If the bit_count is smaller than
               the buffer size, some data will not be sent.
        bit_count -- (Optional) Number of bits (not bytes) to
               bitbang. If left unspecificed, defaults to the size
               of buf.
        toggle_cs: (Optional) If true, toggle the CS line
        """
        if bit_count == -1:
            bit_count = len(buf) * 8

        byte_length = (bit_count + 7) // 8

        if byte_length > (2048 - 8):
            print(
                'Message too large, bit_count:{:}'.format(bit_count))
            exit(1)

        if byte_length != len(buf):
            print(
                'Size mismatch, bit_count:{:} len(buf):{:}'.format(
                    bit_count, len(buf) * 8))
            exit(1)

        header = struct.pack('>I',
                             bit_count)
        msg = bytearray()
        msg.extend(header)
        msg.extend(buf)

        if toggle_cs:
            self._write(self.FLASHER_REQUEST_SPI_BITBANG_CS, msg)
            msg_in = self._read(
                self.FLASHER_REQUEST_SPI_BITBANG_CS,
                byte_length)
        else:
            self._write(self.FLASHER_REQUEST_SPI_BITBANG_NO_CS, msg)
            msg_in = self._read(
                self.FLASHER_REQUEST_SPI_BITBANG_NO_CS,
                byte_length)

        return msg_in

    def nvcm_command(self, cmd: bytes) -> None:
        """NVCM fast path: Run a command on the NVCM memory, then
        """
        pass

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
