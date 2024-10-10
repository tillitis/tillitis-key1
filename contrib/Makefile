# Copyright (C) 2024 - Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only

# image produced by build-image targets
BUILDIMAGE=tkey-builder-local

# default image used when running a container
IMAGE=ghcr.io/tillitis/tkey-builder:4

all:
	@echo "Targets:"
	@echo "run                   Run a shell using image '$(IMAGE)' (Podman)"
	@echo "run-make              Build the FPGA bitstream using image '$(IMAGE)' (Podman)"
	@echo "run-tb                Run all the testbenches using image '$(IMAGE)' (Podman)"
	@echo "run-make-no-clean     Like run-make but without cleaning first, useful for iterative firmware dev"
	@echo "run-make-clean_fw     Like run-make but cleans only firmware"
	@echo "flash                 Program the SPI flash on the TKey - needs an existing bitstream"
	@echo "pull                  Pull down the image '$(IMAGE)' (Podman)"
	@echo "build-image           Build a toolchain image named '$(BUILDIMAGE)' (Podman)"
	@echo "                      A newly built image can be used like: make IMAGE=$(BUILDIMAGE) run"
	@echo "docker-run            Run a shell using image '$(IMAGE)' (Docker)"
	@echo "docker-build-image    Build a toolchain image named '$(BUILDIMAGE)' (Docker)"

run:
	podman run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it \
	 $(IMAGE) /usr/bin/bash

docker-run:
	docker run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it \
	 $(IMAGE) /usr/bin/bash

run-make:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga.bin

run-make-spi:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga.bin YOSYS_FLAG=-DINCLUDE_SPI_MASTER


run-tb:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_tb tb

run-make-testfw:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga_testfw.bin

run-make-no-clean:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make application_fpga.bin

run-make-clean_fw:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_fw application_fpga.bin

flash:
	podman run --rm \
	--device /dev/bus/usb/$(lsusb | grep -m 1 1209:8886 | awk '{ printf "%s/%s", $2, substr($4,1,3) }') \
	--mount type=bind,source="`pwd`/../hw/application_fpga",target=/build \
	-w /build/hw/application_fpga \
	-it $(IMAGE) tillitis-iceprog /build/application_fpga.bin


pull:
	podman pull $(IMAGE)

build-image:
	podman build -t $(BUILDIMAGE) .

docker-build-image:
	docker build -t $(BUILDIMAGE) .
