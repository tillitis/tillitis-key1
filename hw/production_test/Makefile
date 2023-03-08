SHELL := /bin/bash

PYTHON_FILES = \
    encode_usb_strings.py \
    production_test_runner.py \
    production_tests.py \
    pybin2nvcm.py \
    pynvcm.py \
    random_production_test.py \
    reset.py \
    usb_test.py


# autopep8: Fixes simple format errors automatically
# mypy: static type hint analysis
# pylint: pep8 and static code analysis
lint:
	autopep8 --in-place --max-line-length 70 --aggressive --aggressive  ${PYTHON_FILES}
	mypy --disallow-untyped-defs ${PYTHON_FILES}
	pylint --generated-member=usb1.TRANSFER_COMPLETED,usb1.USBErrorInterrupted,usb1.USBErrorIO ${PYTHON_FILES}

# Check that the NVCM generator gives a correct output for a known binary
verify-pybin2nvcm:
	./pybin2nvcm.py nvcm_test/application_fpga.bin verify.nvcm
	cmp verify.nvcm nvcm_test/application_fpga.nvcm

verify-nvcm:
	time ./pynvcm.py --verify nvcm_test/application_fpga.bin

program-nvcm-danger:
	./pynvcm.py -i
	time ./pynvcm.py --my-design-is-good-enough --ignore-blank --write ../application_fpga/application_fpga.bin --verify nvcm_test/application_fpga.bin
	./pynvcm.py -b

randomize-production-test:
	./production_tests.py
