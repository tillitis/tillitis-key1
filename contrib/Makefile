all:
	@echo "Build targets: "
	@echo "build			Build an image for building tools with Docker"
	@echo "run			Run a shell using the above image with Docker"
	@echo "podman-build       	Build an image with podman"
	@echo "podman-run		Run a shell using above image with podman"
	@echo "podman-run-make		Run the above image with podman and build the FPGA bitstream"

build:
	docker build -t tkey-builder .

run:
	docker run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it tkey-builder /usr/bin/bash

podman-build:
	podman build -t tkey-builder .

podman-run:
	podman run --rm --mount type=bind,source="`pwd`/../",target=/build -w /build -it tkey-builder /usr/bin/bash

podman-run-make:
	podman run --rm --mount type=bind,source="`pwd`/../hw/application_fpga",target=/build -w /build -it tkey-builder make
