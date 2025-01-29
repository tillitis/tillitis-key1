#! /bin/sh -e

# Copyright (C) 2025 Tillitis AB
# SPDX-License-Identifier: GPL-2.0-only

## Build the specific versions of the tools we need to build the TKey
## FPGA bitstream and apps.

cd /
mkdir src

# ----------------------------------------------------------------------
# Project icestorm
# ----------------------------------------------------------------------
git clone https://github.com/YosysHQ/icestorm /src/icestorm
cd /src/icestorm

# No tags or releases yet. Pin down to a specific commit.
git checkout 738af822905fdcf0466e9dd784b9ae4b0b34987f
make -j$(nproc --ignore=2)
make install
git describe --all --always --long --dirty > /usr/local/repo-commit-icestorm

# ----------------------------------------------------------------------
# Our own custom iceprog for the RPi 2040-based programmer. Will be
# upstreamed.
# ----------------------------------------------------------------------
git clone -b interfaces --depth=1 https://github.com/tillitis/icestorm /src/icestorm-tillitis
cd /src/icestorm-tillitis/iceprog
make -j$(nproc --ignore=2)
make PROGRAM_PREFIX=tillitis- install
git describe --all --always --long --dirty > /usr/local/repo-commit-tillitis--icestorm

# ----------------------------------------------------------------------
# yosys
# ----------------------------------------------------------------------
git clone -b 0.45 --depth=1 https://github.com/YosysHQ/yosys /src/yosys
cd /src/yosys

# Make sure the digest is correct and no history has changed
git checkout  9ed031ddd588442f22be13ce608547a5809b62f0

git submodule update --init
make -j$(nproc --ignore=2)
make install
git describe --all --always --long --dirty > /usr/local/repo-commit-yosys

# ----------------------------------------------------------------------
# nextpnr
# ----------------------------------------------------------------------
git clone -b nextpnr-0.7 https://github.com/YosysHQ/nextpnr /src/nextpnr
cd /src/nextpnr
# Make sure the digest is correct and no history has changed
git checkout 73b7de74a5769095acb96eb6c6333ffe161452f2

# Add "Fix handling of RNG seed" #1369
git cherry-pick --no-commit 6ca64526bb18ace8690872b09ca1251567c116de

# Add early exit if place fails on timing
sed -i \
    '345i \ \ \ \ general.add_options()("exit-on-failed-target-frequency",' \
    common/kernel/command.cc
sed -i \
    '346i \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ "exit if target frequency is not achieved (use together with "' \
    common/kernel/command.cc
sed -i \
    '347i \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ "--randomize-seed option)");' \
    common/kernel/command.cc
sed -i \
    '348i \\' \
    common/kernel/command.cc
sed -i \
    '662s/if (do_route) {/if (do_route \&\& (vm.count("exit-on-failed-target-frequency") ? !had_nonfatal_error : true)) {/' \
    common/kernel/command.cc
sed -i \
    '244s/bool warn_on_failure = false/bool warn_on_failure = true/' \
    common/kernel/timing.h

cmake -DARCH=ice40 .
make -j$(nproc --ignore=2)
make install
git describe --all --always --long --dirty > /usr/local/repo-commit-nextpnr

# ----------------------------------------------------------------------
# icarus verilog
# ----------------------------------------------------------------------
git clone -b v12_0 --depth=1 https://github.com/steveicarus/iverilog /src/iverilog
cd /src/iverilog

# Make sure the digest is correct and no history has changed
git checkout 4fd5291632232fbe1ba49b2c26bb6b2bf1c6c9cf
sh autoconf.sh
./configure
make -j$(nproc --ignore=2)
make install
git describe --all --always --long --dirty > /usr/local/repo-commit-iverilog

# ----------------------------------------------------------------------
# verilator
# ----------------------------------------------------------------------
git clone -b v5.028 --depth=1 https://github.com/verilator/verilator /src/verilator
cd /src/verilator

# Make sure the digest is correct and no history has changed
git checkout 8ca45df9c75c611989ae5bfc4112a32212c3dacf

autoconf
./configure
make -j$(nproc --ignore=2)
make test
make install
git describe --all --always --long --dirty > /usr/local/repo-commit-verilator

# ----------------------------------------------------------------------
# verible
# ----------------------------------------------------------------------
curl --output /src/verible.tar.gz  -L https://github.com/chipsalliance/verible/releases/download/v0.0-3795-gf4d72375/verible-v0.0-3795-gf4d72375-linux-static-x86_64.tar.gz

# Check against the expected digest
sha512sum -c /verible.sha512

cd /src

tar xvf verible.tar.gz
mv -v verible*/bin/* /usr/local/bin
verible-verilog-format --version | head -1 > /usr/local/repo-commit-verible

# ----------------------------------------------------------------------
# cocotb
# ----------------------------------------------------------------------
git clone -b v1.9.1 https://github.com/cocotb/cocotb.git /src/cocotb
cd /src/cocotb
pip install . --break-system-packages
git describe --all --always --long --dirty > /usr/local/repo-commit-cocotb
