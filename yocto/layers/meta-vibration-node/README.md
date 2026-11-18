# meta-vibration-node

My Yocto layer for the vibration health node on the STM32MP215F-DK. It holds my image,
`vibration-image`, my own programs, and my changes to ST's recipes, written as appends, so ST's
layers stay untouched.

How to set up the build and use this layer: see the [project README](../../../README.md).
Why the changes are made: see the ADRs in [docs/decisions/](../../../docs/decisions/).

## Dependencies

- Yocto Project 5.0 (scarthgap): openembedded-core, meta-openembedded (meta-oe, meta-python).
- ST's OpenSTLinux layers `openstlinux-6.6-yocto-scarthgap-mpu-v26.06.10`: meta-st-stm32mp and
  meta-st-openstlinux. The appends change ST's recipes and the image uses ST's partition and
  flash layout classes, so the layer only works on top of them.
- MACHINE `stm32mp21-disco`, DISTRO `openstlinux-weston`.

## Contents

| File | What it does |
|---|---|
| `conf/layer.conf` | registers the layer with bitbake |
| `recipes-images/images/vibration-image.bb` | the image: package list, excluded packages, masked systemd units, partition sizes |
| `recipes-apps/vibration-hello/` | my first program, a C program that prints the board's hostname, machine and kernel |
| `recipes-bsp/u-boot/u-boot-stm32mp_%.bbappend` + `u-boot-stm32mp/fragment-bootdelay.cfg` | U-Boot without the autoboot delay (`CONFIG_BOOTDELAY=0`) |
| `recipes-bsp/u-boot/stm32mp-extlinux.bbappend` | `extlinux.conf` with one entry, no splash and no initrd, so U-Boot shows no menu (ADR-0001) |
| `recipes-kernel/linux/linux-stm32mp_%.bbappend` | adds my kernel configuration fragment and device tree patch to ST's kernel |
| `recipes-kernel/linux/linux-stm32mp/fragment-vibration-drop.cfg` | switches off the kernel drivers for hardware the node does not use (ADR-0002) |
| `recipes-kernel/linux/linux-stm32mp/0001-stm32mp215f-dk-disable-unused-hardware.patch` | disables that hardware in the board device tree (ADR-0003) |
| `recipes-core/base-files/base-files_%.bbappend` | fixed hostname `vibration-node` |

The appends apply to every image built with this layer, not only to `vibration-image`.

## Build

From the build directory, with the layer added (`bitbake-layers add-layer`):

```
bitbake vibration-image
```

## License

MIT, see `COPYING.MIT`.
