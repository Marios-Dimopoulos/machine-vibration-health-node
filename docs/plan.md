# Project plan: machine-vibration-health-node

Created 2026-11-07, adapted for the STM32MP215F-DK. This is the plan of the project: scope, hardware,
phases, budget, working rules and risks. The current state of the system lives in
[architecture.md](architecture.md). Decisions live in [decisions/](decisions/).

The project is an industrial predictive-maintenance device built on STM32MP2 (Cortex-A35 running Linux
plus Cortex-M33 running Zephyr). The M33 reads the vibration of a machine thousands of times per second
and analyzes it with an FFT. Linux on the A35 detects when the machine starts to fail and sends an alert.
Underneath: secure boot, A/B updates with RAUC, a custom HAT designed in KiCad, and a test rig that
automatically verifies all of the above on the real board.

Repository name: `machine-vibration-health-node`.

## Summary and goal

The device sits on a rotating machine (here a 5 V fan) and understands from the vibration when it starts
to fail. It boots only signed software and updates remotely without "bricking", even if the power is cut
in the middle of an update. A test rig proves all of this automatically.

**Why vibration and not temperature.** When a bearing wears or a shaft goes out of alignment, new
frequencies appear in the vibration weeks before the machine fails. The factory replaces it at a planned
stop instead of an unexpected line stoppage. In addition, sampling thousands of samples per second at
exactly equal intervals really justifies the real-time core. One temperature reading per second could be
read comfortably by Linux alone.

**Size and time.** About 13 weeks at 4 to 6 hours per day, with no deadline: roughly 350 to 500 hours.
Every phase ends with a proof on the board.

### Technical skills per phase

| Skill | Phase |
|---|---|
| Boot chain (TF-A, OP-TEE, U-Boot) and secure boot | 1, 4 |
| A/B updates with RAUC, rollback, dm-verity | 5 |
| High-rate sampling with DMA, FFT on the M33 | 3 |
| Two cores: remoteproc, RPMsg, Zephyr | 3 |
| Anomaly detection | 7 |
| Device tree and drivers for the HAT | 7 |
| PCB design and bring-up | 2, 7 |
| Hardware-in-the-loop tests with labgrid and pytest | 6 |
| SBOM, CVE report, threat model | 8 |

## Architecture

One chip: the M33 analyzes vibration, Linux is updated securely. The diagram is in
[architecture.md](architecture.md).

Every boot stage verifies the signature of the next one. Linux loads the M33 firmware (remoteproc) and
talks to it over RPMsg, much like the previous STM32 and Pi talked over SPI.

Components:
- Boot chain: Boot ROM, TF-A (BL2), OP-TEE, U-Boot, Linux kernel.
- Cortex-M33: Zephyr, samples and FFT, RPMsg to Linux. remoteproc: Linux loads the firmware.
- Cortex-A35 (Linux): RAUC with slot A/B (update with rollback), daemon (detection, MQTT/TLS),
  dm-verity rootfs (read-only), watchdog (reset if it hangs).
- Development PC: Yocto build, signing, OTA, SBOM.
- MQTT broker: receives alerts.
- HAT (own KiCad PCB): current measurement, EEPROM, connector. Connected to the 40-pin header.
- Test rig: Pi Zero 2 W + labgrid, pytest: boot, update, power cut, rollback. Controls power and console.
- Fan + ADXL345: the "machine" being monitored. Connected to the HAT by SPI over a cable.

## Platform and board choice

A SoC with a Cortex-A for Linux and a Cortex-M for real-time on the same chip is needed. This is the
setup used by modern industrial products, and it replaces the old system where the STM32 and the Pi were
two separate boards.

**What was considered** (prices and stock of 2026-10-31, excluding VAT):

| Board | Cores | Price | Availability | Comment |
|---|---|---|---|---|
| [STM32MP257F-DK](https://www.st.com/en/evaluation-tools/stm32mp257f-dk.html) | 2x Cortex-A35 1.5 GHz, Cortex-M33 400 MHz, NPU, 4 GB LPDDR4 | 88.32 EUR ([Farnell](https://si.farnell.com/stmicroelectronics/stm32mp257f-dk/discovery-kit-arm-cortex-a35-m33f/dp/4628687)) | 49 in stock | The original plan. Same STM32 family, the "F" = secure boot and cryptography, the M33 is supported by [Zephyr](https://docs.zephyrproject.org/latest/boards/st/stm32mp257f_dk/doc/index.html) |
| [FRDM-IMX93](https://export.farnell.com/nxp/frdm-imx93/frdm-development-board-for-i-mx/dp/4626785) | 2x Cortex-A55 1.7 GHz, Cortex-M33 250 MHz, NPU, Secure Enclave, CAN | 88.86 GBP | Out of stock, deliveries from 2026-12-16 | Equally good if you find it right away, the project transfers with the same logic. Lead time up to 39 weeks at Farnell |
| [STM32MP157F-DK2](https://www.tme.eu/gb/details/stm32mp157f-dk2/stm-development-kits/stmicroelectronics/) | 2x Cortex-A7 800 MHz, Cortex-M4 | 121.37 GBP | 0 in stock | Older generation (32-bit) and more expensive |

With Greek VAT of 24%, the STM32MP257F-DK costs about 110 EUR (102.69 EUR at the
[ST eStore](https://estore.st.com/en/stm32mp257f-dk-cpn.html)).

**Board change: STM32MP215F-DK.** The STM32MP257F-DK was no longer available, so the project is built
on the STM32MP215F-DK (ordered from Mouser Greece, 63.73 EUR excluding VAT). These differences apply to
the plan:

| Topic | STM32MP257F-DK (original plan) | STM32MP215F-DK (chosen) | Effect on the project |
|---|---|---|---|
| Cortex-A | 2x A35, 1.5 GHz | 1x A35, 1.5 GHz | No change in the architecture |
| Cortex-M33 | 400 MHz | 300 MHz | The FFT runs at a lower clock: measure the processing time per window |
| NPU | Yes | No | Anomaly detection is done only by statistical comparison on the A35 (the NPU model is dropped) |
| Memory | 4 GB LPDDR4 | 2 GB LPDDR4 | Enough for your own Yocto image |
| Boot storage | - | microSD (no eMMC) | The RAUC A/B slots are partitions on the same microSD |
| Connectivity | - | 100 Mbit Ethernet (RJ45), USB-C power, M.2 E-Key slot, 40-pin GPIO | The update loop and MQTT need no more than 100 Mbit |

**Unverified for the MP215F-DK, to be checked before the corresponding phases:**
1. The secure boot flow on the MP21 (TF-A, OP-TEE, signed FIT).
2. The pinout of the 40-pin connector (CN12) compared to Raspberry Pi, before ordering the HAT PCB.
3. The type of the CN5 connector.

## System hardware: sensor, HAT, rig

**Vibration sensor.** Bolted onto the fan (not onto the HAT, otherwise you measure the vibration of the
board) as a ready-made breakout module connected with a short cable to the HAT connector. Chip selection
criteria (decision in phase 2, with an ADR): SPI interface, sample rate of at least a few kHz, internal
FIFO, and available as a breakout. The choice that ended up in the order is the **ADXL345**.

**Fan and mounting.** A 5 V fan (for example the kind used in Raspberry Pi cases) that needs no extra
power supply, and is the "machine". The sensor is fixed rigidly on the body of the fan, because the
sensor can pick up more noise from the table than vibration from the fan. Before the detection is
written, the spectrum is compared with and without tape and it is confirmed that the difference is
clearly visible.

**HAT (own PCB, KiCad).** A small two-layer HAT, not a board around the SoC (PCB design is not a core
requirement for an embedded software role; reading schematics and bringing up a new board is). It contains:
- a connector for the vibration sensor
- a current-measurement chip (for example INA219)
- EEPROM, LED, button
- hand-solderable components

PCB order: 5 pieces from JLCPCB or similar. Small two-layer boards up to 100x100 mm have a base price of
about 2 USD ([cost guide](https://highway.hackclub.com/guides/JLC-cost-optimizing)); the biggest cost is
shipping. The order is placed at the end of week 4 (completion of phase 2), so the boards arrive before
bring-up (phase 7). The accelerometer is not on the HAT: it is read over an SPI cable. Before ordering,
every connection is checked against the datasheet of the corresponding part; an error that gets through
is fixed with a wire and recorded (a first PCB revision with errors is normal).

**Test rig.** The Raspberry Pi Zero 2 W and [labgrid](https://github.com/labgrid-project/labgrid)
control a relay on the board power and read the console from a USB-UART (with a micro-USB hub for enough
ports). The Raspberry Pi Pico acts as a logic analyzer for the sensor SPI and for HAT bring-up. The tests
are in pytest: boot, update, power cut at a random moment of an update, rollback, data flow.

**Hardware you already have.** Raspberry Pi Zero 2 W, Nucleo-F411RE (not needed in this project),
Raspberry Pi Pico (logic analyzer), USB-UART, 2 microSD 32 GB, passives and cables. The two microSD, the
Pi Zero 2 W, the USB-UART and the Pico cover the rest. A 5 V fan and a relay that switches only the 5 V DC
line (never mains voltage) were then bought, in the Mouser order.

## Phases

The work fits in about 13 weeks (about 4 to 6 hours per day). PCB design starts early so the boards arrive
before bring-up, and sampling starts with the sensor on wires. Every phase ends with a proof on the board.
The only irreversible decision (fuses) comes at the end of week 7.

| Phase | Weeks | Decision points |
|---|---|---|
| 1. Bring-up, own Yocto image | 1-2 | |
| 2. HAT design in KiCad | 2-4 | PCB order: end of week 4 |
| 3. Samples and FFT on the M33 | 3-5 | |
| 4. Secure boot | 6-7 | Fuses or not: end of week 7 (irreversible) |
| 5. A/B updates with RAUC | 8-9 | |
| 6. Test rig with labgrid | 9-10 | |
| 7. HAT bring-up, anomaly detection | 10-11 | |
| 8. SBOM, threat model, documentation | 12-13 | Demo and delivery: end of week 13 |

Status legend: `[ ]` not started, `[~]` in progress, `[x]` done and proof recorded in `docs/captures/`.

### 1. Bring-up and own image (weeks 1-2) `[ ]`
Start from the ready-made ST image and then build your own, with the ST BSP layer and your own layer, such
as `meta-vibration-node`. Read every boot stage on the serial console.
- Proof: your image boots from microSD, and a table shows the time of each stage, measured from the log.

### 2. HAT design (weeks 2-4) `[ ]`
Schematic and two-layer layout in KiCad: connector for the vibration sensor, current-measurement chip
(for example INA219), EEPROM, LED, button. Hand-solderable components. Here you also choose the
accelerometer, with an ADR.
- Proof: ERC and DRC with no errors, every connection checked against the datasheets, order placed at the
  end of week 4.

### 3. Sampling and FFT on the M33 (weeks 3-5) `[ ]`
With the breakout on wires, before the HAT arrives. Zephyr firmware that reads the sensor over SPI with
DMA at a fixed rate, computes the FFT and sends the spectrum to Linux over RPMsg. Linux loads the firmware
with remoteproc.
- Proof: the logic analyzer shows that samples arrive at equal intervals, and you measure how much they
  deviate (jitter). The spectrum shows a peak at the rotation frequency of the fan. You kill the M33
  firmware, and Linux detects it and restarts it.

### 4. Secure boot (weeks 6-7) `[ ]`
Keys, signing of TF-A and FIP, OP-TEE, U-Boot that accepts only a signed FIT (kernel and device tree).
- Proof: a tampered kernel image is rejected at boot, and the log shows it.
- Decision point: the fuses and the "closing" of the chip are irreversible. The decision is written in an ADR.

### 5. A/B updates with RAUC (weeks 8-9) `[ ]`
Two rootfs slots, signed bundles, boot counter in U-Boot, watchdog and a health check after boot. Read-only
rootfs with dm-verity, a separate data partition, refusal of older versions. The update also carries the
M33 firmware.
- Proof: a broken image rolls back by itself to the previous slot, and an older version is rejected.

### 6. Test rig (weeks 9-10) `[ ]`
The Pi Zero 2 W with labgrid controls a relay on the power and reads the console from the USB-UART. Tests
in pytest: boot, update, power cut at a random moment of an update, rollback, data flow.
- Proof: one command runs the whole suite. 100 updates with random power cuts, and the device boots from
  a valid slot every time.

### 7. HAT bring-up and anomaly detection (weeks 10-11) `[ ]`
Soldering, multimeter measurements, device tree overlay, drivers for the current measurement and the
EEPROM. The daemon learns the spectrum of the healthy fan and sends an MQTT alert when it changes. You
start with a simple statistical comparison. A small model on the NPU was an optional step (the
MP215F-DK has no NPU, so detection stays statistical).
- Proof: with tape on a fan blade, an alert within a few seconds. Without tape, no false alert in 24
  hours. A graph shows the board power at every stage of an update.

### 8. Wrap-up (weeks 12-13) `[ ]`
SBOM in SPDX from Yocto, CVE report, threat model, support-period document, README with measured results,
demo video.
- Proof: someone who does not know you builds the image and runs the tests from the README alone.

If something runs late, cut from the end: phase 8 shrinks and phase 7 stays on simple statistical
detection. Phases 3 to 6 are the core and are not cut. If time is left over, the next step is the model
on the NPU (which does not exist on the MP215F-DK) or a patch to an open source project (Zephyr, U-Boot)
for something you find broken.

## Hardware and budget

The hardware of the project costs about 160 to 205 EUR (initial estimate with the MP257F-DK) and, with
a soldering iron and a multimeter, 205 to 280 EUR. Only the price of the board was verified, the rest are
estimates. If the frame must stay close to 200 EUR, a possible saving is borrowing a soldering iron and a
multimeter from a lab (45 to 75 EUR).

| Item | Purpose | Cost incl. VAT (EUR) |
|---|---|---|
| Board (originally STM32MP257F-DK, finally STM32MP215F-DK) | The board of the project | Draft: ~110 for the MP257F-DK. Order: 63.73 excl. VAT for the MP215F-DK |
| USB-C power supply 5 V / 3 A | ST recommends it for the board ([data brief](https://st.com/resource/en/data_brief/stm32mp257f-dk.pdf)). Not found whether it is included in the box. The house has an Apple USB-C charger (with C-to-C cable) and a Xiaomi 15 W USB-A (with A-to-C cable) as an alternative | 10-15 (estimate) |
| HAT PCB, 5 pieces, with shipping | From JLCPCB or similar | 15-25 (estimate) |
| HAT components | Current-measurement chip, EEPROM, connectors, header, passives | 10-20 (estimate) |
| Accelerometer on a breakout module | The vibration sensor (finally ADXL345) | 5-15 (estimate) |
| 5 V fan | The "machine" you monitor. A 5 V one, for example the Raspberry Pi case type | 3-8 (estimate); chosen type 5 V 40 mm, ~5 EUR, local or Skroutz |
| Relay and micro-USB hub | So the Pi Zero cuts the board power and has ports for the console | 8-12 (estimate) |
| Soldering iron with temperature control, solder, flux | For the HAT. If you get a Pinecil, it needs a USB-C PD 20 V / 3 A charger ([spec](https://arnowelzel.de/en/pinecil-v2)) | 30-50 (estimate) |
| Multimeter | For HAT bring-up | 15-25 (estimate) |

The available budget is 100-150 EUR, up to about 200 EUR.

**Order placed (Mouser Greece, DDP, FedEx International Priority).** Total incl. VAT 100.62 EUR
(products 83.85 EUR, shipping 0.00 EUR, VAT 16.77 EUR):

| Product | Price excl. VAT (EUR) |
|---|---|
| STM32MP215F-DK (511-STM32MP215F-DK) | 63.73 |
| SEN0032 | 5.07 |
| DFR0251 | 8.43 |
| FIT0030 | 1.55 |
| FIT0121 | 5.07 |

**Tools from Skroutz** (needed by week 3-4, for the accelerometer headers, and for the HAT of week 6).
Prices as planned at the time of writing; the actual cart may differ.

| Item | Price (EUR) |
|---|---|
| 908S 80 W soldering iron (the review and the temperature control are unverified) | 9.87 |
| Goobay solder 0.8 mm, 20 g | 8.03 |
| Side cutter Optum Plato 170 | 3.99 |
| Multimeter UT131B | 13.80 |
| Desoldering braid Goot CP-2015 (optional) | 6.90 |
| 5 V 40 mm fan | ~5 |
| microSD card and reader (the board boots only from microSD; the two 32 GB microSD you already have can cover it) | optional |

Flux is optional, because the solder most likely has a rosin core. The PCBs and the HAT components have
not been ordered yet and are not included in the two orders above.

## Working practices and safety

- **What you learn, you write first.** Boot chain, secure boot, RAUC settings, device tree, Zephyr: first
  version is yours, then review and questions about mistakes. (In this project the working mode is
  defined in `CLAUDE.md`: explain first, ready example, user runs and modifies it.)
- **What you generate, you check.** Tests in pytest, scripts, glue in recipes, README: checking is quick.
- **The rig is the independent check.** Any code written with the help of tools passes through the tests
  on the board.
- **Every claim about the hardware is checked on the page.** When a tool says what a bit or a fuse does,
  you write the document and the page in the ADR (like UM1724 in the previous repo).
- **Signing keys never go into a prompt or the repo.** The same applies to Wi-Fi passwords.
- **README with measured results**, and a section with 2-3 cases where a mistake was made and how it was
  fixed. Decisions (fuses, sensor choice) are written in ADRs.

## Risks and what to watch for

The biggest risk is "locking" the board in secure boot. The others only cost time.

| Risk | What you do |
|---|---|
| **Fuses and closing the chip.** In ST's guide for STM32MP13 and MP15, writing the key to the fuses and the closing are irreversible. If you close without the signature check having passed, the chip dies ([ST wiki](https://wiki.st.com/stm32mpu/wiki/How_to_perform_Secure_Boot_from_Distribution_package)). The page does not cover the MP25 (nor the MP21), so the flow was not verified. | Read the MP21 documentation first. Set up and test the whole chain without closing. Never close your only board. The decision is written in an ADR (end of week 7). |
| **RPMsg from Zephyr on the M33.** Zephyr supports the M33 of the DK and loading with remoteproc, but its page does not mention OpenAMP or RPMsg. On the ST forum there is a report of an RPMsg problem on the same DK, with STM32Cube firmware, marked as solved ([ST community](https://community.st.com/stm32-mpus-embedded-software-and-solutions-46/stm32mp257-openamp-rpmsg-endpoint-not-getting-created-161965)). | Test it on the first day of phase 3. If it does not work in 2-3 days, switch to ST's firmware (STM32CubeMP2) and keep Zephyr for later. For the MP215F-DK the issue was unverified. |
| **Weak vibration signal.** The sensor may pick up more noise from the table than vibration from the fan. | Fix the sensor rigidly on the body of the fan. Before writing detection, compare the spectrum with and without tape and make sure the difference is clearly visible. |
| **First PCB revision with errors.** This is normal. | Check every connection against the datasheet before ordering. An error is fixed with a wire and recorded, like every company on its first spin. For the CN12 pinout on the MP215F-DK: check before ordering the PCB. |
| **The relay cuts the board's 5 V.** | Cut only the supply line (5 V DC), with a common ground. Measure with a multimeter before connecting the board. Never mains voltage. |
| **Time.** 13 weeks is ambitious. | Cut from the end (see phases). Phases 3 to 6 stay. |
| **Board without an NPU.** The MP215F-DK has no NPU. | Anomaly detection is done statistically on the A35. |

## Sources

Pages opened for the initial document, on 2026-10-31.

- ST: [STM32MP257F-DK](https://www.st.com/en/evaluation-tools/stm32mp257f-dk.html),
  [data brief](https://st.com/resource/en/data_brief/stm32mp257f-dk.pdf),
  [STM32MP257](https://st.com/en/microcontrollers-microprocessors/stm32mp257.html),
  [eStore](https://estore.st.com/en/stm32mp257f-dk-cpn.html)
- Farnell: [STM32MP257F-DK](https://si.farnell.com/stmicroelectronics/stm32mp257f-dk/discovery-kit-arm-cortex-a35-m33f/dp/4628687),
  [FRDM-IMX93](https://export.farnell.com/nxp/frdm-imx93/frdm-development-board-for-i-mx/dp/4626785)
- TME: [STM32MP157F-DK2](https://www.tme.eu/gb/details/stm32mp157f-dk2/stm-development-kits/stmicroelectronics/)
- Zephyr: [STM32MP257F-DK](https://docs.zephyrproject.org/latest/boards/st/stm32mp257f_dk/doc/index.html)
- ST wiki: [Secure Boot from the Distribution package](https://wiki.st.com/stm32mpu/wiki/How_to_perform_Secure_Boot_from_Distribution_package)
- ST community: [RPMsg endpoint on STM32MP257](https://community.st.com/stm32-mpus-embedded-software-and-solutions-46/stm32mp257-openamp-rpmsg-endpoint-not-getting-created-161965)
- [labgrid](https://github.com/labgrid-project/labgrid)
- Hack Club: [JLC cost optimizing](https://highway.hackclub.com/guides/JLC-cost-optimizing)
- Pinecil V2: [power supply specifications](https://arnowelzel.de/en/pinecil-v2)
