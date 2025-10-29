# Copyright (C) 2024 - Tillitis AB
# SPDX-License-Identifier: BSD-2-Clause

# image produced by build-image targets
BUILDIMAGE=tkey-builder-local

# default image used when running a container
IMAGE=ghcr.io/tillitis/tkey-builder:5rc2

all:
	@echo "Targets:"
	@echo "run                        Run a shell using image '$(IMAGE)' (Podman)"
	@echo "run-make                   Build the FPGA bitstream using image '$(IMAGE)' (Podman)"
	@echo "run-make-ch552             Build the CH552 firmware using image '$(IMAGE)' (Podman)"
	@echo "run-tb                     Run all the testbenches using image '$(IMAGE)' (Podman)"
	@echo "run-make-no-clean          Like run-make but without cleaning first, useful for iterative firmware dev"
	@echo "run-make-ch552-no-clean    Like run-make-ch552 but without cleaning first, useful for iterative firmware dev"
	@echo "run-make-clean_fw          Like run-make but cleans only firmware"
	@echo "flash                      Build bitstream and defaultapp.bin then program them and the partition table onto the TKey SPI flash"
	@echo "pull                       Pull down the image '$(IMAGE)' (Podman)"
	@echo "build-image                Build a toolchain image named '$(BUILDIMAGE)' (Podman)"
	@echo "                           A newly built image can be used like: make IMAGE=$(BUILDIMAGE) run"
	@echo "docker-run                 Run a shell using image '$(IMAGE)' (Docker)"
	@echo "docker-build-image         Build a toolchain image named '$(BUILDIMAGE)' (Docker)"

run:
	podman run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it \
	 $(IMAGE) /usr/bin/bash -l

docker-run:
	docker run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it \
	 $(IMAGE) /usr/bin/bash -l

run-make:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga.bin

run-make-ch552:
	podman run --rm --mount type=bind,source="`pwd`/../hw/usb_interface/ch552_fw",target=/build -w /build -it \
	 $(IMAGE) make clean usb_device.bin

run-tb:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_tb tb

run-make-testfw:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga_testfw.bin

run-make-no-clean:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make application_fpga.bin

run-make-ch552-no-clean:
	podman run --rm --mount type=bind,source="`pwd`/../hw/usb_interface/ch552_fw",target=/build -w /build -it \
	 $(IMAGE) make usb_device.bin

run-make-clean_fw:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_fw application_fpga.bin

flash:
	podman run --rm \
	--device /dev/bus/usb/$(lsusb | grep -m 1 1209:8886 | awk '{ printf "%s/%s", $2, substr($4,1,3) }') \
	--mount type=bind,source="`pwd`/../hw/application_fpga",target=/build \
	-w /build \
	-it $(IMAGE) make prog_flash


pull:
	podman pull $(IMAGE)

build-image:
	podman build -t $(BUILDIMAGE) .

docker-build-image:
	docker build -t $(BUILDIMAGE) .
