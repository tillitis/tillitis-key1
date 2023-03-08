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
from iceflasher import IceFlasher

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

TP1_PINS = {
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
    flasher = IceFlasher()
    flasher.gpio_set_direction(TP1_PINS['5v_en'], True)
    flasher.gpio_put(TP1_PINS['5v_en'], True)
    time.sleep(0.3)

    return True


def disable_power() -> bool:
    """Disable power to the TK-1"""
    time.sleep(.1)
    flasher = IceFlasher()
    flasher.gpio_set_direction(TP1_PINS['5v_en'], True)
    flasher.gpio_put(TP1_PINS['5v_en'], False)

    return True


def measure_voltages(device: IceFlasher,
                     sample_count: int) -> dict[str, float]:
    """Measure the voltage levels of the tk-1 power rails

    Keyword arguments:
    device -- programmer
    sample_count -- number of samples to average
    """
    adc_vals = [0.0, 0.0, 0.0]
    for _ in range(0, sample_count):
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

    flasher = IceFlasher()
    vals = measure_voltages(flasher, 20)
    flasher.close()

    disable_power()

    print(
        'voltages:',
        ', '.join(
            f'{val[0]}V:{val[1]:.3f}' for val in vals.items()))
    if (
        (abs(vals['1.2'] - 1.2) > .2)
        | (abs(vals['2.5'] - 2.5) > .2)
        | (abs(vals['3.3'] - 3.3) > .2)
    ):
        return False

    return True


def flash_validate_id() -> bool:
    """Read the ID from TK-1 SPI flash, and compare to known values"""
    result = run([
        parameters['iceprog'],
        '-t'
    ],
        capture_output=True,
        check=True)
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
            print(f'Detected flash type: {flash_type}')
            return True

    return result.returncode == 0


def flash_program() -> bool:
    """Program and verify the TK-1 SPI flash with the application test gateware"""
    result = run([
        parameters['iceprog'],
        parameters['app_gateware']
    ],
        check=True)
    disable_power()
    print(result)

    return result.returncode == 0


def flash_check() -> bool:
    """Verify the TK-1 SPI flash is programmed with the application test gateware"""
    result = run([
        parameters['iceprog'],
        '-c',
        parameters['app_gateware']
    ],
        check=True)
    disable_power()
    print(result)

    return result.returncode == 0


def test_extra_io() -> bool:
    """Test the TK-1 RTS, CTS, and GPIO1-4 lines"""

    flasher = IceFlasher()
    for pin in TP1_PINS.values():
        flasher.gpio_set_direction(pin, False)
    flasher.close()

    disable_power()
    time.sleep(1)
    enable_power()

    time.sleep(0.2)

    flasher = IceFlasher()
    flasher.gpio_put(TP1_PINS['rts'], False)
    flasher.gpio_set_direction(TP1_PINS['rts'], True)

    expected_results = [1 << (i % 5) for i in range(9, -1, -1)]

    results = []
    for _ in range(0, 10):
        vals = flasher.gpio_get_all()
        pattern = (vals >> 17) & 0b11111
        # print(f'{vals:016x} {pattern:04x}')
        results.append(pattern)

        flasher.gpio_put(TP1_PINS['rts'], True)
        flasher.gpio_put(TP1_PINS['rts'], False)

    flasher.gpio_set_direction(TP1_PINS['rts'], False)
    flasher.close()

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

    with open(infile, 'rb') as fin:
        firmware_data = bytearray(fin.read())

    pos = firmware_data.find(magic)

    if pos < 0:
        raise ValueError('failed to find magic string')

    firmware_data[pos:(pos + len(magic))] = replacement

    with open(outfile, 'wb') as fout:
        fout.write(firmware_data)


def flash_ch552(serial_num: str) -> bool:
    """Flash an attached CH552 device with the USB CDC firmware"""

    print(serial_num)
    inject_serial_number(
        parameters['ch552_firmware'],
        parameters['ch552_firmware_injected'],
        serial_num)

    # Program the CH552 using CHPROG
    result = run([
        parameters['chprog'],
        parameters['ch552_firmware_injected']
    ],
        check=True)
    print(result)
    return result.returncode == 0


def erase_ch552() -> bool:
    """Erase an attached CH552 device"""

    # Program the CH552 using CHPROG
    result = run([
        parameters['chprog'],
        parameters['ch552_firmware_blank']
    ],
        check=True)
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
    """Search for a serial device that has the correct description"""
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
    """Load the firmware onto a CH552, and verify that it boots"""
    if not test_found_bootloader():
        print('Error finding CH552!')
        return False

    serial_num = str(uuid.uuid4())

    if not flash_ch552(serial_num):
        print('Error flashing CH552!')
        return False

    if not find_ch552(serial_num):
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
    """Test UART communication, RGB LED, and touchpad"""
    description = {
        'vid': 0x1207,
        'pid': 0x8887,
        'manufacturer': 'Tillitis',
        'product': 'MTA1-USB-V1'
    }

    dev = serial.Serial(
        find_serial_device(description),
        9600,
        timeout=.2)

    if not dev.isOpen():
        print('couldn\'t find/open serial device')
        return False

    for _ in range(0, 5):
        # Attempt to clear any buffered data from the serial port
        dev.write(b'0123')
        time.sleep(0.2)
        dev.read(20)

        try:
            dev.write(b'0')
            [count, touch_count] = dev.read(2)
            print(
                f'read count:{count}, touch count:{touch_count}')

            input(
                '\n\n\nPress touch pad once and check LED, then press Enter')
            dev.write(b'0')
            [count_post, touch_count_post] = dev.read(2)
            print(
                'read count:{count_post}, touch count:{touch_count_post}')

            if (count_post -
                count != 1) or (touch_count_post -
                                touch_count != 1):
                print('Unexpected values returned, trying again')
                continue

            return True
        except ValueError as error:
            print(error)
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
    except AttributeError:
        pass
    except OSError:
        pass
    except ValueError:
        pass


def random_test_runner() -> None:
    """"Run the non-interactive production tests in a random order

    This routine is intended to be used for finding edge-cases with
    the production tests. It runs the non-interactive tests (as well
    as some nondestructive tests from the nvcm module) in a random
    order, and runs continuously.
    """

    def nvcm_read_info() -> bool:
        """Check that the nvcm read info command runs"""
        pynvcm.sleep_flash(TP1_PINS, 15)
        nvcm = pynvcm.Nvcm(TP1_PINS, 15)
        nvcm.power_on()
        nvcm.init()
        nvcm.nvcm_enable()
        nvcm.info()
        nvcm.power_off()
        return True

    def nvcm_verify_blank() -> bool:
        """Verify that the NVCM memory is blank"""
        pynvcm.sleep_flash(TP1_PINS, 15)
        nvcm = pynvcm.Nvcm(TP1_PINS, 15)
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

    pass_count = 0
    while True:
        i = random.randint(0, (len(tests) - 1))
        print(f'\n\n{pass_count}: running: {tests[i].__name__}')
        if not tests[i]():
            sys.exit(1)
        pass_count += 1


if __name__ == "__main__":
    import random
    import pynvcm
    import sys

    random_test_runner()
