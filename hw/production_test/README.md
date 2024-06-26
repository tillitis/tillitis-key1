# TK-1 and TP-1 production tests

Production tests for the TK-1 and TP-1 PCBs

## Usage

These instructions are tested on Ubuntu 22.10.

Set up a python virtualenv to run the production test:

    sudo apt install python3.10-venv
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    deactivate

To run the production test script:

    source venv/bin/activate
    ./production_test.py

The script will then print a menu with all available tests:

    Tillitis TK-1 and TP-1 Production tests



    === Test sequences ===
    1. tk1_test_sequence: voltage_test, flash_validate_id, flash_program, sleep_2, test_extra_io, ch552_program, test_txrx_touchpad
    2. tp1_test_sequence: program_pico, sleep_2, flash_validate_id
    3. mta1_usb_v1_programmer_test_sequence: program_pico, sleep_2, voltage_test, flash_validate_id, sleep_2, test_extra_io

    === Manual tests ===
    4. program_pico: Load the ice40 flasher firmware onto the TP-1
    5. voltage_test: Measure 3.3V 2.5V, and 1.2V voltage rails on the TK-1
    6. flash_validate_id: Read the ID from TK-1 SPI flash, and verify that it matches the expected value
    7. flash_program: Program and verify the TK-1 SPI flash with the application test gateware
    8. flash_check: Verify the TK-1 SPI flash is programmed with the application test gateware
    9. test_extra_io: Test the TK-1 RTS, CTS, and GPIO1-4 lines by measuring a test pattern generated by the app_test gateware
    10. ch552_program: Load the CDC ACM firmware onto a CH552 with a randomly generated serial number, and verify that it boots correctly
    11. test_txrx_touchpad: Test UART communication, RGB LED, and touchpad by asking the operator to interact with the touch pad
    12. enable_power: Enable power to the TK-1
    13. disable_power: Disable power to the TK-1



    Please type an option number and press return:


There are two types of tests listed: test sequences, which are used
to run full production tests, and manual tests, which are used to
test a single subcircuit. It is recommended to test all boards using
a test sequence first, and to use the manual tests for diagnosing
issues with individual boards, or for re-testing repaired boards.

## Test Sequences

These sequences are used as production tests for the TK-1 and TP-1.

### TK-1 production test (tk1_test_sequence)

This test checks all major subcircuits on the TK-1, and is used to
verify that the PCBA was assembled correctly. It should be run on all
newly assembled TK-1 boards.

Requirements:

* Programmed MTA1_USB_V1_Programmer board, fitted with a wider (green)
  plastic clip
* Unprogrammed TK-1
* USB micro cable to attach TP-1 board to computer
* USB C extension cable to attach TK-1 to computer

It runs the following tests, in order:

1. voltage_test
2. flash_validate_id
3. flash_program
4. sleep_2
5. test_extra_io
6. ch552_program
7. test_txrx_touchpad

Note: If the CH552 has been programmed already, then the test
sequence will fail. In that case, manually run the other tests
in the sequence, skipping the 'ch554_program' test.

### TP-1 (tp1_test_sequence)

This test programs a TP-1, then tests that it can program a TK-1.
It should be run on all newly assembled TP-1 boards.


Requirements:

* Unprogrammed TP-1
* TK-1 programmed with the application test gateware
* USB micro cable to attach TP-1 board to computer

The TP-1 production test runs the following tests, in order:

1. program_pico
2. sleep_2
3. flash_validate_id

### MTA1_USB_V1_Programmer (mta1_usb_v1_programmer_test_sequence)

Requirements:

* Unprogrammed MTA1_USB_V1_Programmer
* TK-1 programmed with the application test gateware
* USB micro cable to attach MTA1_USB_V1_Programmer board to computer

The TP-1 production test runs the following tests, in order:

1. program_pico
2. sleep_2
3. voltage_test
4. flash_validate_id
5. sleep_2
6. test_extra_io

## Individual tests

These tests target a specific sub-circuit or funcationality on the
TP-1 and TK-1. Each test is designed to be run successfully in
isolation.

### program_pico: Load the ice40 flasher firmware onto the TP-1

This test loads the ice40_flasher firmware into the TP-1.

Usage instructions:
1. Attach an unprogrammed TP-1 to the computer using a micro USB
   cable.
2. Run the test.
3. The test program will copy the firmware onto the TP-1.
4. Once the firmware is copied, the TP-1 will automatically reset,
   and re-initialize as a ice40_flasher.

Notes:
* This test assumes that the computer is configured to auto-mount USB
storage devices, and that the Pico will be mounted to
/media/lab/RPI-RP2. This is true for a computer runnnig Ubuntu 22.10
when a user is logged into a Gnome GUI session. The script will need
to be adjusted for other environments.

### voltage_test: Measure 3.3V 2.5V, and 1.2V voltage rails on the TK-1

This test uses ADC in the Pico to measure the power supplies on the
TK-1. It samples the voltage of each power supply multiple times,
averages the result, and verifies that they are within +/-0.2V of the
specification.

Usage instructions:
1. Attach a programmed/tested MTA1_USB_V1_Programmer to the computer
   using a micro USB cable.
2. Place a TK-1 into the MTA1_USB_V1_Programmer. The TK-1 can be
   programmed or unprogrammed.
3. Run the test.
4. The test will use the MTA1_USB_V1_Programmer to power on the TK-1,
   measure the voltage rails, then power off the TK-1.
5. The test will report the measurements and pass/fail metric.

Notes:
* The accuracy of the ADC is poor; external hardware would be
  required to do a more extensive test. The power supplies used are
  all fixed-voltage devices, so the chance of of an off-spec (but
  still working) device is considered to be low.
* This test does not verify that the power sequencing is correct.

### flash_validate_id: Read the ID from TK-1 SPI flash, and verify that it's not all 0's or 1's

This test uses the TP-1 or MTA1_USB_V1_Programmer to read a TK-1 SPI
flash ID. It can be used to quickly check if a TK-1 device is
inserted properly into the programmger.

Usage instructions:
1. Attach a programmed/tested MTA1_USB_V1_Programmer or TP-1 to the
   computer using a micro USB cable.
2. Place a TK-1 into the MTA1_USB_V1_Programmer. The TK-1 can be
   programmed or unprogrammed.
3. Run the test.
4. The test will use the programmer to power on the TP-1, read out
   the SPI flash ID, then power off the TK-1.
5. The test will check if the flash ID matches any known valid flash
   ID types.
6. If the flash ID matches an known value, the test will print the
   type and return a pass metric.
7. If the flash ID does not match a known value, the test will print
   the type and return a pass metric.

Notes:
* An earlier version of this test just checked if the flash ID was
  0x00000000 or 0xFFFFFFFF; this version is more exact.

### flash_program: Program and verify the TK-1 SPI flash with the application test gateware

This test uses the TP-1 or MTA1_USB_V1_Programmer to write the
application test gateware a TK-1 SPI flash. This gateware is needed
to run the test_extra_io and test_txrx_touchpad tests. The test uses
the external iceprog utility to perform the flash operation.

Usage instructions:

1. Attach a programmed/tested MTA1_USB_V1_Programmer or TP-1 to the
   computer using a micro USB cable.
2. Place a TK-1 into the MTA1_USB_V1_Programmer. The TK-1 can be
   programmed or unprogrammed.
3. Run the test.
4. The test will use the programmer to power on the TP-1, program the
   SPI flash with the gateware, verify the flash by reading the data
   back out, then power off the TK-1.
5. The test will report a pass/fail metric vased on the result of the
   verification phase.

### flash_check: Verify the TK-1 SPI flash is programmed with the application test gateware

This test uses the TP-1 or MTA1_USB_V1_Programmer to verify that the
application test gateware is written to a TK-1 SPI flash. The test
uses the external iceprog utility to perform the verification
operation.

Usage instructions:

1. Attach a programmed/tested MTA1_USB_V1_Programmer or TP-1 to the
   computer using a micro USB cable.
2. Place a programmed TK-1 into the MTA1_USB_V1_Programmer.
3. Run the test.
4. The test will use the programmer to power on the TP-1, verify the
   flash by reading the data back out, then power off the TK-1.
5. The test will report a pass/fail metric vased on the result of the
   verification phase.

### test_extra_io: Test the TK-1 RTS, CTS, and GPIO1-4 lines by measuring a test pattern generated by the app_test gateware

This test uses MTA1_USB_V1_Programmer to verify that the RTS, CTS, and
GPIO1-4 lines are connected correctly to the ICE40 FPGA.

On the FPGA side, the application gateware implements a simple state
machine for this test. The RTS line is configured as an input, and
the CTS, GPIO1, GPIO2, GPIO3, and GPIO4 lines are configured as
outputs. The values of the outputs are configured so that only one
output is high at a time, while the rest are low. After reset, the
GPIO4 line is high. Each time the RTS line is toggled, the next
output on the list is set high.

On the programmer side, the Pico GPIO pin connected to the TK-1 RTS
line is configured as an output, and the Pico GPIO pins connected to
the other TK-1 line are configured as inputs. The pico checks that
each input line is working by cycling the RTS output high and low,
then reading the values of each input. This process is repeated 5
times until all output lines are measured, then the results are
compared to a table of expected values.

Usage instructions:

1. Attach a programmed/tested MTA1_USB_V1_Programmer or TP-1 to the
   computer using a micro USB cable.
2. Place a programmed TK-1 into the MTA1_USB_V1_Programmer.
3. Run the test.
4. The test will use the programmer to power on the TP-1, run the IO
   test, then power off the TK-1.
5. The test will report a pass/fail metric vased on the result of the
   test.

### ch552_program: Load the CDC ACM firmware onto a CH552 with a randomly generated serial number, and verify that it boots correctly

TODO

### test_txrx_touchpad: Test UART communication, RGB LED, and touchpad by asking the operator to interact with the touch pad

TODO

### enable_power: Enable power to the TK-1

This test uses the TP-1 or MTA1_USB_V1_Programmer to enable power to
an attached TK-1. This isn't a functional test, but can be used for
manual testing such as measuring voltage rails with a multimeter,
probing clock signals with an oscilloscope, etc.

Usage instructions:

1. Attach a programmed/tested MTA1_USB_V1_Programmer or TP-1 to the
   computer using a micro USB cable.
2. Place a TK-1 into the MTA1_USB_V1_Programmer. The TK-1 can be
   programmed or unprogrammed.
3. Run the test.
4. The test will use the programmer to power on the TP-1
5. The test will report a pass metric if the command completed
   successfully.

### disable_power: Disable power to the TK-1

This test uses the TP-1 or MTA1_USB_V1_Programmer to disable power to
an attached TK-1. This isn't a functional test, but can be used after
the enable_power command was used to turn on a device.

Usage instructions:

1. Attach a programmed/tested MTA1_USB_V1_Programmer or TP-1 to the
   computer using a micro USB cable.
2. Place a TK-1 into the MTA1_USB_V1_Programmer. The TK-1 can be
   programmed or unprogrammed.
3. Run the test.
4. The test will use the programmer to power off the TP-1
5. The test will report a pass metric if the command completed
   successfully.

## Firmware binaries

To make the test environment easier to set up, some pre-compiled
binares are included in the binaries/ subdirectory. These can also be
built from source, by following the below instructions.

Before building the firmware, follow the [toolchain setup](https://github.com/tillitis/tillitis-key1/blob/main/doc/toolchain_setup.md)
instructions.

### CH552 firmware

See the build instructions in the
[ch552_fw](../usb_interface/ch552_fw/README.md) directory.

### Test Application gateware

This uses the Symbiflow toolchain:

    cd ~/tillitis-key1/hw/production_test/application_fpga_test_gateware
    make
    cp top.bin ../binaries/

### TP-1 Raspberry Pi Pico firmware

Follow the instructions in the [ice40_flasher](https://github.com/Blinkinlabs/ice40_flasher#building-the-firmware)
repository, then copy the output 'main.uf2' file to the binaries
directory.
