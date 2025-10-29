#!/bin/bash -e

# SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
# SPDX-License-Identifier: BSD-2-Clause

if [ $# != 2 ]
then
    echo "Usage: $0 slot_num app_file"
    echo ""
    echo "Where slot_num is 0 or 1."
    exit
fi

SLOT_NUM="$1"
APP="$2"

if [ "$SLOT_NUM" = "0" ]; then
    START_ADDRESS=0x30000
elif [ "$SLOT_NUM" = "1" ]; then
    START_ADDRESS=0x50000
else
    echo "Invalid slot_num"
    exit 1
fi

echo "WARNING: Will install default partition table."
read -p "Press CTRL-C to abort. Press key to continue." -n1 -s

# Write both copies of the partition table
tillitis-iceprog -o 128k default_partition.bin
tillitis-iceprog -o 0xf0000 default_partition.bin

# Erase existing pre loaded app
tillitis-iceprog -o "$START_ADDRESS" -e 128k

# Write pre loaded app
tillitis-iceprog -o "$START_ADDRESS" "$APP"
