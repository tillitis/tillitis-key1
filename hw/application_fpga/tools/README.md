# Tools

We have developed some tools necessary for the build.

- `app_bin_to_spram_hex.py`: Script used to include a device app in a
  testbench simulation.

- `b2s`: Compute and print a BLAKE2s digest over a file. Used for the
  digest of the app in app slot 0 included in the firmware.

- `default_partition.bin`: Default partition table for the flash.

- `load_preloaded_app.sh`: Script to load two copies of the partition
  table to flash and a pre-loaded to app slot 0 or 1. Needs
  `default_partition.bin`, generated with `tkeyimage` and the binary
  of the device app to load. Call like: `./load_preloaded_app 0
  path/to/binary`.

- `makehex.py`: Used to build hex version of firmware ROM for FPGA
  bitstream build.

- `patch_uds_udi.py`: Script used to patch in the Unique Device Secret
  in `data/uds.hex` and the Unique Device Identifier in `data/udi.hex`
  into the bitstream without having to rebuild the entire bitstream.

- `run_pnr.sh`: Script to run place and route with `nextpnr` in order
  to find a routing seed that will meet desired timing.

- `tkeyimage`: Utility to create and parse flash images with a TKey
  filesystem or the partition table

- `tpt/tpt.py`: Utility to create the Unique Device Secret (UDS) and
  Unique Device Identity (UDI) interactively.
