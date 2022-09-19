# Threat model

## Introduction
The mta1_mkdf device is a platform for running secure applications in a
restricted execution environment physically separate from the
device host. The secure applications provide functionality and
controlled access to derived secrets on the device. The purpose of the
device is to solve typical end user authentication problems.

For more information about the mta1_mkdf, please see the [mta1_mkdf
System Description](../system_description/system_description.md).

This document describes the threat model for the mta1_mkdf. Based on the
system description and use cases, the threat model tries to capture and
describe the threats that needs to be mitigated in order for the
mta1_mkdf to meet its purpose and objectives.


## Version information
The threat model applies to version one of the mta1_mkdf. The threat
model will be updated as our knowledge and abilities progress and new
versions are developed.

### Version one
* A publicly available, but limited device targeted for a select set of
friendly, competent users.
* Supports Linux and possibly MacOS as host
* Used for a limited set of use cases, primarily for testing

* Users are expected to:
  * Note that a device is missing within 24 hours
  * Note that a device has been physically tampered with
  * Note that a device behaves in an unexpected way


#### Use cases for Version one

**Time Based OTP (TOTP)**
Used at least once per day. Current time supplied by the host.

1. The user connect the device to the host
2. The user runs SW on the host to load the TOTP application
3. The user provides its User Supplied Secret (USS) via host SW
4. The user triggers a TOTP operation using host SW
5. The user press the button on the device
6. The device returns the calculated TOTP token
7. SW on the host use the TOTP token to perform authentication for an
   application or target host


**Ed25519 Signing**
Used at least once per day.

1. The user connect the device to the host
2. The user runs SW on the host to load the Ed25519 application
3. The user triggers an Ed25519 signing operation using host SW. This
   also loads the message or hash to be signed
4. The user press the button on the device
5. The device returns the Ed25519 signature
6. SW on the host use the signature for authentication, verification


**SSH connect**
Used at least once per day.

1. The user connect the device to the host
2. The user runs SW on the host to load the SSH auth application
3. The user provides its User Supplied Secret (USS) via host SW
4. The user trigger a SSH connect operation from the host
5. The user press the button on the device
6. The device returns a SSH signature
7. The SSH application on the host use the SSH signature for
   authentication to the remote SSH server


### Version N
* Publicly available devices for end users for which there are no
  expectation on knowledge or competence beyond normal IT usage skills

* Used in normal IT systems, but also for more sensitive enterprise and
  operational use cases


## Assumptions
* There are no backdoors or vulnerabilities in Lattice iCE40 UltraPlus
  FPGA devices that allow access to internal configuration memory after
  the device has been locked.

* The Project IceStorm toolchain, including YoSys and NextPnR generates
  a correct design, and also does not inject hardware exfiltration
  mechanisms in the generated bitstream.

* There is no access to the contents of the internal, Non-Volatile
  Configuration Memory (NVM) from the FPGA fabric besides the
  configuration circuit.

* Toolchain for development of FPGA HW, application_fpga FW does not
  contain backdoors etc.

* The design including source code for FPGA, SDK, FW, boot SW, board
  design is open and published.

* The end user is not an attacker. The end user at least doesn't
  knowingly aid, support the attacker in attacks on its device


## Assets
* UDS - Unique Device Secret. Provisioned and stored during
  device manufacturing. Never to be replaced during the life time of
  a given device. Used to derive application secrets. Must never leave
  the device. Mullvad must NOT store a copy of the UDS.

* USS - User Supplied Secret. Provisioned by the application. May
  possibly be replaced many times. Supplied from the host to the
  device. Should not be revealed to a third party.

* UDI - Unique Device ID. Provisioned and stored during
  device manufacturing. Never to be replaced or altered during the life
  time of a given device. May be copied, extracted, read from the device.

* UDA - Unique Device Authentication Secret. Provisioned and stored during
  device manufacturing. Never to be replaced during the life time of
  a given device. Used to authenticate a specific device. Must never
  leave the device. Mullvad MUST have a copy of the UDA.


## Threat Actors - The bad guys
Different actors have different reasons, access to competence, resources
etc. This description tries to capture the possible attacks and attacks
vectors through four synthetic threat actors.


### 0. Average Joe
[Average Joe Soundtrack](https://www.youtube.com/watch?v=BB0DU4DoPP4)

* Curious opportunist
* No real competence, no resources beyond a personal computer
* No planning or preparation before an attack
* Prepared to invest little time (minutes) or resources - for example to
  connect a device found, try a few passwords
* End game is to gain access to possible information, resources unknown
  to the attacker before the attack is performed


### 1. The CCC Hacker
[CCC Hacker Soundtrack](https://www.youtube.com/watch?v=l8DBEbmPh7E)

* Sympathetic to the goals of the project
* Wants to probe all parts and the system in a quest to determine how
  the device really works, use it in possibly different ways, find
  weaknesses (and get them fixed).
* Is possibly a user, but in this case not the legitimate end user
* Have a high level of competence
* Prepared to spend time to prepare and perform an attack. Possibly low
  effort over an extended period
* Access to compute resources. Possibly access to lab equipment
* Will try all possible SW and HW attack vectors. In and out of scope
* End game is to find flaws in threat model. Acquire knowledge and
  findings to produce an interesting talk at CCC, USENIX or Security
  Fest


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
* Prepared to perform physical visits (missions) at target (end user) as
  well as Mullvad or Mullvad suppliers in order to manipulate, steal,
  replace components, systems
* SW based attacks. Is assumed to remotely own the host
* Supply chain attacks - both on SW and HW, components
* Supply chain attacks on application, host application, SDK,
  development
* End game: Long term stealth presence providing access to information
  about the end user


## Attacks in Scope
The following attacks are in scope for version one

* All digital attacks from the host including but not limited to:

  * The framing protocol and all recipients (endpoints)
    * manipulation, fuzzing, injection, reordering and replay of any and
      all communication

* Time based side channel attacks on challenge-response device
authentication

* Time based side channel attacks on UDS based key derivation

* Time based side channel attacks on secure applications developed by
  Mullvad

* Supply chain physical attacks on devices provisioned by Mullvad

* Decapping and physical probing on the FPGA

* Supply chain attacks on Mullvad provided HW and SW design resources


## Attacks out of scope
The following attacks are out of scope for version one

* Electromagnetic-, power-, and optical-based fault injection attacks

* Electromagnetic-based side channel, differential power analysis and
  correlation leakage attacks



## Consequences - Game Over
The following Game Over Scenarios have been identified

* The attacker gains access to the UDS and the USS
  * Requires replacement of the device with a new unit


## Work In Progress
TODOs and random notes.

* TODO: Mention and separately describe the NVCM and the CRAM.

* TODO: Mention limitations related to EBR and SPRAM
