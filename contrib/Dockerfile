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

# Enable bash completion
RUN sed -i '/#if ! shopt -oq posix; then/ s/^#//' /etc/bash.bashrc
RUN sed -i '/#  if \[ -f \/usr\/share\/bash-completion\/bash_completion \]; then/ s/^#//' /etc/bash.bashrc
RUN sed -i '/#    . \/usr\/share\/bash-completion\/bash_completion/ s/^#//' /etc/bash.bashrc
RUN sed -i '/#  elif \[ -f \/etc\/bash_completion \]; then/ s/^#//' /etc/bash.bashrc
RUN sed -i '/#    . \/etc\/bash_completion/ s/^#//' /etc/bash.bashrc
RUN sed -i '/#  fi/ s/^#//' /etc/bash.bashrc
RUN sed -i '/#fi/ s/^#//' /etc/bash.bashrc

FROM base as toolsbuilder

RUN git clone --depth=1 https://github.com/YosysHQ/icestorm /src
WORKDIR /src
RUN git checkout 738af822905fdcf0466e9dd784b9ae4b0b34987f \
    && make -j$(nproc --ignore=2) \
    && make install \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-icestorm
WORKDIR /
RUN rm -rf /src

# Custom iceprog for the RPi 2040-based programmer (will be upstreamed).
RUN git clone -b interfaces --depth=1 https://github.com/tillitis/icestorm /src
WORKDIR /src/iceprog
RUN make -j$(nproc --ignore=2) \
    && make PROGRAM_PREFIX=tillitis- install \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-tillitis--icestorm
WORKDIR /
RUN rm -rf /src

RUN git clone -b 0.45 --depth=1 https://github.com/YosysHQ/yosys /src
WORKDIR /src
RUN git submodule update --init \
    && make -j$(nproc --ignore=2) \
    && make install \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-yosys
WORKDIR /
RUN rm -rf /src

RUN git clone -b nextpnr-0.7 https://github.com/YosysHQ/nextpnr /src
WORKDIR /src
# Add "Fix handling of RNG seed" #1369
RUN git cherry-pick --no-commit 6ca64526bb18ace8690872b09ca1251567c116de
# Add early exit if place fails on timing
RUN sed -i \
    '345i \ \ \ \ general.add_options()("exit-on-failed-target-frequency",' \
    common/kernel/command.cc
RUN sed -i \
    '346i \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ "exit if target frequency is not achieved (use together with "' \
    common/kernel/command.cc
RUN sed -i \
    '347i \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ "--randomize-seed option)");' \
    common/kernel/command.cc
RUN sed -i \
    '348i \\' \
    common/kernel/command.cc
RUN sed -i \
    '662s/if (do_route) {/if (do_route \&\& (vm.count("exit-on-failed-target-frequency") ? !had_nonfatal_error : true)) {/' \
    common/kernel/command.cc
RUN sed -i \
    '244s/bool warn_on_failure = false/bool warn_on_failure = true/' \
    common/kernel/timing.h
RUN cmake -DARCH=ice40 . \
    && make -j$(nproc --ignore=2) \
    && make install \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-nextpnr
WORKDIR /
RUN rm -rf /src

RUN git clone -b v12_0 --depth=1 https://github.com/steveicarus/iverilog /src
WORKDIR /src
RUN sh autoconf.sh \
    && ./configure \
    && make -j$(nproc --ignore=2) \
    && make install \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-iverilog
WORKDIR /
RUN rm -rf /src

RUN git clone -b v5.028 --depth=1 https://github.com/verilator/verilator /src
WORKDIR /src
RUN autoconf \
    && ./configure \
    && make -j$(nproc --ignore=2) \
    && make test \
    && make install \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-verilator
WORKDIR /
RUN rm -rf /src

ADD https://github.com/chipsalliance/verible/releases/download/v0.0-3795-gf4d72375/verible-v0.0-3795-gf4d72375-linux-static-x86_64.tar.gz /src/verible.tar.gz
WORKDIR /src
RUN tar xvf verible.tar.gz
RUN mv -v verible*/bin/* /usr/local/bin
RUN verible-verilog-format --version | head -1 > /usr/local/repo-commit-verible
WORKDIR /
RUN rm -rf /src

RUN git clone -b v1.9.1 https://github.com/cocotb/cocotb.git /src
WORKDIR /src
RUN pip install . --break-system-packages \
    && git describe --all --always --long --dirty > /usr/local/repo-commit-cocotb
WORKDIR /
RUN rm -rf /src

FROM base
LABEL org.opencontainers.image.description="Toolchain for building TKey FPGA bitstream"
COPY --from=toolsbuilder /usr/local/ /usr/local
