FROM docker.io/library/ubuntu:24.04 as base

RUN apt-get -qq update -y \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get install -y --no-install-recommends \
               autoconf \
               bash-completion \
               bison \
               build-essential \
               ca-certificates \
               clang \
               clang-format \
               clang-tidy \
               cmake \
               curl \
               flex \
               g++ \
               gawk \
               gcc \
               git \
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
               libusb-1.0-0-dev \
               libreadline-dev \
               lld \
               llvm \
               make \
               nano \
               ncdu \
               perl \
               pkg-config \
               psmisc \
               python-is-python3 \
               python3 \
               python3-full \
               python3-dev \
               python3-pip \
               python3-pytest \
               sdcc \
               splint \
               tcl-dev \
               vim \
               xdot \
               zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

FROM base as toolsbuilder

COPY buildtools.sh /buildtools.sh
COPY verible.sha512 /verible.sha512

RUN /buildtools.sh

FROM base
LABEL org.opencontainers.image.description="Toolchain for building TKey FPGA bitstream"
COPY --from=toolsbuilder /usr/local/ /usr/local
