# Install python3 HID package https://pypi.org/project/hid/
import hid
import time
import struct
import math

USB_VID = 0xcafe


class ice40_flasher:
    def __init__(self):
        self.dev = None
        for dict in hid.enumerate(USB_VID):
            self.dev = hid.Device(dict['vendor_id'], dict['product_id'])
        if self.dev is None:
            raise IOError(
                "Couldn't find any hid device with vendor id 0x%x" %
                (USB_VID))

    def close(self):
        """Close the HID device"""
        self.dev.close()

    def led_set(self, value: bool) -> None:
        """Set the state of the onboard LED

        Keyword arguments:
        value -- True: On, False: Off
        """
        msg = struct.pack('>BBB',
                          0x0,
                          0x00,
                          (1 if value else 0)
                          )

        # print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_set_direction(self, pin: int, direction: bool) -> None:
        """Set the direction of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        value -- True: Set pin as output, False: set pin as input
        """
        msg = struct.pack('>BBII',
                          0x0,
                          0x10,
                          (1 << pin),
                          ((1 if direction else 0) << pin),
                          )

        # print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_set_pulls(self, pin: int, pullup: bool, pulldown: bool) -> None:
        """ True: Enable pullup/pulldown, False: Disable pullup/pulldown """
        """Configure the pullup and pulldown resistors for a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        pullup -- True: Enable pullup, False: Disable pullup
        pulldown -- True: Enable pulldown, False: Disable pulldown
        """
        msg = struct.pack('>BBIII',
                          0x0,
                          0x12,
                          (1 << pin),
                          ((1 if pullup else 0) << pin),
                          ((1 if pulldown else 0) << pin),
                          )

        # print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_put(self, pin: int, val: bool) -> None:
        """Set the output level of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        val -- True: High, False: Low
        """
        msg = struct.pack('>BBII',
                          0x0,
                          0x20,
                          1 << pin,
                          (1 if val else 0) << pin,
                          )

        # print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_get_all(self) -> int:
        """Read the input levels of all GPIO pins"""
        msg = struct.pack('<BB',
                          0x0,
                          0x30,
                          )

        # print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

        msg_in = self.dev.read(5)
        # print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])

        [id, gpio_states] = struct.unpack('>BI', msg_in)
        assert (id == 0x30)

        return gpio_states

    def gpio_get(self, pin: int) -> bool:
        """Read the input level of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        """
#        msg = struct.pack('<BB',
#            0x0,
#            0x30,
#            )
#
#        print(['{:02x}'.format(b) for b in msg])
#        self.dev.write(msg)
#
#        msg_in = self.dev.read(5)
#        print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])
#
#        [id, gpio_states] = struct.unpack('>BI', msg_in)
#        assert(id == 0x30)
        gpio_states = self.gpio_get_all()

        return ((gpio_states >> pin) & 0x01) == 0x01

    def spi_bitbang(
            self,
            sck_pin: int,
            mosi_pin: int,
            miso_pin: int,
            buf: bytearray) -> bytearray:
        """Bitbang a SPI transfer using the specificed GPIO pins

        Note that this command does not handle setting a CS pin, that must be accomplished
        separately, for instance by calling gpio_set() on the pin controlling the CS line.

        Keyword arguments:
        sck_pin -- GPIO pin number to use as the SCK signal
        mosi_pin -- GPIO pin number to use as the MOSI signal
        miso_pin -- GPIO pin number to use as the MISO signal
        buf -- Byte buffer to send. If the bit_count is smaller than the buffer size, some data will not be sent.
        bit_count -- (Optional) Number of bits (not bytes) to bitbang. If left unspecificed, defaults to the size of buf.
        """

        ret = bytearray()

        max_chunk_size = 56
        for i in range(0, len(buf), max_chunk_size):
            chunk = buf[i:i + max_chunk_size]
            ret.extend(
                self.spi_bitbang_inner(
                    sck_pin=sck_pin,
                    mosi_pin=mosi_pin,
                    miso_pin=miso_pin,
                    buf=chunk))

        return ret

    def spi_bitbang_inner(
            self,
            sck_pin: int,
            mosi_pin: int,
            miso_pin: int,
            buf: bytearray,
            bit_count: int = -
            1) -> bytearray:
        """Bitbang a SPI transfer using the specificed GPIO pins

        Note that this command does not handle setting a CS pin, that must be accomplished
        separately, for instance by calling gpio_set() on the pin controlling the CS line.

        Keyword arguments:
        sck_pin -- GPIO pin number to use as the SCK signal
        mosi_pin -- GPIO pin number to use as the MOSI signal
        miso_pin -- GPIO pin number to use as the MISO signal
        buf -- Byte buffer to send. If the bit_count is smaller than the buffer size, some data will not be sent.
        bit_count -- (Optional) Number of bits (not bytes) to bitbang. If left unspecificed, defaults to the size of buf.
        """
        if bit_count == -1:
            bit_count = len(buf) * 8

        if bit_count > 56 * 8:
            print('Message too large, bit_count:{:}'.format(bit_count))
            exit(1)

        if bit_count > len(buf) * 8:
            print(
                'Bit count larger than buffer, bit_count:{:} len(buf):{:}'.format(bit_count),
                len(buf) * 8)
            exit(1)

        header = struct.pack('>BBBBBI',
                             0x0,
                             0x40,
                             sck_pin,
                             mosi_pin,
                             miso_pin,
                             bit_count)
        msg = bytearray()
        msg.extend(header)
        msg.extend(buf)

        # print(['{:02x}'.format(b) for b in msg])
        self.dev.write(bytes(msg))

        msg_in = self.dev.read(64)
        # print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])

        [id, bits_read] = struct.unpack('>BI', msg_in[0:5])
        assert (id == 0x40)
        assert (bits_read == bit_count)
        bytes_read = math.ceil(bit_count / 8)
        return msg_in[5:(5 + bytes_read)]

    def adc_read_all(self) -> list[float]:
        """Read the voltage values of ADC 0, 1, and 2

        The firmware will read the values for each input multiple times, and return averaged values for each input.
        """
        msg = struct.pack('<BB',
                          0x0,
                          0x50,
                          )

#        print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

        msg_in = self.dev.read(13)
#        print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])

        [id, ch0, ch1, ch2] = struct.unpack('>BIII', msg_in)
        assert (id == 0x50)

        return ch0 / 1000000, ch1 / 1000000, ch2 / 1000000


if __name__ == '__main__':
    def test_gpio_loopback(flasher, gpioa, gpiob):

        flasher.gpio_set_direction(gpioa, True)
        flasher.gpio_set_direction(gpiob, False)

        flasher.gpio_put(gpioa, True)
        assert flasher.gpio_get(gpiob)

        flasher.gpio_put(gpioa, False)
        assert flasher.gpio_get(gpiob) == False

        flasher.gpio_set_direction(gpiob, True)
        flasher.gpio_set_direction(gpioa, False)

        flasher.gpio_put(gpiob, True)
        assert flasher.gpio_get(gpioa)

        flasher.gpio_put(gpiob, False)
        assert flasher.gpio_get(gpioa) == False

        flasher.gpio_put(gpioa, False)
        flasher.gpio_put(gpiob, False)

    def test_spi_4_bits(flasher):
        sck_pin = 10
        mosi_pin = 13
        miso_pin = 11

        flasher.gpio_set_direction(sck_pin, True)
        flasher.gpio_set_direction(mosi_pin, True)
        flasher.gpio_set_direction(miso_pin, False)

        bit_count = 4
        buf = bytearray()
        buf.append(0x51)
        buf_in = flasher.spi_bitbang(
            sck_pin,
            mosi_pin,
            miso_pin,
            buf,
            bit_count
        )
        assert (buf_in == buf)

    def test_spi_50_bytes(flasher):
        sck_pin = 10
        mosi_pin = 13
        miso_pin = 11
        cs_pin = 12

        flasher.gpio_set_direction(cs_pin, True)
        flasher.gpio_put(cs_pin, False)

        flasher.gpio_set_direction(sck_pin, True)
        flasher.gpio_set_direction(mosi_pin, True)
        flasher.gpio_set_direction(miso_pin, False)

        buf = bytearray()
        for i in range(0, 50):
            buf.append(i)

        bit_count = len(buf) * 8
        buf_in = flasher.spi_bitbang(
            sck_pin,
            mosi_pin,
            miso_pin,
            buf,
            bit_count
        )

        flasher.gpio_put(cs_pin, True)

    flasher = ice40_flasher()
    flasher.led_set(True)

    flasher.gpio_set_direction(7, True)
    flasher.gpio_put(7, False)
    time.sleep(.1)
    flasher.gpio_put(7, True)
    print(flasher.adc_read_all())
    exit(0)

    vals = [0, 0, 0]
    sample_count = 100
    for sample in range(0, sample_count):
        val = flasher.adc_read_all()
        vals[0] += val[0]
        vals[1] += val[1]
        vals[2] += val[2]

    print(['{:.3f}'.format(val / sample_count / 1000000) for val in vals])
    exit(0)

    # Test program: put a jumper between gpio16 and gpio17
    # gpioa = 16
    # gpiob = 17
    # test_gpio_loopback(flasher, gpioa, gpiob)

    flasher.gpio_set_direction(7, True)
    flasher.gpio_put(7, True)

    # test_spi_4_bits(flasher)
    test_spi_50_bytes(flasher)
