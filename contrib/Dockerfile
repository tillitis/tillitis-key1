FROM docker.io/library/ubuntu:22.10 as base

RUN apt-get -qq update -y \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get install -y --no-install-recommends \
               bison \
               build-essential \
               ca-certificates \
               clang \
               clang-format \
               cmake \
               flex \
               gawk \
               git \
               golang \
               graphviz \
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
               libhidapi-dev \
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
    && rm -rf /var/lib/apt/lists/*


FROM base as toolsbuilder

RUN git clone --depth=1 https://github.com/YosysHQ/icestorm /src
WORKDIR /src
RUN make -j$(nproc --ignore=2) \
    && make install
RUN git >/usr/local/repo-commit-icestorm describe --tags --always --dirty
WORKDIR /
RUN rm -rf /src

# Custom iceprog for the RPi 2040-based programmer (will be upstreamed).
RUN git clone -b interfaces --depth=1 https://github.com/tillitis/icestorm /src
WORKDIR /src/iceprog
RUN make -j$(nproc --ignore=2) \
    && make PROGRAM_PREFIX=tillitis- install
RUN git >/usr/local/repo-commit-tillitis--icestorm describe --tags --always --dirty
WORKDIR /
RUN rm -rf /src

RUN git clone https://github.com/YosysHQ/yosys /src
WORKDIR /src
# Avoiding current issue with yosys & icebram, filed in:
# https://github.com/YosysHQ/yosys/issues/3478
RUN git checkout 06ef3f264afaa3eaeab45cc0404d8006c15f02b1 \
    && make -j$(nproc --ignore=2) \
    && make install
RUN git >/usr/local/repo-commit-yosys describe --tags --always --dirty
WORKDIR /
RUN rm -rf /src

RUN git clone https://github.com/YosysHQ/nextpnr /src
WORKDIR /src
# Use nextpnr-0.4. Aa few commits later we got issues, like on f4e6bbd383f6c43.
RUN git checkout nextpnr-0.4 \
    && cmake -DARCH=ice40 -DCMAKE_INSTALL_PREFIX=/usr/local . \
    && make -j$(nproc --ignore=2) \
    && make install
RUN git >/usr/local/repo-commit-nextpnr describe --tags --always --dirty
WORKDIR /
RUN rm -rf /src


FROM base
LABEL org.opencontainers.image.description="Toolchain for building TKey FPGA bitstream, firmware, apps"
COPY --from=toolsbuilder /usr/local/ /usr/local
