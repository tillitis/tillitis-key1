# Tillitis TKey threat model

## Introduction

The Tillitis TKey is a platform for running device apps in a
restricted execution environment physically separate from the client.
The device provides the device app access to secrets derived through
measurement of a device app or, in a chain of apps, derived keys from
the chain. The device app in turn provides functionality and
controlled access to assets to a companion client app as needed to
solve different use cases.

For more about the TKey, see [TKey Developer
Handbook](https://dev.tillitis.se/), [the firmware
documentation](../../hw/application_fpga/fw/README.md) or [the top
level hardware documentation](../../hw/application_fpga/README.md).

The threat model tries to describe the threats that needs to be
mitigated in order for the device app to work in a secure and
trustworthy manner.

### TKey Unlocked

The TKey comes in two versions: the ordinary TKey for general use and
the TKey Unlocked. The Unlocked is for people who want to provision
the bitstream themselves, either because they want to choose the
Unique Device Secret themselves or want to change the hardware design
or the firmware.

This threat model and the mitigations listed applies to unlocked
devices too as long as they have been provisioned with:

- the bitstream from the official release,
- a unique, random UDS,
- a unique UDI

The configuration must have been written into the NVCM and locked by
blowing the fuses.

XXX: Might need to remove above if we rely on new casing as
mitigations in the threat model.

## Assumptions

- The security boundary is the FPGA itself. The client, the CH552 USB
  controller, and the flash chip, are outside of the security
  boundary, even if we make our best efforts to secure them they are
  basically outside of our control.

- There are no backdoors or vulnerabilities in Lattice iCE40 UltraPlus
  FPGA devices that allow external access to the internal
  configuration memory (Non-Volatile Configuration Memory, NVCM) after
  the device configuration has been written to the NVCM and external
  access has been locked down through the fuses.

- There is no access path to the contents of the NVCM from the FPGA
  fabric besides the configuration circuit.

- The Lattice iCE40 UltraPlus FPGAs are vulnerable to a warm boot
  attack which allows an attacker with physical access to load a FPGA
  configuration even though the NVCM has been programmed and locked.

- The FPGA development toolchain, including YoSys, NextPnR and
  IceStorm generates a correct design, and also does not inject
  hardware exfiltration mechanisms in the generated bitstream.

- The end user is not an attacker. The end user at least doesn't
  knowingly aid the attacker in attacks on their device.

## Assets

- UDS - Unique Device Secret. 

  Provisioned and stored in the FPGA NVCM during TKey device
  provisioning. Never to be replaced or altered during the life time
  of a given TKey device. Used to derive application secrets. Must
  never leave the device. Tillitis must not store a copy of the
  UDS.

- UDI - Unique Device ID.

  Provisioned and stored in the FPGA NVCM during TKey device
  provisioning. Never to be replaced or altered during the life time
  of a given device. Not secret, although the second half (serial
  number) is more sensitive, since it uniquely identifies a specific
  device.

- USS - User Supplied Secret. 

  Provisioned by the user from the client during loading of the device
  app. Should not be revealed to a third party.

- CDI - Compound Device Identity. 

  Computed by firmware. Used by the device app to derive secrets, keys
  et c. as needed. The CDI should never be exposed outside of the
  FPGA.

- Vendor public key

  Used in a verified boot scenario to verify the next app to start.

  The vendor public key can either be stored on the flash and be
  associated with the device app in slot 1 or be fed to a boot
  verifier app from the client.

- Filesystem partition table

- Firmware memory

  The firmware has its own memory, `FW_RAM`. It does all its sensitive
  computations here.

- RAM

  The TKey has a memory where the device app is loaded and executed.
  It's available for the apps.

- Firmware mode

  The TKey has two execution modes: firmware mode and app mode.
  Firmware mode has more priveleges. When running in firmware mode the
  entire memory map (including some of the other assets listed here)
  are available for read and write access.
  
- Preloaded apps

  The TKey comes with two preloaded apps stored in slot 0 and slot 1,
  the boot verifier and the FIDO2 app. By default they are started in
  a chain.

## Threats and mitigations

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

### General physical attacks

Threat: Opening the TKey to access the chips and the programming pins.

Mitigation:

- A transparent TKey casing glued together which makes it harder to
  open up without leaving physical marks indicating tamper attempts. A
  weak protection, but might help against giving it back modified.

### General memory map attacks

Threats:

- Illegal execution.
- Illegal access.

Mitigation: 

- Depending on the hardware mode (firmware mode/app mode), different
  areas of the memory map are available. Unmapped or unavailable
  memory forces the CPU into a trap state that requires a reboot. This
  state is indicated by the hardware by slowly flashing the status LED
  red. No software execution takes place.

### Firmware fuzzing

Threat: Finding exploits in the firmware.

Mitigation:

- Firmware has a strict protocol state machine. Illegal messages in a
  specific state leads to a CPU trap state that requires a reboot.

### UDS

Threat: Leaking.

Mitigations:

- The UDS is part of the bitstream, locked down within the NVCM in the
  FPGA chip. The data in NVCM shouldn't be visible on X-ray and isn't
  visible with microscopes.

- The hardware design allows only one read of the entire UDS once per
  power-cycle, meant to be done by the firmware.

- The firmware reads out UDS exactly once and keeps it in the special
  firmware RAM for a very short period while doing the CDI
  computation, almost immediately discarding it after just a few
  instructions. In order not to keep the UDS in `FW_RAM` on
  predictable cycles, it randomizes when the UDS handling takes place
  using the TRNG.

- UDS is generated and provisioned in an airgapped environment. It's
  kept only until the bitstream for that particular device is
  provisioned and then deleted.
  
### UDI

Threats:

- Changing after provisioning.
- Behave maliciously for specific devices.

Mitigations:

- The UDI is part of the bitstream, locked down within the NVCM in the
  FPGA chip. The NVCM is write-once and then locked down. According to
  our assumptions it's no longer changeable.

- The second part of the UDI, the serial number, is not available to
  device apps, only the vendor ID and product ID (see `sys_get_vidpid`
  syscall). A malicious device app can't access the serial number and
  can therefore not behave maliciously for a specific device and not
  others.

### USS and USS digest

Threat: Leaking.

Mitigations:

- On the client side, our apps use the pinentry protocol to get the
  USS. They usually depend on gnome3-pinentry on Linux and Gpg4Win on
  Windows, but this isn't enforced. The protection is weak.
  Keyloggers, especially on X11, would probably be leaking the USS.
  
- The firmware uses the USS in the CDI computation. The entire memory
  the USS is kept in, `FW_RAM`, is then wiped and made unavailable to
  the device app by hardware protection.

  Since the firmware does the computation, the device app doesn't even
  see the USS digest.

- On the device side, the USS is only handled as a digest, not the
  passphrase the user entered, but this digest is also very sensitive,
  as it's the actual argument used in the CDI computation, but this
  means the firmware can't leak any passphrase entered by the user.

### CDI

Threats:

- Leaking.
- Changing by device app.

Mitigations:

- Leaking: A malicious app designed to leak the CDI, the base of its
  secrets, will in the measured boot case, leak the /wrong/ secret
  since the CDI is a measurement of the app itself. If the app running
  is changed to be malicious, the CDI becomes different. It is
  computed like this:
  
  `CDI = blake2s(UDS, blake2s(app)[, USS])`

- Leaking: A malicious app designed to leak in the verified boot case
  will not be allowed to execute by the security policy of the boot
  verifier.

  Typically the boot verifier will check the app vendor's signature
  over an app digest against a vendor public key. It will only
  continue if the signature verifies. It will then leave the app
  digest to firmware which will load the new app. If the new app's
  digest is not the same as the already verified app digest execution
  will halt.

  If the boot verifier app itself is malicious and doesn't have a good
  security policy the new CDI will depend on the CDI of the verifier
  app itself and will thus be the /wrong/ CDI, which protects the real
  CDI from leaking. In the verified boot case the new CDI is computed
  by firmware like this:
  
  `cdi = blake2s(UDS, blake2s(CDI of previous app, seed)[, USS])`

  The seed is whatever data the previous app might want to include in
  the measurement, for instance the digest of its security policy.

- Changing: CDI is protected by hardware. It is only writable in
  firmware mode, not app mode. This protects against an app changing
  its own CDI to something not measured by the firmware.

### Vendor public key

Threat: Used to run or install a verified, but malicious device app.

Mitigation:

### Filesystem partition table

Threats: Corruption or malicious change.

Mitigations:

- Corruption: The partition table is held in two copies on flash. It
  also contains a checksum. Firmware checks the checksum at every read
  and computes it on every write. If it ever differs, the filesystem
  is marked as suspect and the copy is used.

- Malicious change: Only firmware is allowed to change the partition
  table. Barring bugs, a malicious change will require physical
  access, breaking of the case, and accessing the flash directly,
  which is currently out of scope.

### Firmware memory (`FW_RAM`)

Threats: 

- Leaking sensitive data. 
- Execution.

Mitigation:

- The entire `FW_RAM` is cleared when the FPGA is powered up and the
  FPGA is configured.
  
  XXX but it is not cleared by hardware after each reset?

  TODO: Perhaps the firmware should clear the `FW_RAM` before doing a
  reset.

- The firmware's stack in `FW_RAM` is cleared by firmware before
  jumping to an app.

- Firmware memory is hardware protected by the current execution mode.
  It is only available for reading and writing in firmware mode.

- It is at all times protected by hardware against execution.

### RAM

Threats: 

- Leaking sensitive data.
- Execution of writable areas. 

Mitigation:

- Execution: A device app can mark one continous area as
  non-executable, typically the areas after `.text`, up to and
  including the stack.

- Writable: No protection.

- Leaking: Firmware fills entire RAM with random data at startup to
  ensure that any rests of data that might have been present are
  wiped.

- Leaking: Firmware turns on a hardware assisted RAM address and data
  scrambling mechanism. This makes it harder for an outside attacker
  to find assets generated by and stored in the RAM by applications.
  Note that this mitigates an attack from outside the CPU, not from an
  exploit towards applications running on it.

  The address and data scrambling is initiated with different
  randomness from the TRNG at each start of the firmware.

### Firmware mode

Threat: Unlawful privelege escalation.

Mitigation:

- Hardware execution modes: Firmware mode (all access, except UDS
  after first read). App mode: Sensitive stuff not readable or
  writable (see specific assets).

- Hardware protection at app start. Execution begins in firmware mode,
  but when starting the app it automatically changes the hardware
  status to app mode, locking off sensitive areas of the memory map
  from reading and/or writing. The entire ROM, where firmware resides,
  is marked non-executable in app mode. It must still be readable,
  though, since tkey-verify checks it.

- Hardware protection for system call privilege escalation. In order
  to access the filesystem and other sensitive stuff the app needs to
  escalate privilege temporarily to firmware mode. This is done with
  hardware support from an interrupt call.
  
  When this interrupt is raised, the device temporarily enters
  firmware mode and the system call handler in the firmware. Arguments
  are passed in registers. The firmware does the system call and then
  automatically goes back to the hardware app mode when returning to
  the app.

### Preloaded apps

Threat: Changed to malicious apps.

Mitigation:

- App slot 0, containing the boot verifier, can not be changed after
  provisioning. It's app digest is mentioned in the firmware in ROM.
  Firmware will only start it after a power cycle if the computed
  digested is the same.

- App slot 1: No protection against changing app. Instead, we rely on
  the app getting it's own CDI with the measured boot/verified boot
  combination so a malicious app cannot leak secrets.

## Known weaknesses

- All RAM is writable in app mode, even addresses marked executable.

- A warm boot attack against the Lattice iCE40 UltraPlus, changing the
  FPGA configuration while the chip is powered on will leave the EBR
  and SPRAM intact after the new configuration is loaded. This means
  we might leak contents of EBR and SPRAM.

  Mitigations:
  
  - Requires physical access.

  - Requires breaking the case.

  - SPRAM is protected by a scrambling mechanism. See above under RAM.

  - EBR is not protected, so any hardware cores that use EBR is still
    vulnerable.

    XXX: List which ones use EBR?

- The CH552 MCU providing USB communication contains firmware that
  implements the UART communication with the FPGA. The firmware can be
  updated by performing port knocking. The knock sequence is to apply
  3.3V through a 10k resistor to the D+ line, while powering on the
  device.

  Physical access and access to a CH55x programmer board means an
  attacker can change the USB controller firmware. This means they can
  listen to and modify all communication between the client and the
  firmware and the client and a device app.
 
  There may be vulnerabilities reachable through USB to the firmware
  of the CH552, allowing both controlled execution and modification of
  the CH552 firmware.

  Possible mitigation: Use of a challenge/response with an app with a
  known public key, like the one used with `tkey-verify`, can prove
  that the communication is unmodified.

## Scope

#### In scope

- SW attacks from the host against the firmware in the FPGA as well as
  the FPGA design itself via the USB host interface.

- Timing attacks on the firmware and the FPGA design.

#### Out of scope

- Leakage and glitching attacks including:

  - Faulting of the execution by the CPU in the FPGA and the CH552
    MCU.

  - EM leakage.

- Warm boot attacks. It should be hard to successfully perform against
  the TKey, but the attack is not yet fully mitigated.

- Attacks on the TKey device apps.

- Attacks on anything outside the FPGA chip.

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
  findings to produce an interesting talk at Chaos Communication
  Congress, USENIX or Security Fest

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

#### In scope

- SW attacks from the client against the firmware in the FPGA as well
  as the FPGA design itself via USB.

- Timing attacks on the firmware and the FPGA design.

#### Out of scope

- All physical and electrical attacks applied to the board, including:

  - Reading out of the UDS from the external flash chip. A bitstream
    on flash is not used in production, only development.
  - Triggering of the FPGA warm boot functionality. It should be hard
    to successfully perform against the TKey, but the attack is not
    yet fully mitigated.

  - Triggering firmware update of the CH552 MCU, using the port
    knocking mechanism.

- Glitching attacks including:

  - Faulting of the execution by the CPU in the FPGA and the CH552 MCU
  - Disturbance of the TRNG entropy generation

- EM leakage.

- Attacks on the TKey device apps.

- Leakage and glitching attacks including:
  - Faulting of the execution by the CPU in the FPGA and the CH552 MCU
  - EM leakage
