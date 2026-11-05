FROM debian:trixie-slim AS base

RUN export DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        autoconf \
        bash-completion \
        bison \
        build-essential \
        ca-certificates \
        clang-18 \
        clang-format-18 \
        clang-tidy-18 \
        cmake \
        curl \
        flex \
        g++ \
        gawk \
        gcc \
        git \
        golang \
        gperf \
        graphviz \
        help2man \
        less \
        libboost-dev \
        libboost-filesystem-dev \
        libboost-iostreams-dev \
        libboost-program-options-dev \
        libboost-python-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libeigen3-dev \
        libffi-dev \
        libfl-dev \
        libftdi-dev \
        libpython3-dev \
        libreadline-dev \
        libusb-1.0-0-dev \
        lld-18 \
        llvm-18 \
        make \
        nano \
        ncdu \
        perl \
        pkg-config \
        psmisc \
        python-is-python3 \
        python3 \
        python3-dev \
        python3-full \
        python3-pip \
        python3-pytest \
        sdcc \
        splint \
        tcl-dev \
        vim \
        xdot \
        zlib1g-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

FROM base AS tools-builder

COPY buildtools.sh /buildtools.sh
COPY verible.sha512 /verible.sha512
COPY yosys-cellaigs-eval-order.patch /yosys-cellaigs-eval-order.patch

RUN /buildtools.sh

FROM base AS runtime

COPY --from=tools-builder /usr/local/ /usr/local/

ENV PATH="/usr/lib/llvm-18/bin:$PATH"

LABEL org.opencontainers.image.description="Toolchain for building TKey FPGA bitstream"
