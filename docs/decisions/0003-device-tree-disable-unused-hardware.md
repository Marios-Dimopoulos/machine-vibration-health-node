# ADR-0003: Disable unused hardware in the device tree with a patch

**Status:** Accepted
**Date:** 2026-11-17
**Phase:** 1

## Context

The board device tree, `stm32mp215f-dk.dts` (ST's kernel patch, OpenSTLinux v26.06.10), sets
`status = "okay"` on the DK's optional hardware: the camera interface (`csi`, `dcmipp`), the I2C2
bus with the IMX335 camera and the Goodix touchscreen, the display (`ltdc`, `panel-rgb`,
`panel-backlight`), the microphones (`mdf1`, `dmic-0`, `dmic-1`, `sound`), the USB port and its
charger detection (`usbotg_hs`, `usb2_phy2`, `typec`, `voltage-divider-cc1/2`, `adc_1`), and debug
tracing (`dbg_bus`, the `cs_*` nodes, `ddrperfm`).

ADR-0002 switched off the drivers for this hardware, but the device tree still declared it. So the
kernel still created those devices, and the description did not match the product.

I2C2 only goes to the camera connector CN4 and the display's touch panel (UM3571 Rev 1, p. 21,
Table 12). No I2C bus reaches the 40-pin header CN12: pins 3 and 5 carry I3C1, pins 27 and 28 I3C2
(UM3571 Rev 1, p. 26, Table 17). So disabling I2C2 does not take anything from the HAT.

## Options considered

### Option A: Leave the device tree as it is
- How it works: no change. The drivers are already gone (ADR-0002).
- Pros: nothing to maintain.
- Cons: the kernel still creates devices for hardware that is not there, and the device tree
  describes a different product.

### Option B: A patch to the board device tree
- How it works: `0001-stm32mp215f-dk-disable-unused-hardware.patch` in my layer, added to
  `SRC_URI` of ST's kernel recipe. It appends `&node { status = "disabled"; };` blocks at the end of
  `stm32mp215f-dk.dts`. A later definition of a property wins, so ST's lines stay untouched.
- Pros: one final `.dtb` with the same name, so U-Boot and `extlinux.conf` need no change. The
  patch only adds lines and is easy to review.
- Cons: if ST changes the end of the file, the patch fails to apply (`Hunk FAILED`) and has to be
  refreshed.

### Option C: A device tree overlay
- How it works: a separate `.dtbo` that U-Boot merges onto the base `.dtb` at every boot.
- Pros: the base `.dtb` stays ST's, and overlays can be switched without rebuilding the kernel.
- Cons: needs U-Boot and `extlinux.conf` set up to apply it, and a base built with symbols. Overlays
  are meant for optional hardware, and this change is permanent.

## Decision

Option B. Removing the camera, display, audio and USB is a permanent property of the product, and
a patch changes the base device tree with no extra boot setup. Overlays fit hardware that may or
may not be present, like the HAT later.

## Consequences

- Easier: the device tree describes the real product. Loaded modules 19 to 17 (the I2C2 driver no
  longer loads), boot 16.0 to 15.2 s (kernel 2.40 to 2.01 s), measured with `tools/boot_time.py`.
  The kernel time dropped more than expected, probably because devices with no driver no longer
  wait in the deferred probe list.
- Harder: an ST update that changes `stm32mp215f-dk.dts` can break the patch.
- Using any of this hardware again means removing its block from the patch and its line from the
  kernel fragment (ADR-0002).
- Follow-up: the HAT's bus (open item 4 in `architecture.md`). The HAT itself can be described with
  an overlay in phase 7.

## Interview angle

The vendor's device tree describes the evaluation kit with all its options; I patched it to describe
my product, so the kernel only creates devices that really exist, which also cut 0.8 s of boot.
