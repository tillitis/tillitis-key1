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
            raise IOError("Couldn't find any hid device with vendor id 0x%x" % (USB_VID))

    def close(self):
        self.dev.close()

    def led_set(self, value):
        msg = struct.pack('>BBB',
            0x0,
            0x00,
            (1 if value else 0)
            )

        #print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_set_direction(self, pin, direction):
        msg = struct.pack('>BBII',
            0x0,
            0x10,
            (1<<pin),
            ((1 if direction else 0)<<pin),
            )

        #print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_set_pulls(self, pin, pullup, pulldown):
        msg = struct.pack('>BBIII',
            0x0,
            0x12,
            (1<<pin),
            ((1 if pullup else 0)<<pin),
            ((1 if pulldown else 0)<<pin),
            )

        #print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_put(self, pin, val):
        msg = struct.pack('>BBII',
            0x0,
            0x20,
            1 << pin,
            (1 if val else 0) << pin,
            )

        #print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

    def gpio_get_all(self):
        msg = struct.pack('<BB',
            0x0,
            0x30,
            )

        #print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

        msg_in = self.dev.read(5)
        #print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])

        [id, gpio_states] = struct.unpack('>BI', msg_in)
        assert(id == 0x30)

        return gpio_states

    def gpio_get(self, pin):
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
        gpio_states = gpio_get_all()

        return ((gpio_states >> pin) & 0x01) == 0x01

    def spi_bitbang(self, sck_pin, mosi_pin, miso_pin, bit_count, buf):
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

        #print(['{:02x}'.format(b) for b in msg])
        self.dev.write(bytes(msg))

        msg_in = self.dev.read(64)
        #print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])

        [id, bits_read] = struct.unpack('>BI', msg_in[0:5])
        assert(id == 0x40)
        assert(bits_read == bit_count)
        bytes_read = math.ceil(bit_count / 8)
        return msg_in[6:(6+bytes_read)]

    def adc_read_all(self):
        msg = struct.pack('<BB',
            0x0,
            0x50,
            )

#        print(['{:02x}'.format(b) for b in msg])
        self.dev.write(msg)

        msg_in = self.dev.read(13)
#        print(len(msg_in), ['{:02x}'.format(b) for b in msg_in])

        [id, ch0, ch1, ch2] = struct.unpack('>BIII', msg_in)
        assert(id == 0x50)

        return ch0/1000000, ch1/1000000, ch2/1000000

if __name__ == '__main__':
    def test_gpio_loopback(flasher, gpioa, gpiob):
    
        flasher.gpio_set_direction(gpioa, True)
        flasher.gpio_set_direction(gpiob, False)
    
        flasher.gpio_put(gpioa, True)
        assert flasher.gpio_get(gpiob) == True
    
        flasher.gpio_put(gpioa, False)
        assert flasher.gpio_get(gpiob) == False
    
        flasher.gpio_set_direction(gpiob, True)
        flasher.gpio_set_direction(gpioa, False)
    
        flasher.gpio_put(gpiob, True)
        assert flasher.gpio_get(gpioa) == True
    
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
    
        bit_count = 4;
        buf = bytearray()
        buf.append(0x51)
        buf_in = flasher.spi_bitbang(
            sck_pin,
            mosi_pin,
            miso_pin,
            bit_count,
            buf
            )
        assert(buf_in == buf)
    
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
        for i in range(0,50):
            buf.append(i)
    
        bit_count = len(buf)*8;
        buf_in = flasher.spi_bitbang(
            sck_pin,
            mosi_pin,
            miso_pin,
            bit_count,
            buf
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
    
    vals = [0,0,0]
    sample_count = 100
    for sample in range(0, sample_count):
        val = flasher.adc_read_all()
        vals[0] += val[0]
        vals[1] += val[1]
        vals[2] += val[2]
    
    print(['{:.3f}'.format(val/sample_count/1000000) for val in vals])
    exit(0)
    
    # Test program: put a jumper between gpio16 and gpio17
    #gpioa = 16
    #gpiob = 17
    #test_gpio_loopback(flasher, gpioa, gpiob)
    
    flasher.gpio_set_direction(7, True)
    flasher.gpio_put(7, True)
    
    #test_spi_4_bits(flasher)
    test_spi_50_bytes(flasher)
