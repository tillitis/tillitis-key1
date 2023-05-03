# Threat model

## Introduction

The Tillitis TKey device is a platform for running device apps in a
secure, restricted execution environment physically separate from the
client. The device provides the device app access to secrets derived
through measurement of the loaded device app. The device app in turn
provides functionality and controlled access to assets to a companion
client app as needed to solve different use cases.

This document describes the threat model for the Tillitis TKey device
and the device app. Based on [the system
description](system_description.md) and use cases, the threat model
tries to capture and describe the threats that needs to be mitigated
in order for the device app to work in a secure and trustworthy
manner.


## Assumptions

* There are no backdoors or vulnerabilities in Lattice iCE40 UltraPlus
  FPGA devices that allow external access to the internal
  configuration memory (Non-Volatile Configuration Memory, NVCM) after
  the device configuration has been written to the NVCM and external
  access has been locked down through the fuses.

* There is no access path to the contents of the NVCM from the FPGA
  fabric besides the configuration circuit.

* There exist a possible warm boot attack against the Lattice iCE40
  UltraPlus FPGAs, which allows an attacker with physical access to
  load a FPGA configuration even though the NVCM has been programmed
  and locked.

* The FPGA development toolchain, including YoSys, NextPnR and
  IceStorm generates a correct design, and also does not inject
  hardware exfiltration mechanisms in the generated bitstream.

* The end user is not an attacker. The end user at least doesn't
  knowingly aid the attacker in attacks on their device.

## Assets

* UDS - Unique Device Secret. Provisioned and stored in the FPGA NVCM
  during TKey device provisioning. Never to be replaced or altered
  during the life time of a given TKey device. Used to derive
  application secrets. Must never leave the device. Tillitis must NOT
  not store a copy of the UDS.

* UDI - Unique Device ID. Provisioned and stored in the FPGA NVCM
  during TKey device provisioning. Never to be replaced or altered
  during the life time of a given device. May be copied, extracted,
  read from the device.

* USS - User Supplied Secret. Provisioned by the user from the client
  during loading of the device app. Should not be revealed to a third
  party.

* CDI - Compound Device Identity. Computed by firmware when an
  application is loaded using the UDS, the USS, and a hash of the
  device app binary. Used by the application to derive secrets, keys
  etc. as needed. The CDI should never be exposed outside of the FPGA.

## Threats and threat vectors

There are two major type of attacks

1. Software (SW) based. These are attacks against the TKey device that
   are performed from a client and enter the TKey device through the
   USB port. The SW attacks includes buffer flow attacks, attacks on
   the firmware protocol.

2. Hardware (HW) based. These are attacks against the FPGA design of
   the TKey device as well as the PCB. The HW attacks includes fault
   injection, side-channel leakage as well as warm boot attacks. These
   attacks may be performed from the client through the USB port,
   through the TKey enclosure, or near the TKey device.


## Threat Actors - The bad guys

Different actors have different reasons for performing attacks. They
have also different access to competence, resources etc. This
description tries to capture examples of possible attacks and how the
TKey device should be able to stand up against them.


### 0. Average Joe
[Average Joe Soundtrack](https://www.youtube.com/watch?v=BB0DU4DoPP4)

* Curious opportunist
* No real competence, no resources beyond a personal computer
* No planning or preparation before an attack
* Prepared to invest little time (minutes) or resources - for example
  to connect a device found, try a few user supplied secrets
* End game is to gain access to possible information, client resources
  unknown to the attacker before the attack is performed

Attacks by Average Joe will come from the USB port and is SW based, or
just manual attempts. Given a hard to guess USS, the TKey Device
should withstand any attack no matter how long time the attack is
allowed

### 1. The CCC Hacker
[CCC Hacker Soundtrack](https://www.youtube.com/watch?v=l8DBEbmPh7E)

* Sympathetic to the goals of the project
* Wants to probe all parts and the system in a quest to determine how
  the device really works, use it in possibly different ways, find
  weaknesses (and get them fixed)
* Is possibly a user, but in this case not the legitimate end user
* Have a high level of competence
* Prepared to spend time to prepare and perform an attack. Possibly low
  effort over an extended period
* Access to compute resources. Possibly access to lab equipment
* Will try all possible SW and HW attack vectors. In and out of scope
* End game is to find flaws in threat model. Acquire knowledge and
  findings to produce an interesting talk at CCC, USENIX or Security
  Fest

Over time (with new releases), and given feedback by the CCC Hacker,
the TKey device should be able to withstand attacks by the CCC Hacker.

### 2. vERyRevil
[vERyRevil Soundtrack](https://www.youtube.com/watch?v=sTSA_sWGM44)

* Ransomware gang. Driven by short term financial gain
* Short term focus. Fastest possible access to economic assets
* Have, or can acquire high level of competence
* Have access to large amount of resources
* Have time and is prepared to spend time on preparations
* Short time to perform an attack. Will not persist for a long time
* Will do strict cost benefit-analysis to decide to perform, abort
  attacks if they don't work
* SW based attacks. Is assumed to remotely own the host
* Supply chain attacks on secure application, host application, SDK,
  infiltration of device and application development
* End game is to gain access, control over resources protected by the
  device. Resources that can be used as leverage for financial gain

Over time (with new releases), The TKey device should be able to
withstand SW attacks by vERyRevil.

### 4. APT4711
[APT4711 Soundtrack](https://www.youtube.com/watch?v=lrWV6pxepDo)

* State actor
* Interested in access to information, perform surveillance, and
  possibly control of the end user or resources
* Long term focus. Attacks are discreet and persistent
* Access to high competence
* Access to very large amounts of resources
* Prepared to invest a lot of time, effort to prepare and execute an
  attack
* Prepared to perform physical visits (evil maid missions) at target
  (end user) as well as Tillitis or the suppliers to Tillitis as
  needed to manipulate, steal, replace components, systems
* SW based attacks. Is assumed to remotely own the host
* Supply chain attacks - both on SW and HW, components
* Supply chain attacks on application, host application, SDK,
  development
* End game: Long term stealth presence providing access to information
  about the end user

Over time (with new releases), The TKey device should be able to
withstand SW based attacks. Over time, the TKey Device should be able
to make evil maid attacks take long enough time to make in infeasible to
perform without the user discovering the missing device.

## TKey Release specific scope

This threat model will be updated for each release of the TKey device.
For each version we describe what threats are in scope, what threats
are out of scope and what mitigations are in place.

### TK1-23.03.1-Bellatrix

This is the first general release of the TKey TK1 end user device. In this
device the FPGA bitstream is stored and locked into the NVCM. This means
that the bitstream can't be changed or read out from the device.

The UDS and UDI assets are generated during provisioning by Tillitis, and
are stored as part of the FPGA bitstream. The UDS is generated using
the tpt tool and is not stored by Tillitis after generation.

The FPGA design contain some mechanisms for execution protection,
execution monitoring as well as functionality designed to make warm
boot based evil maid attacks harder to successfully perform, i.e. take
longer time. Moreover the transparent TKey casing is glued together
which makes it harder to open up without leaving physical marks
indicating tamper attempts.

The FPGA design as well as the firmware has been audited, and
hardening of these has been performed to some degree. For more
information, see the [Release Notes](/doc/release_notes.md)

#### Known possible weakneses

The CH552 MCU providing USB host communication contains firmware that
implements the UART communication with the FPGA. The CH552 firmware
can be updated by performing *port knocking*. The knock sequence is to
apply 3.3V through a 10k resistor to the D+ line, while powering on
the device.

There may be possible buffer overflow attacks via the USB host
interface into the firmware of the CH552, allowing both execution and
modification of the firmware CH552.

#### In scope

- SW attacks from the host against the firmware in the FPGA as well as
  the FPGA design itself via the USB host interface.

- Timing attacks on the firmware and the FPGA design.

#### Out of scope

- Leakage and glitching attacks including:
  - Faulting of the execution by the CPU in the FPGA and the CH552 MCU
  - EM leakage

- Warm boot attacks. It should be hard to successfully perform against
  the TKey, but the attack is not yet fully mitigated.

- Attacks on the TKey device apps.


### engineering-release-1

This is an early release aimed at developers interested in writing
applications for Tillitis TKey. The design allows easy access to the
board, and is even shipped with a programmer to download new FPGA
bitstreams.


#### Known weakneses

The bitstream, which includes the Unique Device Secret (UDS) as well
as the firmware implementing the measured boot are stored as part of
the bitstream in an external Flash memory connected with SPI to the
FPGA.

The CH552 MCU providing USB host communication contains firmware that
implements the UART communication with the FPGA. The firmware can be
updated by performing *port knocking*. The knock sequence is to apply
3.3V through a 10k resistor to the D+ line, while powering on the
device.

There may be possible buffer overflows via the USB host interface to
the firmware of the CH552, allowing both execution and modification of
the firmware CH552.

#### In scope

(Attacks we really would like to have investigated.)

- Digital attacks from the host against the firmware in the FPGA, and
  the FPGA design itself via the host interface.

- Timing attacks on the firmware in the FPGA.

#### Out of scope

- All physical and electrical attacks applied to the board, including:
  - Reading out of the UDS from the external Flash chip
  - Triggering of the FPGA warm boot functionality
  - Triggering firmware update of the CH552 MCU, using the port
    knocking mechanism

- Glitching attacks including:
  - Faulting of the execution by the CPU in the FPGA and the CH552 MCU
  - Disturbance of the TRNG entropy generation

- EM leakage
