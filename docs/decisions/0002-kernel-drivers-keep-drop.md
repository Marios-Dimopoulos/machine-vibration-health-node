# ADR-0002: Switch off kernel drivers for hardware the node does not use

**Status:** Accepted
**Date:** 2026-11-17
**Phase:** 1

## Context

ST's kernel recipe (`meta-st-stm32mp/recipes-kernel/linux/linux-stm32mp_6.6.bb`) starts from Linux's
generic arm64 `defconfig` and merges ST's fragments on top. The result had 2065 options built in
(`=y`) and 933 modules (`=m`), about 1054 `kernel-module-*` packages in my image.

The board's device tree (`stm32mp215f-dk.dts`) declares the DK's optional hardware with
`status = "okay"`: camera (IMX335 on `i2c2`, DCMIPP, CSI), touchscreen (Goodix on `i2c2`), display
and backlight, digital microphones, and the USB-C port. So at every boot udev loaded and probed 70
modules (`lsmod`), including drivers for hardware that is not connected
(`imx335 0-001a: failed to find sensor: -6`, `Goodix-TS 0-005d: I2C communication failure: -6`).

What the vibration node needs from Linux:
- Ethernet: network, SSH, MQTT (phase 7).
- remoteproc for the M33 (phase 3).
- I2C controller driver: the HAT's current sensor and EEPROM (phase 7). The PMIC is not on a Linux
  I2C bus: its regulators are reached through OP-TEE over SCMI (`VOLTD_SCMI_STPMIC_*` in the device
  tree).
- Crypto, CRC, OP-TEE, the watchdog (phases 4 and 5). OP-TEE and the watchdog are built in.
- The SoC temperature/voltage sensor (`mr75203`), the firewall (`ip_tables`) and IPv6.

The board's only USB connector, CN10, is used for power. The network is Ethernet and the console is
CN2 (UM3571 Rev 1, p. 11, section 7.4.2 and p. 16, section 8.1).

## Options considered

### Option A: Keep ST's configuration
- How it works: no change.
- Pros: same kernel as ST's reference image, nothing to maintain.
- Cons: every boot loads and probes drivers for hardware that is not there. Hundreds of modules in
  the image, and more kernel code to track for security fixes.

### Option B: A config fragment that switches off whole subsystems
- How it works: my layer adds `fragment-vibration-drop.cfg` to ST's `KERNEL_CONFIG_FRAGMENTS`
  (`linux-stm32mp_%.bbappend`). It is merged last, so its `# CONFIG_X is not set` lines win.
  Switching off a menu (`CONFIG_MEDIA_SUPPORT`, `CONFIG_SOUND`, `CONFIG_USB_SUPPORT` ...) removes
  every driver under it.
- Pros: about 25 lines, readable, in git. ST's configuration is still the base, so a new ST release
  only needs my fragment applied again.
- Cons: Kconfig can turn an option back on through `select`, and ST's `merge_config.sh -m` does not
  warn. The final `.config` has to be checked after each build.

### Option C: My own full kernel configuration
- How it works: start from a minimal configuration and enable only what I need
  (`KERNEL_EXTERNAL_DEFCONFIG`).
- Pros: the smallest possible kernel, full control.
- Cons: thousands of options to own, and every ST update has to be merged by hand. A wrong choice can
  stop the board from booting, and the cause is hard to find.

## Decision

Option B. It removes the hardware I know I do not have with a short, reviewable file, and keeps
ST's tested configuration as the base, which matters while I am still learning the board.

Switched off: camera and video (`MEDIA_SUPPORT`), display (`DRM`), touchscreen, backlight, audio
(`SOUND`, `STM32_MDF_*`), Wi-Fi and Bluetooth (`WLAN`, `CFG80211`, `MAC80211`, `BT`, `RFKILL`),
USB and the CN10 charger detection (`USB_SUPPORT`, `PHY_STM32_USB2PHY`, `STM32_ADC*`,
`IIO_RESCALE`, the IIO timer triggers), debug tracing and DDR counters (`CORESIGHT`, `STM`,
`STM32_DDR_PMU`), the debug bus (`STM32_DBG_BUS`) and UIO (`UIO`).

Also switched off, built in by the generic `defconfig` but not usable or not used here (measured with
`initcall_debug`: about 0.2 s of kernel boot): PCI (the STM32MP21 has no PCIe, DS14556 Rev 2), the
network filesystems (`NFS_FS`, `SUNRPC`, `NET_9P`, `9P_FS`), virtualization (`VIRTUALIZATION`,
`VFIO`), huge pages (`HUGETLBFS`), other vendors' SD controllers (`MMC_SDHCI`, `MMC_DW`), disk quotas
(`QUOTA`) and JFFS2. Kept on purpose: `loop` and `squashfs` (RAUC bundles, phase 5) and the jitter
entropy random source (crypto).

## Consequences

- Easier: 70 loaded modules down to 22, packages 1164 to 850, rootfs about 142 to 114 MB, boot
  18.0 to 16.7 s (`tools/boot_time.py`). No more failed probes in the boot log.
- Harder: no USB at all in Linux (no USB network or console as a debug path), and no camera,
  display or audio without changing the fragment. Flashing is not affected: it runs in the Boot
  ROM and U-Boot.
- After each build, check that no switched-off option is back in the final `.config`.
- Later steps: loaded modules 22 to 19 (debug bus, UIO), then built-in options 1951 to 1702 and
  boot 16.6 to 16.0 s (PCI, network filesystems and the rest).
- If the HAT or a later phase needs a removed subsystem (for example an ADC), remove its line from
  the fragment.

## Interview angle

A vendor kernel supports every board the vendor sells; I kept the vendor's configuration as the
base and switched off, in one small fragment, the subsystems for hardware my product does not
have, then verified the result in the final `.config` and on the board.
