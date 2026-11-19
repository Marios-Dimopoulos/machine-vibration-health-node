# ADR-0001: Boot without an initrd

**Status:** Accepted
**Date:** 2026-11-16
**Phase:** 1

## Context

ST's image boots through an initrd, `st-image-resize-initrd`, which U-Boot loads next to the kernel
(`INITRD` line in `extlinux.conf`). It has two jobs:
- On the first boot only, grow each ext4 filesystem (`resize2fs`) to fill its partition
  (`init-resize.sh`, skipped once `/etc/.resized` exists). The partition sizes themselves are fixed
  by ST's flash layout (rootfs partition 4 GiB, rootfs filesystem about 172 MB).
- On every boot, mount the rootfs, start udev, load the modules for every device and wait for all
  probes to finish (`init-rootfs_modules.sh`: `udevadm trigger --action=add` and `udevadm settle`).

Sources: `meta-st-openstlinux/recipes-st/images/st-image-resize-initrd.bb` and
`meta-st-openstlinux/recipes-st/initrd/st-initrd/` (meta-st, OpenSTLinux v26.06.10).

In the boot log, the initrd starts at 2.15 s (kernel time) and systemd only starts at 8.58 s. In
between, the initrd probes every device, including hardware I do not have (camera, touchscreen,
display). Then systemd runs `systemd-udev-trigger` and processes every device a second time. The
"initrd + rootfs" stage is 6.7 s of the 25.2 s boot (`tools/boot_time.py`, trim-1 boots).

My kernel can mount the rootfs without help: the SD card driver and ext4 are built in
(`CONFIG_MMC_STM32_SDMMC=y`, `CONFIG_EXT4_FS=y` in the kernel `.config`), and the kernel command
line already has `root=PARTUUID=... rootwait`.

## Options considered

### Option A: Keep ST's initrd
- How it works: no change.
- Pros: the filesystems grow to fill their partitions on the first boot. Same as ST's reference image.
- Cons: all devices are probed twice on every boot, and systemd starts about 6 s late. More code in
  the boot path that I do not need.

### Option B: No initrd
- How it works: `UBOOT_EXTLINUX_INITRD = ""` removes the `INITRD` line from `extlinux.conf`, and the
  image no longer installs `st-initrd` into bootfs. The kernel mounts the rootfs itself.
- Pros: one less boot stage, each device probed once (by systemd's udev), simpler boot chain.
- Cons: no automatic resize. The filesystems keep their build-time sizes (ST's content x 1.3 plus
  my 8 MB `IMAGE_ROOTFS_EXTRA_SPACE`: rootfs 148 MB, 26 MB free), and the rest of each partition
  stays unused.

### Option C: My own minimal initrd
- How it works: a small initrd of my own that does only what I need.
- Pros: room for later jobs that need to run before the rootfs is mounted.
- Cons: nothing needs it today, so it is extra work and boot time for no gain.

## Decision

Option B. The kernel already has everything it needs to mount the rootfs, and the only job of the
initrd that I would lose, the first-boot resize, does not need an initrd: the filesystem size can be
set at build time, or grown by systemd (`x-systemd.growfs`), once the A/B layout of phase 5 fixes the
partition sizes.

## Consequences

- Easier: a shorter and simpler boot chain, with devices probed once, by systemd's udev.
- Harder: the filesystems do not grow. Until phase 5, the rootfs has only about 26 MB free.
- The first boot is no longer longer than the others (no resize step).
- Drivers now load later, from systemd's udev, which exposed a race in ST's `st-hostname` service:
  it reads the MAC address of `end0` after `network.target`, which does not wait for the interface,
  so the Ethernet driver was not loaded yet and the hostname became `stm32mp21-discounknownunknown`.
  The initrd had hidden this by loading every driver before systemd started. Fix: `st-hostname`
  removed, fixed hostname `vibration-node` set in `base-files` (one board only).

## Interview angle

The initrd only exists to help the kernel reach its root filesystem; my kernel can do that alone, so
I removed a whole boot stage that was probing every device twice.
