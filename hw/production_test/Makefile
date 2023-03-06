SHELL := /bin/bash

PYTHON_FILES = \
    usb_test.py \
    icenvcm.py \
    icebin2nvcm.py \
    encode_usb_strings.py \
    reset.py

# autopep8: Fixes simple format errors automatically
# mypy: static type hint analysis
# pylint: pep8 and static code analysis
lint:
	autopep8 --in-place --max-line-length 70 --aggressive --aggressive  ${PYTHON_FILES}
	mypy --disallow-untyped-defs ${PYTHON_FILES}
	pylint --generated-member=usb1.TRANSFER_COMPLETED,usb1.USBErrorInterrupted,usb1.USBErrorIO ${PYTHON_FILES}

# Check that the NVCM generator gives a correct output for a known binary
verify-nvcm:
	./icebin2nvcm.py nvcm_test/application_fpga.bin verify.nvcm
	cmp verify.nvcm nvcm_test/application_fpga.nvcm

verify:
	time ./icenvcm.py --verify nvcm_test/application_fpga.bin

program:
	./icenvcm.py -i
	time ./icenvcm.py --my-design-is-good-enough --ignore-blank --write ../application_fpga/application_fpga.bin --verify nvcm_test/application_fpga.bin
	./icenvcm.py -b
