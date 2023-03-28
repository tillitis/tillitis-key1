
# image produced by build-image targets
BUILDIMAGE=tkey-builder-local

# default image used when running a container
IMAGE=ghcr.io/tillitis/tkey-builder:1

all:
	@echo "Targets:"
	@echo "run                   Run a shell using image '$(IMAGE)' (Podman)"
	@echo "run-make              Build the FPGA bitstream using image '$(IMAGE)' (Podman)"
	@echo "run-make-no-clean     Like run-make but without cleaning first, useful for iterative firmware dev"
	@echo "run-make-clean_fw     Like run-make but cleans only firmware"
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

run-make-testfw:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean application_fpga_testfw.bin

run-make-no-clean:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make application_fpga.bin

run-make-clean_fw:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it \
	 $(IMAGE) make clean_fw application_fpga.bin

pull:
	podman pull $(IMAGE)

build-image:
	podman build -t $(BUILDIMAGE) .

docker-build-image:
	docker build -t $(BUILDIMAGE) .
