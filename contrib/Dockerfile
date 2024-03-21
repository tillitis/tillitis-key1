FROM docker.io/library/ubuntu:23.10 as base

RUN apt-get -qq update -y \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get install -y --no-install-recommends \
               bison \
               build-essential \
               ca-certificates \
               clang \
               clang-format \
               clang-tidy \
               cmake \
               flex \
               gawk \
               git \
               golang \
               graphviz \
               iverilog \
               less \
               libboost-dev \
               libboost-filesystem-dev \
               libboost-iostreams-dev \
               libboost-program-options-dev \
               libboost-thread-dev \
               libeigen3-dev \
               libffi-dev \
               libftdi-dev \
               libglib2.0-dev \
               libusb-1.0-0-dev \
               libpixman-1-dev \
               libreadline-dev \
               lld \
               llvm \
               mercurial \
               nano \
               ninja-build \
               pkg-config \
               python3 \
               python3-dev \
               tcl-dev \
               verilator \
               vim \
               xdot \
               sdcc \
               splint \               
               cmake \
               gcc-arm-none-eabi \
               libnewlib-arm-none-eabi \
               libstdc++-arm-none-eabi-newlib \
    && rm -rf /var/lib/apt/lists/*


FROM base as toolsbuilder

RUN git clone https://github.com/YosysHQ/icestorm /src
WORKDIR /src
RUN git checkout d20a5e9001f46262bf0cef220f1a6943946e421d \
    && make -j$(nproc --ignore=2) \
    && make install
RUN git >/usr/local/repo-commit-icestorm describe --all --always --long --dirty
WORKDIR /
RUN rm -rf /src

# Custom iceprog for the RPi 2040-based programmer (will be upstreamed).
RUN git clone -b interfaces --depth=1 https://github.com/tillitis/icestorm /src
WORKDIR /src/iceprog
RUN make -j$(nproc --ignore=2) \
    && make PROGRAM_PREFIX=tillitis- install
RUN git >/usr/local/repo-commit-tillitis--icestorm describe --all --always --long --dirty
WORKDIR /
RUN rm -rf /src

RUN git clone -b yosys-0.36 --depth=1 https://github.com/YosysHQ/yosys /src
WORKDIR /src
RUN make -j$(nproc --ignore=2) \
    && make install
RUN git >/usr/local/repo-commit-yosys describe --all --always --long --dirty
WORKDIR /
RUN rm -rf /src

RUN git clone -b nextpnr-0.6 --depth=1 https://github.com/YosysHQ/nextpnr /src
WORKDIR /src
RUN cmake -DARCH=ice40 -DCMAKE_INSTALL_PREFIX=/usr/local . \
    && make -j$(nproc --ignore=2) \
    && make install
RUN git >/usr/local/repo-commit-nextpnr describe --all --always --long --dirty
WORKDIR /
RUN rm -rf /src

RUN git clone --branch 1.5.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git /usr/local/pico-sdk
WORKDIR /usr/local/pico-sdk
RUN git submodule update --init
RUN git >/usr/local/repo-commit-picosdk describe --all --always --long --dirty
WORKDIR /


FROM base
LABEL org.opencontainers.image.description="Toolchain for building TKey FPGA bitstream, firmware, apps, programmer firmware"
COPY --from=toolsbuilder /usr/local/ /usr/local
