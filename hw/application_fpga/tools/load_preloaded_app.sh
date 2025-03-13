#!/bin/bash -e

if [ $# != 1 ]
then
    echo "Usage: $0 app_file"
    exit
fi

APP="$1"

echo "WARNING: Will erase entire partition table."
read -p "Press CTRL-C to abort. Press key to continue." -n1 -s

# Erase partition table
tillitis-iceprog -o 0x20000 -e 64k

# Erase existing pre loaded app
tillitis-iceprog -o 0x30000 -e 128k

# Write pre loaded app
tillitis-iceprog -o 0x30000 "$APP"
