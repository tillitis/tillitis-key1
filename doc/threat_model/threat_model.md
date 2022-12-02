# Threat model

## Introduction
The Tillitis TKey is a platform for running secure applications in a
restricted execution environment physically separate from the device
host. The secure applications provide functionality and controlled
access to derived secrets on the device. The purpose of the device is
to solve typical end user authentication problems.

This document describes the threat model for device. Based on the
system description and use cases, the threat model tries to capture and
describe the threats that needs to be mitigated in order for the
device to meet its purpose and objectives.


## Version information
The threat model will get updated and expanded for each release.

### engineering-release-1
This is an early release aimed at developers interested
in writing applications for Tillitis TKey. The design allows easy access to
the board, and is even shipped with a programmer to download new FPGA bitstreams.


#### Known weakneses
The bitstream, which includes the Unique Device Secret (UDS) as well as the firmware
implementing the measured boot are stored as part of the bitstream in an external
Flash memory connected to the FPFGA.

The CH552 MCU providing USB host communication contains FW that implements the UART
communication with the FPGA. The firmware can be updated by performing *port knocking*.
The knock sequence is to apply 3.3V through a 10k resistor to the D+ line,
while powering on the device.

There may be possible buffer overflows via the USB host interface to the FW of the CH552,
allowing both execution and modification of the FW CH552.


#### Out of scope
- All physical and electrical attacks applied to the board, including:
  - Reading out of the UDS from the external Flash chip
  - Triggering of the FPGA warm boot functionality
  - Triggering FW update of the CH552 MCU, using the port knocking mechanism

- Glitching attacks including:
  - Faulting of the execution by the CPU in the FPGA and the CH552 MCU
  - Disturbance of the TRNG entropy generation

- EM leakage


#### In scope
(Attacks we really would like to have investigated.)

- Digital attacks from the host against the FW in the FPGA, and the FPGA design itself
  via the host interface.

- Timing attacks on the FW in the FPGA.
