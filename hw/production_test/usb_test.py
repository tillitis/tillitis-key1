#!/usr/bin/env python
import usb.core
import usb.util
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
    FLASHER_REQUEST_BOOTLOADRE = 0xFF

    def __init__(self):
#        self.dev = None
#        for dict in hid.enumerate(USB_VID):
#            self.dev = hid.Device(dict['vendor_id'], dict['product_id'])
#        if self.dev is None:
#            raise IOError("Couldn't find any hid device with vendor id 0x%x" % (USB_VID))

        # See: https://github.com/pyusb/pyusb/blob/master/docs/tutorial.rst
        self.dev = usb.core.find(idVendor=0xcafe, idProduct=0x4010)
        
        if self.dev is None:
            raise ValueError('Device not found')
        
        self.dev.set_configuration()

    def close(self):
        """Close the HID device"""
        #self.dev.close()
        pass

    def _write(self, request_id: int, data: bytes):
        self.dev.ctrl_transfer(0x40, request_id,0,0,data)

    def _write_bulk(self, request_id: int, data: bytes):
        msg = bytearray()
        msg.append(request_id)
        msg.extend(data)
        self.dev.write(0x01, data)

    def _read(self, request_id: int, length: int) -> bytes:
        #ctrl_transfer(self, bmRequestType, bRequest, wValue=0, wIndex=0, data_or_wLength = None, timeout = None):
        # Request type:
        # bit 7: direction 0:host to device (OUT), 1: device to host (IN)
        # bits 5-6: type: 0:standard 1:class 2:vendor 3:reserved
        # bits 0-4: recipient: 0:device 1:interface 2:endpoint 3:other
        ret = self.dev.ctrl_transfer(0xC0, request_id,0,0,length)
        return ret

    def gpio_set_direction(self, pin: int, direction: bool) -> None:
        """Set the direction of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        value -- True: Set pin as output, False: set pin as input
        """
        msg = struct.pack('>II',
            (1<<pin),
            ((1 if direction else 0)<<pin),
            )

#self._write_bulk(self.FLASHER_REQUEST_PIN_DIRECTION_SET, msg)
        self._write(self.FLASHER_REQUEST_PIN_DIRECTION_SET, msg)

    def gpio_set_pulls(self, pin: int, pullup: bool, pulldown: bool) -> None:
        """ True: Enable pullup/pulldown, False: Disable pullup/pulldown """
        """Configure the pullup and pulldown resistors for a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        pullup -- True: Enable pullup, False: Disable pullup
        pulldown -- True: Enable pulldown, False: Disable pulldown
        """
        msg = struct.pack('>III',
            (1<<pin),
            ((1 if pullup else 0)<<pin),
            ((1 if pulldown else 0)<<pin),
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
        msg_in = self._read(self.FLASHER_REQUEST_PIN_VALUES_GET,4)
        [gpio_states] = struct.unpack('>I', msg_in)

        return gpio_states

    def gpio_get(self, pin: int) -> bool:
        """Read the input level of a single GPIO pin

        Keyword arguments:
        pin -- GPIO pin number
        """
        gpio_states = gpio_get_all()

        return ((gpio_states >> pin) & 0x01) == 0x01

    def spi_pins_set(
            self,
            sck_pin: int,
            cs_pin: int,
            mosi_pin: int,
            miso_pin: int) -> None:
        """Set the pins to use for SPI transfers

        Keyword arguments:
        sck_pin -- GPIO pin number to use as the SCK signal
        cs_pin -- GPIO pin number to use as the CS signal
        mosi_pin -- GPIO pin number to use as the MOSI signal
        miso_pin -- GPIO pin number to use as the MISO signal
        """
        header = struct.pack('>BBBB',
                             sck_pin,
                             cs_pin,
                             mosi_pin,
                             miso_pin)
        msg = bytearray()
        msg.extend(header)

        self._write(self.FLASHER_REQUEST_SPI_PINS_SET,msg)

    def spi_bitbang(
            self,
            buf: bytearray,
            toggle_cs: bool = True) -> bytearray:
        """Bitbang a SPI transfer

        Keyword arguments:
        buf -- Byte buffer to send. If the bit_count is smaller than the buffer size, some data will not be sent.
        toggle_cs: (Optional) If true, toggle the CS line
        """

        ret = bytearray()

        max_chunk_size = (1024-8)
        for i in range(0, len(buf), max_chunk_size):
            chunk = buf[i:i + max_chunk_size]
            ret.extend(
                self.spi_bitbang_inner(
                    buf=chunk,
                    toggle_cs=toggle_cs))

        return ret

    def spi_bitbang_inner(
            self,
            buf: bytearray,
            bit_count: int = -1,
            toggle_cs: bool = True) -> bytearray:
        """Bitbang a SPI transfer using the specificed GPIO pins

        Note that this command does not handle setting a CS pin, that must be accomplished
        separately, for instance by calling gpio_set() on the pin controlling the CS line.

        Keyword arguments:
        buf -- Byte buffer to send. If the bit_count is smaller than the buffer size, some data will not be sent.
        bit_count -- (Optional) Number of bits (not bytes) to bitbang. If left unspecificed, defaults to the size of buf.
        toggle_cs: (Optional) If true, toggle the CS line
        """
        if bit_count == -1:
            bit_count = len(buf) * 8

        byte_length = (bit_count+7)//8

        if byte_length > (1024-8):
            print('Message too large, bit_count:{:}'.format(bit_count))
            exit(1)
        
        if byte_length != len(buf):
            print(
                'Bit count size mismatch, bit_count:{:} len(buf):{:}'.format(bit_count),
                len(buf) * 8)
            exit(1)

        header = struct.pack('>I',
                             bit_count)
        msg = bytearray()
        msg.extend(header)
        msg.extend(buf)

        if toggle_cs:
            self._write(self.FLASHER_REQUEST_SPI_BITBANG_CS,msg)
            msg_in = self._read(self.FLASHER_REQUEST_SPI_BITBANG_CS, byte_length)
        else:
            self._write(self.FLASHER_REQUEST_SPI_BITBANG_NO_CS,msg)
            msg_in = self._read(self.FLASHER_REQUEST_SPI_BITBANG_NO_CS, byte_length)

        return msg_in

    def adc_read_all(self) -> list[float]:
        """Read the voltage values of ADC 0, 1, and 2

        The firmware will read the values for each input multiple times, and return averaged values for each input.
        """
        msg_in = self._read(self.FLASHER_REQUEST_ADC_READ,3*4)
        [ch0, ch1, ch2] = struct.unpack('>III', msg_in)

        return ch0/1000000, ch1/1000000, ch2/1000000

if __name__ == '__main__':
    flasher = ice40_flasher()
    print(flasher.gpio_get_all())
    print(flasher.adc_read_all())

    # for pin in range(10,13):
    #    flasher.gpio_set_direction(pin, True)

    # while True:
    #     for pin in range(10,13):
    #         flasher.gpio_put(pin, True)
    #         flasher.gpio_put(pin, False)

    flasher.gpio_set_direction(10, True)
    flasher.gpio_set_direction(11, True)
    flasher.gpio_set_direction(13, False)


    buf = [0x01,0x02,0x03, 0xFE]

    while True:
        flasher.spi_bitbang_inner(sck_pin=10, mosi_pin=11, miso_pin=13, buf=buf)
