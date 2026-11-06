# SPDX-FileCopyrightText: 2024 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

# image produced by build-image targets
BUILDIMAGE=tkey-builder-local

# platforms built by the build-image-multiarch targets
PLATFORMS=linux/amd64,linux/arm64

# default image used when running a container
IMAGE=ghcr.io/tillitis/tkey-builder:5rc2

PACKAGE ?=

all:
	@echo "Targets:"
	@echo "run                              Run a shell using image '$(IMAGE)' (Podman)"
	@echo "run-make PACKAGE=...             Build the FPGA bitstream using image '$(IMAGE)' (Podman)"
	@echo "run-make-ch552                   Build the CH552 firmware using image '$(IMAGE)' (Podman)"
	@echo "run-tb PACKAGE=...               Run all the testbenches using image '$(IMAGE)' (Podman)"
	@echo "run-make-no-clean PACKAGE=...    Like run-make but without cleaning first, useful for iterative firmware dev"
	@echo "run-make-ch552-no-clean          Like run-make-ch552 but without cleaning first, useful for iterative firmware dev"
	@echo "run-make-clean_fw PACKAGE=...    Like run-make but cleans only firmware"
	@echo "export-artifacts PACKAGE=...     Build release artifacts locally and collect them in ./artifacts"
	@echo "flash                            Build bitstream and defaultapp.bin then program them and the partition table onto the TKey SPI flash"
	@echo "pull                             Pull down the image '$(IMAGE)' (Podman)"
	@echo "build-image                      Build a toolchain image named '$(BUILDIMAGE)' (Podman)"
	@echo "                                 A newly built image can be used like: make IMAGE=$(BUILDIMAGE) run"
	@echo "build-image-multiarch            Build a multiarch toolchain image named '$(BUILDIMAGE)' for $(PLATFORMS) (Podman)"
	@echo "docker-run                       Run a shell using image '$(IMAGE)' (Docker)"
	@echo "docker-build-image               Build a toolchain image named '$(BUILDIMAGE)' (Docker)"
	@echo "docker-build-image-multiarch     Build a multiarch toolchain image named '$(BUILDIMAGE)' for $(PLATFORMS) (Docker)"
	@echo ""
	@echo "Configuration Options:"
	@echo "PACKAGE=...                      Target hardware package. Allowed PACKAGE values: sg48, uwg30"
	@echo ""
# Find the TP1 programmer to be able to import the device into the
# container.
tpdev != lsusb -d 1209:8886 | awk '{ printf "%s/%s", $$2, substr($$4,1,3) }'

ifneq ($(strip $(tpdev)),)
tp = --device /dev/bus/usb/$(tpdev)
endif

#-------------------------------------------------------------------
# Canned recipe: check that PACKAGE is set at recipe time.
#-------------------------------------------------------------------
define CHECK_PACKAGE
	@if [ -z "$(PACKAGE)" ]; then \
			echo "PACKAGE variable is not defined. Please run 'make <target> PACKAGE=...'"; \
		exit 1; \
	fi
endef

run:
	podman run --rm \
	$(tp) \
	--mount type=bind,source="`pwd`/../",target=/build \
	-w /build \
	-it $(IMAGE) /usr/bin/bash -l

docker-run:
	docker run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it \
	 $(IMAGE) /usr/bin/bash -l

run-make:
	$(CHECK_PACKAGE)
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga.bin PACKAGE=$(PACKAGE)

run-make-ch552:
	podman run --rm --mount type=bind,source="`pwd`/../hw/usb_interface/ch552_fw",target=/build -w /build -it \
	 $(IMAGE) make clean usb_device.bin

run-tb:
	$(CHECK_PACKAGE)
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_tb tb PACKAGE=$(PACKAGE)

run-make-testfw:
	$(CHECK_PACKAGE)
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga_testfw.bin PACKAGE=$(PACKAGE)

run-make-no-clean:
	$(CHECK_PACKAGE)
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make application_fpga.bin PACKAGE=$(PACKAGE)

run-make-ch552-no-clean:
	podman run --rm --mount type=bind,source="`pwd`/../hw/usb_interface/ch552_fw",target=/build -w /build -it \
	 $(IMAGE) make usb_device.bin

run-make-clean_fw:
	$(CHECK_PACKAGE)
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_fw application_fpga.bin PACKAGE=$(PACKAGE)

flash:
	$(CHECK_PACKAGE)
	podman run --rm \
	$(tp) \
	--mount type=bind,source="`pwd`/../hw/application_fpga",target=/build \
	-w /build \
	-it $(IMAGE) make prog_flash PACKAGE=$(PACKAGE)

# Export release artifacts
ARTIFACTS_DIR = artifacts
FPGA_DIR = ../hw/application_fpga
CH552_DIR = ../hw/usb_interface/ch552_fw

export-artifacts:
	$(CHECK_PACKAGE)
	rm -rf $(ARTIFACTS_DIR)
	$(MAKE) -C $(FPGA_DIR) clean
	$(MAKE) -C $(FPGA_DIR) PACKAGE=$(PACKAGE) \
		application_fpga_par.json bram_fw.hex firmware.hex
	$(MAKE) -C $(CH552_DIR) clean usb_device.bin
	mkdir -p $(ARTIFACTS_DIR)
	cp $(FPGA_DIR)/application_fpga_par.json \
		$(FPGA_DIR)/bram_fw.hex \
		$(FPGA_DIR)/firmware.hex \
		$(CH552_DIR)/usb_device.bin \
		$(ARTIFACTS_DIR)/
	cp $(FPGA_DIR)/application_fpga.bin.$(PACKAGE).sha256 \
		$(ARTIFACTS_DIR)/application_fpga.bin.sha256
	GOOS=linux GOARCH=amd64 CGO_ENABLED=0 go build \
		-C $(FPGA_DIR)/tools/tkeyimage \
		-o $(abspath $(ARTIFACTS_DIR))/tkeyimage
.PHONY: export-artifacts

pull:
	podman pull $(IMAGE)

build-image:
	podman build -t $(BUILDIMAGE) .

build-image-multiarch:
	-podman manifest rm $(BUILDIMAGE)
	podman build --platform $(PLATFORMS) --manifest $(BUILDIMAGE) .

docker-build-image:
	docker build -t $(BUILDIMAGE) .

docker-build-image-multiarch:
	docker buildx build --platform $(PLATFORMS) -t $(BUILDIMAGE) .
