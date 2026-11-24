# Project plan: machine-vibration-health-node

The plan of the project: goal, phases with the proof that ends each one, and risks.
The current state of the system is in [architecture.md](architecture.md). Decisions are in
[decisions/](decisions/).

## Goal

The device sits on a rotating machine (here a 5 V fan) and detects from its vibration when it starts
to fail. It boots only signed software and updates remotely without bricking, even if the power is
cut in the middle of an update. A test rig proves all of this automatically on the real board.

**Why vibration and not temperature.** When a bearing wears or a shaft goes out of alignment, new
frequencies appear in the vibration weeks before the machine fails, so it can be replaced at a
planned stop. Sampling thousands of times per second at exactly equal intervals is also what
justifies the real-time core; one temperature reading per second could be done by Linux alone.

## Architecture

One chip: the Cortex-M33 (Zephyr) samples the vibration and computes an FFT, the Cortex-A35 (Linux)
detects anomalies and is updated securely. Every boot stage verifies the signature of the next one.
Linux loads the M33 firmware (remoteproc) and talks to it over RPMsg. The diagram is in
[architecture.md](architecture.md).

Board constraints that shape the plan (STM32MP215F-DK):
- One Cortex-A35 and a Cortex-M33 at 300 MHz: measure the FFT processing time per window.
- No NPU: anomaly detection is statistical, on the A35.
- Boot only from microSD (no eMMC): the RAUC A/B slots are partitions on the same card.
- 100 Mbit Ethernet: enough for updates and MQTT.

## System hardware

**Vibration sensor.** ADXL345 on a breakout, bolted onto the fan (not onto the HAT, otherwise it
measures the board) and connected to the HAT with a short SPI cable. Criteria: SPI, a sample rate of
at least a few kHz, an internal FIFO, available as a breakout.

**Fan.** A 5 V fan is the monitored machine. The sensor is fixed rigidly on its body. Before the
detection is written, the spectrum with and without tape on a blade is compared, to confirm the
difference is clearly visible.

**HAT.** A small two-layer KiCad PCB on pins 17-30 of the 40-pin header (the other pins stay free),
with hand-solderable parts: a connector for the sensor, an EEPROM, an LED and a button. Ordered at
the end of phase 2 so it arrives before bring-up in phase 7. Every connection is checked against the datasheets before ordering; an error
that gets through is fixed with a wire and recorded.

**Test rig.** A Raspberry Pi Zero 2 W with labgrid controls a relay on the board's 5 V DC supply
(never mains) and reads the console through a USB-UART. An INA219 in the USB-C power line, next to
the relay, measures the board's current, and the Pi Zero reads it over I2C. A Raspberry Pi Pico
acts as a logic analyzer for the sensor SPI and HAT bring-up. Tests are written in pytest.

## Phases

About 13 weeks, with no deadline. PCB design starts early so the boards arrive before bring-up, and
sampling starts with the sensor on wires. Every phase ends with a proof on the board.

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

### 1. Bring-up and own image (weeks 1-2) `[x]`
I start from the ready-made ST image, then build my own with the ST BSP layer and my own layer,
`meta-vibration-node`. I read every boot stage on the serial console.
- Proof: my image boots from microSD, and a table shows the time of each stage, measured from the log.
- Done (2026-11-18): `vibration-image` boots from microSD in 15.0 s, logs in
  `captures/boot-time/vibration-image-trimmed_*.log`, stage table in [architecture.md](architecture.md).

### 2. HAT design (weeks 2-4) `[~]`
Schematic and two-layer layout in KiCad: sensor connector, EEPROM, LED, button.
- Proof: ERC and DRC with no errors, every connection checked against the datasheets, order placed at
  the end of week 4.

### 3. Sampling and FFT on the M33 (weeks 3-5) `[~]`
With the breakout on wires, before the HAT arrives. Zephyr firmware reads the sensor over SPI with DMA
at a fixed rate, computes the FFT and sends the spectrum to Linux over RPMsg. Linux loads the firmware
with remoteproc.
- Proof: the logic analyzer shows samples at equal intervals, with the jitter measured. The spectrum
  shows a peak at the fan's rotation frequency. I kill the M33 firmware, and Linux detects it and
  restarts it.

### 4. Secure boot (weeks 6-7) `[ ]`
Keys, signing of TF-A and FIP, OP-TEE, U-Boot that accepts only a signed FIT (kernel and device tree).
- Proof: a tampered kernel image is rejected at boot, and the log shows it.
- Decision point: the fuses and closing the chip are irreversible.

### 5. A/B updates with RAUC (weeks 8-9) `[ ]`
Two rootfs slots, signed bundles, boot counter in U-Boot, watchdog and a health check after boot.
Read-only rootfs with dm-verity, a separate data partition, refusal of older versions. The update also
carries the M33 firmware.
- Proof: a broken image rolls back by itself to the previous slot, and an older version is rejected.

### 6. Test rig (weeks 9-10) `[ ]`
The Pi Zero 2 W with labgrid controls the relay and reads the console. Tests in pytest: boot, update,
power cut at a random moment of an update, rollback, data flow.
- Proof: one command runs the whole suite. 100 updates with random power cuts, and the device boots
  from a valid slot every time.

### 7. HAT bring-up and anomaly detection (weeks 10-11) `[ ]`
Soldering, multimeter measurements, device tree overlay, driver for the EEPROM. The daemon learns
the spectrum of the healthy fan and sends an MQTT alert when it changes, starting with a simple
statistical comparison.
- Proof: with tape on a fan blade, an alert within a few seconds. Without tape, no false alert in 24
  hours. A graph shows the board power at every stage of an update.

### 8. Wrap-up (weeks 12-13) `[ ]`
SBOM in SPDX from Yocto, CVE report, threat model, support-period document, demo video. README with
measured results and 2-3 cases where a mistake was made and how it was fixed.
- Proof: someone who does not know me builds the image and runs the tests from the README alone.

If something runs late, cut from the end: phase 8 shrinks and phase 7 stays on simple statistical
detection. Phases 3 to 6 are the core and are not cut.

## Risks

| Risk | What I do |
|---|---|
| **Fuses and closing the chip.** Writing the keys to the fuses and closing the chip are irreversible; closing without a working signature check kills the chip. ST's guide covers the MP13 and MP15, not the MP21 ([ST wiki](https://wiki.st.com/stm32mpu/wiki/How_to_perform_Secure_Boot_from_Distribution_package)). | I read the MP21 documentation first. I set up and test the whole chain without closing. I never close the only board. |
| **RPMsg from Zephyr on the M33.** Not confirmed for this board. An RPMsg problem was reported on the MP257 DK with STM32Cube firmware ([ST community](https://community.st.com/stm32-mpus-embedded-software-and-solutions-46/stm32mp257-openamp-rpmsg-endpoint-not-getting-created-161965)). | I test it on the first day of phase 3. If it does not work in 2-3 days, I switch to ST's firmware (STM32CubeMP2) and keep Zephyr for later. |
| **Weak vibration signal.** The sensor may pick up more noise from the table than vibration from the fan. | I fix the sensor rigidly on the fan. I compare the spectrum with and without tape before writing detection. |
| **First PCB revision with errors.** Normal for a first spin. | I check every connection against the datasheets and the CN12 pinout before ordering. I fix errors with a wire and record them. |
| **The relay cuts the board's 5 V.** | I switch only the 5 V DC line, with a common ground. I measure with a multimeter before connecting the board. Never mains voltage. |
| **Time.** 13 weeks is ambitious. | I cut from the end (see phases). Phases 3 to 6 stay. |
