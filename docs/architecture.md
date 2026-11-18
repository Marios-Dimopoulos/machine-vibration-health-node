# Architecture: current state

This file describes the CURRENT state of the whole system, not the plan. The plan is in
[plan.md](plan.md). Update this file whenever something in the system changes (a component is built,
a decision is made, a measurement is taken), and keep the "Status" column honest.

Last updated: 2026-11-18.

## Summary

Phase 1 (bring-up and own image) is in progress. The board boots ST's ready-made image (Starter
Package) from microSD, and the A35 console works. ST's `st-image-weston` has also been rebuilt from
source with Yocto. My own image, `vibration-image` (layer `meta-vibration-node`), runs my first
program, `vibration-hello`. Trimmed down to what the product needs (packages, services, initrd,
kernel drivers, device tree), it boots in 15.0 s from TF-A to login (29.0 s before trimming).

Status legend: `planned` (not started), `in progress`, `working` (proof recorded in `captures/`),
`blocked`.

## Target architecture (what I am building)

One board, one HAT, one test rig. The M33 analyzes vibration, Linux is updated securely.

```mermaid
flowchart TB
    subgraph SoC["STM32MP215F-DK (SoC)"]
        direction TB
        subgraph BOOT["Boot chain"]
            direction TB
            ROM["Boot ROM"] --> TFA["TF-A (BL2)"] --> OPTEE["OP-TEE"] --> UBOOT["U-Boot"] --> KERNEL["Linux kernel"]
        end
        subgraph M33["Cortex-M33 (Zephyr)"]
            direction TB
            SAMP["Sampling (SPI + DMA), FFT"] --> RPMSG["RPMsg"]
        end
        subgraph A35["Cortex-A35 (Linux)"]
            direction TB
            RAUC["RAUC, slot A / B (update with rollback)"]
            DAEMON["Daemon (detection, MQTT/TLS)"]
            VERITY["dm-verity rootfs (read-only)"]
            WDT["Watchdog (reset if it hangs)"]
        end
        KERNEL -. "starts Linux" .-> A35
        A35 -. "remoteproc loads the firmware" .-> M33
        RPMSG <--> DAEMON
    end
    DEV["Development PC: Yocto build, signing, OTA, SBOM"] --> RAUC
    DAEMON --> MQTT["MQTT broker (alerts)"]
    HAT["HAT (own KiCad PCB): current measurement, EEPROM, connector"] -- "40-pin header" --- SoC
    RIG["Test rig: Pi Zero 2 W + labgrid + pytest (boot, update, power cut, rollback)"] -- "power, console" --- SoC
    FAN["Fan + ADXL345: the monitored machine"] -- "SPI over cable" --- HAT
```

Every boot stage verifies the signature of the next one. Linux loads the M33 firmware (remoteproc) and
talks to it over RPMsg.

## Components and status

| Component | Role | Status |
|---|---|---|
| STM32MP215F-DK | The board of the project | working: boots from microSD (2026-11-13). In USB/UART programming mode (switches 0000) LD5 blinks at about 3-4 Hz (Boot ROM waiting, PH4) |
| Boot chain (Boot ROM, TF-A, OP-TEE, U-Boot) | Signed boot | ST's binaries boot (TF-A v2.10, OP-TEE 4.0, U-Boot 2023.10, Linux 6.6), see `captures/st-starter_first-boot_untimed.log`. Not signed yet (phase 4) |
| Yocto / OpenSTLinux image (`yocto/`) | The Linux system for the A35 | in progress: ST Starter Package OpenSTLinux v26.06.10 (A35-TD, flash layout `FlashLayout_sdcard_stm32mp215f-dk-ca35tdcid-ostl-optee.tsv`) boots (37.3-37.6 s, TF-A to login). ST's `st-image-weston` rebuilt from source boots too (38.1-38.2 s). My own `vibration-image` boots in 15.0 s (29.0 s before trimming). See the Yocto build section |
| Own layer `meta-vibration-node` (`yocto/layers/`) | My recipes and image | working: `vibration-hello` (C program, cross-compiled for aarch64), `vibration-image` (image recipe), and appends to ST's U-Boot, extlinux, kernel and base-files recipes |
| Zephyr firmware for the M33 (`firmware/`) | Sampling, FFT, RPMsg | planned (phase 3) |
| remoteproc + RPMsg link | M33 <-> Linux communication | planned (phase 3), unverified on this board |
| Detection daemon | Statistical anomaly detection, MQTT/TLS alerts | planned (phase 7) |
| RAUC A/B update system | Updates with rollback, dm-verity, watchdog | planned (phase 5) |
| HAT (KiCad PCB) | Current measurement, EEPROM, LED, button, sensor connector | planned (phase 2, bring-up phase 7), not designed yet |
| Test rig (Pi Zero 2 W, labgrid, pytest, relay, USB-UART, Pico) | Automatic verification on the real board | planned (phase 6) |
| Sensor: ADXL345 on a breakout | Vibration measurement | header soldered (pins on the sensor side, continuity checked), not yet connected |
| 5 V fan | The "machine" being monitored | received, not yet used |

## Hardware inventory (what exists today)

Received from the Mouser order (2026-11-11):
- STM32MP215F-DK (511-STM32MP215F-DK).
- SEN0032: DFRobot ADXL345 triple-axis accelerometer breakout (I2C and SPI).
- DFR0251: DFRobot Gravity digital 16 A relay module, 1 channel, 5 V DC supply. Its DC contact rating
  must be checked before phase 6.
- FIT0121: DFRobot jumper wires, female/male, 65 pack.
- FIT0030: wires.
  (Identified by part number from distributor listings: TME, Digi-Key, Jameco, DFRobot wiki.)

Already owned: Raspberry Pi Zero 2 W, Raspberry Pi Pico (logic analyzer), USB-UART adapter (used for
the board's serial console), microSD card reader, power supply for the board (Samsung USB-C charger, 15 W, 5 V / 3 A,
with a USB-C to USB-C cable), 2 microSD 32 GB,
passives and cables, Nucleo-F411RE (not used in this project).
Note: ST verified only SanDisk Class 10 / UHS-1 microSD cards for this board. The card for the board
is a SanDisk, marked Class 10 and A1.

Received: soldering iron, solder wire, multimeter (2026-11-13); side cutter, 5 V fan (by 2026-11-15).
All ordered items have arrived.

## Yocto build (`yocto/`)

One build directory for both images: ST's `st-image-weston` (rebuilt from source, without changes of
mine, the reference for the build setup) and my own `vibration-image`, which reuses its layers and
caches.

| Item | Value |
|---|---|
| Layers | ST manifest `openstlinux-6.6-yocto-scarthgap-mpu-v26.06.10` (github.com/STMicroelectronics/oe-manifest), 7 repositories cloned by hand into `yocto/layers/` at the pinned commits (OE-Core = Yocto 5.0.17) |
| Build directory | `yocto/build-openstlinuxweston-stm32mp21-disco/` |
| MACHINE / DISTRO / images | `stm32mp21-disco` (A35-TD, kernel device tree `stm32mp215f-dk`) / `openstlinux-weston` / `st-image-weston`, `vibration-image` |
| Caches | `yocto/oe-downloads/`, `yocto/oe-sstate-cache/` (outside the build directory, shared by future builds) |
| Flash layout | `FlashLayout_sdcard_stm32mp215f-dk-optee.tsv`, one per image in `flashlayout_<image>/` (ST's Starter Package used MACHINE `stm32mp2` and the `ca35tdcid-ostl` device tree instead). Both images write a `.raw` with the same name to the same folder. For `vibration-image`: `SDCARD_SIZE=768 ./scripts/create_sdcard_from_flashlayout.sh flashlayout_vibration-image/optee/FlashLayout_sdcard_stm32mp215f-dk-optee.tsv` (768 MB `.raw`) |

My image, `vibration-image` (`meta-vibration-node/recipes-images/images/vibration-image.bb`):
- `inherit core-image` with its default package list replaced: `packagegroup-core-boot` plus
  `packagegroup-optee-core`, `vibration-hello`, `e2fsprogs-e2fsck`, `kmod`, and the
  `ssh-server-dropbear` feature. `packagegroup-base-extended` (audio, Bluetooth, Wi-Fi, mDNS) is not
  installed.
- 826 packages (st-image-weston: 2170), rootfs filesystem 112.7 MB with 22 MB free (st-image-weston:
  about 770 MB). No package manager on the board (no `package-management` feature).
- `PACKAGE_EXCLUDE`: `busybox-syslog` (the journal already collects the logs) and ST's example
  firmware `m33projects-stm32mp2`, `m0projects-stm32mp2` (recommended by the machine features
  `m33copro`/`m0copro`). `BAD_RECOMMENDATIONS` does not work with `.deb` packages.
- Masked systemd units (linked to `/dev/null`): `getty@tty1` (no screen) and `systemd-boot-update`
  (no EFI boot).
- Hostname `vibration-node`, set at build time in `base-files` (ST's `st-hostname` service raced with
  the Ethernet driver once the initrd was gone, see ADR-0001).
- Workarounds for ST's partition class (`image_types-stsplitpartitions`):
  - `IMAGE_ROOTFS_EXTRA_SPACE = "8192"`: ST sizes each filesystem as content x 1.3 with no minimum, so
    a small userfs did not fit ext4's journal ("Could not allocate block").
  - `export splitted_directory`: ST's class runs `du` on this unset variable, which scans the whole
    build directory (44 min instead of 32 s). The result is overwritten anyway.
  - The partition sizes must use ST's exact variable names, `${...}` included: with the label written
    out, ST's name expands to the same key at the end of parsing and replaces mine.

Boot chain and kernel changes (in `meta-vibration-node`):
- U-Boot: `CONFIG_BOOTDELAY=0` (config fragment) and a single extlinux entry without splash, so no
  boot menu and no wait.
- No initrd: no `INITRD` line in `extlinux.conf`, `st-initrd` not installed. The kernel mounts the
  rootfs itself (ADR-0001). The filesystems are no longer grown on the first boot.
- Kernel configuration fragment `fragment-vibration-drop.cfg`: camera, display, touchscreen,
  backlight, framebuffer, audio, Wi-Fi, Bluetooth, USB and its charger detection, debug tracing, UIO,
  PCI, network filesystems, virtualization, huge pages, other vendors' SD controllers, quotas and
  JFFS2 switched off (ADR-0002). 17 modules loaded at boot (70 before), 1702 built-in options (2065).
- Device tree patch `0001-stm32mp215f-dk-disable-unused-hardware.patch`: the nodes for that hardware
  are `disabled`, I2C2 included (ADR-0003).

Partition layout on the SD card (`.tsv`, sizes set in the image recipe):

| Partition | Size | Content |
|---|---|---|
| fsbla1/2, metadata1/2, fip-a/b, u-boot-env | about 9.5 MB | TF-A, firmware update metadata, OP-TEE + U-Boot (FIP), U-Boot environment |
| bootfs | 64 MB | kernel `Image.gz` (8.5 MB), device tree, `extlinux.conf` |
| vendorfs | 32 MB (ST: 250 MB) | empty |
| rootfs | 512 MB (ST: 4 GB) | the root filesystem, 112.7 MB |
| userfs | the rest of the 768 MB `.raw`, about 150 MB | empty |

Boot time, TF-A to login (`tools/boot_time.py`, normal boots, logs in `captures/`):

| Stage | ST Starter Package | st-image-weston rebuilt | vibration-image | vibration-image trimmed |
|---|---|---|---|---|
| TF-A BL2 | 0.60 s | 0.62 s | 0.59 s | 0.59 s |
| BL31 + OP-TEE | 0.77 s | 0.84 s | 0.83 s | 0.83 s |
| U-Boot | 5.54 s | 5.55 s | 5.52 s | 1.97 s |
| Kernel | 2.74 s | 3.00 s | 3.02 s | 2.03 s |
| initrd + rootfs (trimmed: no initrd) | 6.39 s | 6.77 s | 6.77 s | 0.79 s |
| systemd to login | 21.4 s | 21.4 s | 12.2 s | 8.78 s |
| **Total** | **37.5 s** | **38.2 s** | **29.0 s** | **15.0 s** |

First boots: 58.6 s (st-image-weston rebuilt), 51.4 s (vibration-image, the initrd grew the
filesystems once), 15.4 s (trimmed). The trimming steps, each measured on the board: U-Boot delay
and menu plus `packagegroup-base` 29.0 to 25.2 s, no initrd 25.2 to 18.0 s, kernel drivers (three
steps) 18.0 to 16.0 s, device tree 16.0 to 15.2 s, systemd services 15.2 to 15.0 s.

With `quiet` on the kernel command line, the trimmed image boots in 13.2 s (2.0 s less: the serial
console at 115200 baud no longer prints about 500 lines). Kept verbose during development; planned
for the product build as a change of the extlinux `APPEND` line.

Changes to `conf/local.conf` (it is not in git):
- `ACCEPT_EULA_stm32mp21-disco = "1"` (ST SLA0048).
- `BB_NUMBER_THREADS = "6"`, `PARALLEL_MAKE = "-j 6"`: 2 of 8 cores left free, less RAM.
- `rm_work` disabled: it deleted bison-native's work directory, which bison's built-in m4 path points
  into (glibc then failed with "m4 subprocess failed").
- `PARALLEL_MAKEINST:pn-openssl = "-j 1"` (and `openssl-native`): OpenSSL's `make install` rebuilt
  its libraries in parallel and failed with undefined references.

Build host: WSL's clock drifted and jumped backwards under load ("Clock skew detected" in meson,
probably also the cause of the OpenSSL rebuild). `chrony` replaced `systemd-timesyncd` so the clock
is corrected by slewing instead of jumping.

## Board facts: STM32MP215F-DK

Platform: 1x Cortex-A35 at 1.5 GHz, Cortex-M33 at 300 MHz, 2 GB LPDDR4, no NPU, no eMMC, 100 Mbit
Ethernet, USB-C power, M.2 E-Key slot, 40-pin GPIO expansion header.

Observed on the physical board (photos taken 2026-11-11) and in ST documentation:

| Item | Fact | Source |
|---|---|---|
| Boot storage | microSD slot, silkscreen "SD CARD", reference CN7, 8 contacts. A microSD card (11 mm wide) is smaller than the socket shell; this is normal | photo, ST hardware description wiki |
| microSD interface | SDMMC1, 4 data bits, High Speed mode at 50 MHz and 3.3 V (about 25 MB/s at most). UHS-I is not supported, so this is the board's maximum. Measured on the board: `/sys/kernel/debug/mmc0/ios` shows `clock: 50000000 Hz`, `timing spec: sd high-speed` | UM3571 Rev 1, p. 17, section 8.2.2 |
| Boot mode switches | SW2 = BOOT0, BOOT1; SW1 = BOOT2, BOOT3. ON = 1 (pin tied to VDDIO through 1 kOhm). microSD boot (A35): BOOT0 ON, others OFF (0b0001). All OFF (0b0000) = UART/USB flash programming. Read only at reset. Valid with the default boot OTP (0b00). Current position: all OFF | UM3571 Rev 1, p. 13, Tables 4-5; schematic B-02, sheet 14; DS14556 Rev 2, p. 14, Table 5 |
| STLINK | CN5: STDC14 debug connector, 14 pins in 2 rows of 7. Carries SWD/JTAG and the A35 console (USART2). Pitch about half of CN12's (by eye), consistent with STDC14 | UM3571 Rev 1, p. 2; MB2059 schematic B-02, sheet 3; counted on the board |
| Display | FFC connector CN8, silkscreen "DISPLAY" | photo, ST wiki |
| Ethernet | RJ45, reference CN11 | photo |
| GPIO expansion | 40-pin header CN12: 3V3 on pins 1, 17; 5V on pins 2, 4; SPI1 on pins 19 (MOSI, PI5), 21 (MISO, PD1), 23 (SCK, PD11), 24 (NSS, PD2). I/O at 3.3 V (VDDIO). Errata in UM3571: pin 16 (SDMMC3.CMD) is PD12, not PH8 (PH8 is the M33 console RX on CN1). Comparison with Raspberry Pi: see open items | UM3571 Rev 1, p. 26, Table 17; schematic B-02, sheet 3 |
| Consoles | CN2 "VCPA35": Cortex-A35 console, USART2 (PA4 TX, PA8 RX). CN1 "VCPM33": Cortex-M33 console, UART4 (PH7 TX, PH8 RX). 3-pin headers, pin 1 = board RX, pin 2 = GND, pin 3 = board TX. A35 console: 115200 baud, Linux device ttySTM0 | UM3571 Rev 1, p. 9, p. 28, p. 30; schematic sheet 3; `captures/st-starter_first-boot_untimed.log` line 207 |
| Silicon revision | The chip reports Rev.Z (UM3571 lists revision "Y"). ES0628 errata must be read for Rev.Z | `captures/st-starter_first-boot_untimed.log` lines 1, 91; UM3571 Rev 1, p. 33 |
| Power and USB | CN10 is the only USB connector: USB Type-C, 5 V power input and USB 2.0 data (device). Its CC1/CC2 lines go to ADC inputs ANA0/ANA1 to detect the current the power source offers (0.5, 1.5 or 3 A). Also used for flashing with STM32CubeProgrammer (UART/USB boot switch position). Checked on the board | UM3571 Rev 1, p. 11, section 7.4.2; p. 16, section 8.1 |

## Interfaces (planned)

| Link | Between | Notes |
|---|---|---|
| SPI + DMA | ADXL345 <-> M33 | via the 40-pin header / HAT cable, fixed sample rate. Pin mapping not defined yet (see unverified items) |
| RPMsg over remoteproc | M33 <-> A35 | the A35 loads the M33 firmware and receives the FFT spectrum |
| MQTT over TLS | A35 -> MQTT broker | alerts when the spectrum deviates from the learned healthy spectrum |
| 40-pin header | board <-> HAT | HAT: current-measurement chip (INA219-type), EEPROM, LED, button, sensor connector |
| Power + console | rig <-> board | relay on the 5 V DC line only (never mains), USB-UART console |
| Logic analyzer | Pico <-> SPI lines | verifies equal sample spacing and measures jitter |

## Open items and unverified facts

1. Secure boot flow on the MP21 (TF-A, OP-TEE, signed FIT): not verified. Required before phase 4.
2. CN12 40-pin pinout compared to Raspberry Pi: not verified. Required before the HAT PCB order
   (end of week 4).
3. RPMsg from Zephyr on this board: not verified. Test on the first day of phase 3.
4. Bus for the HAT's current sensor and EEPROM: no I2C bus reaches CN12. Pins 3 and 5 carry I3C1
   (PA2, PG13), pins 27 and 28 I3C2 (UM3571 Rev 1, p. 26, Table 17), and I2C2 only goes to the
   camera connector and the touch panel (p. 21, Table 12). To check in DS14556 before the HAT design
   (phase 2): I2C as an alternate function of these pins, or I3C driving I2C devices.
5. Every small systemd job takes 0.3-0.8 s during boot (even mounting a tmpfs), so systemd still takes
   8.8 s. Not verified why; reading programs from the SD card (50 MHz, the board's maximum) is
   suspected.
6. ST's `stm32mp-calibration.timer` recalibrates the internal oscillators HSI/CSI every few seconds.
   What depends on these oscillators on this board is not checked (RM0506).
7. `stm32_lptimer` is kept: whether the built-in clock driver `CLKSRC_STM32_LP` needs it is not
   verified.
8. Every boot logs `stm32-rproc 0.m33: ... could not locate channel named "detach"`. To look at with
   remoteproc in phase 3.

## Decisions

ADRs in [decisions/](decisions/), using `decisions/adr-template.md`:
- [ADR-0001](decisions/0001-boot-without-initrd.md): boot without an initrd.
- [ADR-0002](decisions/0002-kernel-drivers-keep-drop.md): switch off kernel drivers for hardware the
  node does not use.
- [ADR-0003](decisions/0003-device-tree-disable-unused-hardware.md): disable unused hardware in the
  device tree with a patch.

Decisions expected: accelerometer choice (phase 2), boot chain and signing (phase 4), fuses and
secure boot lock (end of week 7).
