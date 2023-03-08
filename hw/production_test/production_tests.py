#!/usr/bin/env python
"""Production test definitions for Tillitis TK1 and TP1

The test runner looks for these objects in this file:

parameters: Dictionary of string variables containing overrideable
            parameters, such as locations of external programs.
            These parameters are automatically added as command line
            arguements to the test runner.

manual_tests: List of functions that implement manual tests. These
              tests will be added to the 'manual test' menu. Each
              test should have a pep257 formatted docstring, which
              will be displayed to introduce the text. The tests
              must not take any paraemters, and return True if the
              test passed successfully, or False if it failed.
              Manual tests should be able to run independenly (for
              example, they shouldn't rely on a previous test turning
              on a power supply).

test_sequences: Dictionary of production test sequences. Each entry
                in the dictionary defines a sequence of manual tests
                that, once performed in the specified order, fully
                test a device.

reset(): Cleanup function that will be called if a running test fails
         at any time. This function should catch any exceptions that
         might happen as a result of the cleanup actions (such as
         trying to reset a device that has been removed, etc).
"""

from typing import Any
import time
from subprocess import run
import uuid
import shutil
import os
import serial  # type: ignore
import serial.tools.list_ports  # type: ignore
import usb.core  # type: ignore
import encode_usb_strings
from usb_test import IceFlasher

# Locations for external utilities and files referenced by the test
# program
parameters = {
    'iceprog': 'tillitis-iceprog',
    'chprog': 'chprog',
    'app_gateware': 'binaries/top.bin',
    'ch552_firmware': 'binaries/usb_device_cdc.bin',
    'ch552_firmware_blank': 'binaries/blank.bin',
    'ch552_firmware_injected': '/tmp/ch552_fw_injected.bin',
    'pico_bootloader_source': 'binaries/main.uf2',
    'pico_bootloader_target_dir': '/media/lab/RPI-RP2/'
}

tp1_pins = {
    '5v_en': 7,
    'tx': 8,
    'rx': 9,
    'sck': 10,
    'mosi': 11,
    'ss': 12,
    'miso': 13,
    'crst': 14,
    'cdne': 15,
    'rts': 16,
    'cts': 17,
    'gpio1': 18,
    'gpio2': 19,
    'gpio3': 20,
    'gpio4': 21,
}


def enable_power() -> bool:
    """Enable power to the TK-1"""
    d = IceFlasher()
    d.gpio_set_direction(tp1_pins['5v_en'], True)
    d.gpio_put(tp1_pins['5v_en'], True)
    time.sleep(0.3)

    return True


def disable_power() -> bool:
    """Disable power to the TK-1"""
    time.sleep(.1)
    d = IceFlasher()
    d.gpio_set_direction(tp1_pins['5v_en'], True)
    d.gpio_put(tp1_pins['5v_en'], False)

    return True


def measure_voltages(device: IceFlasher,
                     sample_count: int) -> dict[str, float]:
    """Measure the voltage levels of the tk-1 power rails

    Keyword arguments:
    device -- programmer
    sample_count -- number of samples to average
    """
    adc_vals = [0.0, 0.0, 0.0]
    for i in range(0, sample_count):
        adc_vals = [
            total + sample for total,
            sample in zip(
                adc_vals,
                device.adc_read_all())]

    adc_vals = [total / sample_count for total in adc_vals]

    return dict(zip(['1.2', '2.5', '3.3'], adc_vals))


def voltage_test() -> bool:
    """Measure 3.3V 2.5V, and 1.2V voltage rails on the TK-1"""

    enable_power()

    d = IceFlasher()
    vals = measure_voltages(d, 20)
    d.close()

    disable_power()

    print(
        'voltages:',
        ', '.join(
            '{:}V:{:.3f}'.format(
                val[0],
                val[1]) for val in vals.items()))
    if (
        (abs(vals['1.2'] - 1.2) > .2)
        | (abs(vals['2.5'] - 2.5) > .2)
        | (abs(vals['3.3'] - 3.3) > .2)
    ):
        return False

    return True


def flash_validate_id() -> bool:
    """Read the ID from TK-1 SPI flash, and verify that it matches the expected value"""
    result = run([
        parameters['iceprog'],
        '-t'
    ],
        capture_output=True)
    disable_power()

    err = result.stderr.split(b'\n')
    for line in err:
        if line.startswith(b'flash ID:'):
            vals_b = line.split(b' 0x')[1:]
            flash_id = int(b''.join(vals_b), 16)
            print(line, hex(flash_id))

            # Note: Flash IDs reported by iceprog are slightly wrong
            flash_types = {
                0xb40140b40140b40140b40140b4014: 'XT25F08BDFIGT-S (MTA1-USB-V1)',
                0xef401400: 'W25Q80DVUXIE (TK-1)'}

            flash_type = flash_types.get(flash_id)

            if flash_type is None:
                print('Flash ID invalid')
                return False
            print('Detected flash type: {:}'.format(flash_type))
            return True

    return result.returncode == 0


def flash_program() -> bool:
    """Program and verify the TK-1 SPI flash with the application test gateware"""
    result = run([
        parameters['iceprog'],
        parameters['app_gateware']
    ])
    disable_power()
    print(result)

    return result.returncode == 0


def flash_check() -> bool:
    """Verify the TK-1 SPI flash is programmed with the application test gateware"""
    result = run([
        parameters['iceprog'],
        '-c',
        parameters['app_gateware']
    ])
    disable_power()
    print(result)

    return result.returncode == 0


def test_extra_io() -> bool:
    """Test the TK-1 RTS, CTS, and GPIO1-4 lines by measuring a test pattern generated by the app_test gateware"""

    d = IceFlasher()
    for pin in tp1_pins.values():
        d.gpio_set_direction(pin, False)
    d.close()

    disable_power()
    time.sleep(1)
    enable_power()

    time.sleep(0.2)

    d = IceFlasher()
    d.gpio_put(tp1_pins['rts'], False)
    d.gpio_set_direction(tp1_pins['rts'], True)

    expected_results = [1 << (i % 5) for i in range(9, -1, -1)]

    results = []
    for i in range(0, 10):
        vals = d.gpio_get_all()
        pattern = (vals >> 17) & 0b11111
        # print(f'{vals:016x} {pattern:04x}')
        results.append(pattern)

        d.gpio_put(tp1_pins['rts'], True)
        d.gpio_put(tp1_pins['rts'], False)

    d.gpio_set_direction(tp1_pins['rts'], False)
    d.close()

    disable_power()

    print(results, expected_results, results == expected_results)
    return results == expected_results


def test_found_bootloader() -> bool:
    """Search for a CH552 in USB bootloader mode"""
    print('\n\n\nSearching for CH552 bootloader, plug in USB cable now (times out in 10 seconds)!')
    for _ in range(0, 100):  # retry every 0.1s, up to 10 seconds
        devices = usb.core.find(
            idVendor=0x4348,
            idProduct=0x55e0,
            find_all=True)
        count = len(list(devices))

        if count == 1:
            return True

        time.sleep(0.1)

    post = usb.core.find(
        idVendor=0x4348,
        idProduct=0x55e0,
        find_all=True)
    post_count = len(list(post))
    return post_count == 1


def inject_serial_number(
        infile: str,
        outfile: str,
        serial_num: str) -> None:
    """Inject a serial number into the specified CH552 firmware file"""
    magic = encode_usb_strings.string_to_descriptor(
        "68de5d27-e223-4874-bc76-a54d6e84068f")
    replacement = encode_usb_strings.string_to_descriptor(serial_num)

    f = bytearray(open(infile, 'rb').read())

    pos = f.find(magic)

    if pos < 0:
        print('failed to find magic string')
        exit(1)

    f[pos:(pos + len(magic))] = replacement

    with open(outfile, 'wb') as of:
        of.write(f)


def flash_ch552(serial_num: str) -> bool:
    """Flash an attached CH552 device with the USB CDC firmware, injected with the given serial number"""

    print(serial_num)
    inject_serial_number(
        parameters['ch552_firmware'],
        parameters['ch552_firmware_injected'],
        serial_num)

    # Program the CH552 using CHPROG
    result = run([
        parameters['chprog'],
        parameters['ch552_firmware_injected']
    ])
    print(result)
    return result.returncode == 0


def erase_ch552() -> bool:
    """Erase an attached CH552 device"""

    # Program the CH552 using CHPROG
    result = run([
        parameters['chprog'],
        parameters['ch552_firmware_blank']
    ])
    print(result)
    return result.returncode == 0


def find_serial_device(desc: dict[str, Any]) -> str:
    """Look for a serial device that has the given attributes"""

    for port in serial.tools.list_ports.comports():
        matched = True
        for key, value in desc.items():
            if not getattr(port, key) == value:
                matched = False

        if matched:
            print(port.device)
            return port.device

    raise NameError('Serial device not found')


def find_ch552(serial_num: str) -> bool:
    """Search all serial devices for one that has the correct description and serial number"""
    time.sleep(1)

    description = {
        'vid': 0x1207,
        'pid': 0x8887,
        'manufacturer': 'Tillitis',
        'product': 'MTA1-USB-V1',
        'serial_number': serial_num
    }

    try:
        find_serial_device(description)
    except NameError:
        return False

    return True


def ch552_program() -> bool:
    """Load the CDC ACM firmware onto a CH552 with a randomly generated serial number, and verify that it boots correctly"""
    if not test_found_bootloader():
        print('Error finding CH552!')
        return False

    serial = str(uuid.uuid4())

    if not flash_ch552(serial):
        print('Error flashing CH552!')
        return False

    if not find_ch552(serial):
        print('Error finding flashed CH552!')
        return False

    return True


def ch552_erase() -> bool:
    """Erase the firmware from a previously programmed CH552"""
    if not test_found_bootloader():
        print('Error finding CH552!')
        return False

    if not erase_ch552():
        print('Error erasing CH552!')
        return False

    return True


def test_txrx_touchpad() -> bool:
    """Test UART communication, RGB LED, and touchpad by asking the operator to interact with the touch pad"""
    description = {
        'vid': 0x1207,
        'pid': 0x8887,
        'manufacturer': 'Tillitis',
        'product': 'MTA1-USB-V1'
    }

    s = serial.Serial(
        find_serial_device(description),
        9600,
        timeout=.2)

    if not s.isOpen():
        print('couldn\'t find/open serial device')
        return False

    for _ in range(0, 5):
        # Attempt to clear any buffered data from the serial port
        s.write(b'0123')
        time.sleep(0.2)
        s.read(20)

        try:
            s.write(b'0')
            [count, touch_count] = s.read(2)
            print(
                'read count:{:}, touch count:{:}'.format(
                    count, touch_count))

            input(
                '\n\n\nPress touch pad once and check LED, then press Enter')
            s.write(b'0')
            [count_post, touch_count_post] = s.read(2)
            print(
                'read count:{:}, touch count:{:}'.format(
                    count_post,
                    touch_count_post))

            if (count_post -
                count != 1) or (touch_count_post -
                                touch_count != 1):
                print('Unexpected values returned, trying again')
                continue

            return True
        except ValueError as e:
            print(e)
            continue

    print('Max retries exceeded, failure!')
    return False


def program_pico() -> bool:
    """Load the ice40 flasher firmware onto the TP-1"""
    print('Attach test rig to USB (times out in 10 seconds)')

    firmware_filename = os.path.split(
        parameters['pico_bootloader_source'])[1]

    for _ in range(0, 100):  # retry every 0.1s
        try:
            shutil.copyfile(
                parameters['pico_bootloader_source'],
                parameters['pico_bootloader_target_dir'] +
                firmware_filename)

            # TODO: Test if the pico identifies as a USB-HID device
            # after programming

            return True
        except FileNotFoundError:
            time.sleep(0.1)
        except PermissionError:
            time.sleep(0.1)

    return False


def sleep_2() -> bool:
    """Sleep for 2 seconds"""
    time.sleep(2)
    return True


manual_tests = [
    program_pico,
    voltage_test,
    flash_validate_id,
    flash_program,
    flash_check,
    test_extra_io,
    ch552_program,
    ch552_erase,
    test_txrx_touchpad,
    enable_power,
    disable_power
]

test_sequences = {
    'tk1_test_sequence': [
        voltage_test,
        flash_validate_id,
        flash_program,
        sleep_2,
        test_extra_io,
        ch552_program,
        test_txrx_touchpad
    ],
    'tp1_test_sequence': [
        program_pico,
        sleep_2,
        flash_validate_id
    ],
    'mta1_usb_v1_programmer_test_sequence': [
        program_pico,
        sleep_2,
        voltage_test,
        flash_validate_id,
        sleep_2,
        test_extra_io
    ],
}

# This function will be called if a test fails


def reset() -> None:
    """Attempt to reset the board after test failure"""
    try:
        disable_power()
    except AttributeError as e:
        pass
    except OSError as e:
        pass
    except ValueError as e:
        pass


if __name__ == "__main__":
    # Runs the non-interactive production tests continuously in a
    # random order, to look for interaction bugs

    import random
    import pynvcm
    
    def nvcm_read_info() -> bool:
        tp1_pins = {
            '5v_en': 7,
            'sck': 10,
            'mosi': 11,
            'ss': 12,
            'miso': 13,
            'crst': 14,
            'cdne': 15
        }
    
        pynvcm.sleep_flash(tp1_pins, 15)
    
        nvcm = pynvcm.Nvcm(tp1_pins, 15)
        nvcm.power_on()
        nvcm.init()
        nvcm.nvcm_enable()
        nvcm.info()
        nvcm.power_off()
        return True
    
    def nvcm_verify_blank() -> bool:
        tp1_pins = {
            '5v_en': 7,
            'sck': 10,
            'mosi': 11,
            'ss': 12,
            'miso': 13,
            'crst': 14,
            'cdne': 15
        }
    
        pynvcm.sleep_flash(tp1_pins, 15)
    
        nvcm = pynvcm.Nvcm(tp1_pins, 15)
        nvcm.power_on()
        nvcm.init()
        nvcm.nvcm_enable()
        nvcm.trim_blank_check()
        nvcm.power_off()
        return True
    
    
    tests = [
        nvcm_read_info,
        nvcm_verify_blank,
        voltage_test,
        flash_validate_id,
        flash_program,
        flash_check,
        test_extra_io,
        enable_power,
        disable_power
    ]
    
    parameters['iceprog'] = '/home/matt/repos/tillitis--icestorm/iceprog/iceprog'
    
    while True:
        i = random.randint(0, (len(tests) - 1))
        print(f'\n\n{i} running: {tests[i].__name__}')
        if not tests[i]():
            raise Exception('oops')
