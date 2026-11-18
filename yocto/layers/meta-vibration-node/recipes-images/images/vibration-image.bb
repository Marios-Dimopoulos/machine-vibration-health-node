SUMMARY = "Vibration health node image"
DESCRIPTION = "Minimal image for the STM32MP215F-DK: only what the vibration node needs. \
Packages are added when a phase needs them, not by default."
LICENSE = "MIT"

inherit core-image

# Remote access for development and, later, the test rig
IMAGE_FEATURES += "ssh-server-dropbear"

# Install only what I list, instead of core-image's default
# (packagegroup-core-boot + packagegroup-base-extended).
# packagegroup-base-extended brings audio, Bluetooth, Wi-Fi and mDNS (avahi),
# because ST's machine and distro features enable them.
IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"

#   packagegroup-optee-core Linux side of OP-TEE (TEE client), needed for secure storage later
#   vibration-hello         my first program
#   e2fsprogs-e2fsck        checks the ext4 partitions at boot (after a power cut)
#   kmod                    loads kernel modules (modprobe)
# No st-initrd: the board boots without an initrd (see stm32mp-extlinux.bbappend), so the filesystems
# keep their build-time sizes and are not grown to fill their partitions on the first boot.
CORE_IMAGE_EXTRA_INSTALL += " \
    packagegroup-optee-core \
    vibration-hello \
    e2fsprogs-e2fsck \
    kmod \
"

# Never install ST's example firmware: recommended by the machine features m33copro/m0copro,
# not needed by me (the MP21 has no Cortex-M0). My own M33 firmware comes in phase 3.
# BAD_RECOMMENDATIONS would be the usual tool, but it does not work with .deb packages.
PACKAGE_EXCLUDE += "m33projects-stm32mp2 m0projects-stm32mp2"

# No busybox syslogd/klogd (recommended by packagegroup-core-boot): systemd's journal already
# collects the kernel and program logs, so they only duplicated it.
PACKAGE_EXCLUDE += "busybox-syslog"

# Mask systemd units that have no job on this board. Masking links the unit to /dev/null in /etc,
# so systemd never starts it, even if another unit asks for it.
#   systemd-boot-update  updates systemd-boot on an EFI system partition (this board boots with U-Boot)
#   getty@tty1           login prompt on a screen's virtual terminal; the console is the serial
#                        port (serial-getty@ttySTM0)
ROOTFS_POSTPROCESS_COMMAND += "vibration_mask_units; "
vibration_mask_units() {
    ln -sf /dev/null ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/systemd-boot-update.service
    ln -sf /dev/null ${IMAGE_ROOTFS}${sysconfdir}/systemd/system/getty@tty1.service
}

# Partition sizes on the SD card, in KB. ST's defaults are for its demo images (rootfs 4 GB,
# vendorfs 250 MB), which made a 5 GB .raw that is mostly empty. These values go into the .tsv.
# The names must match ST's exactly, ${...} included (st-machine-flashlayout-stm32mp.inc): with
# "rootfs" written out, ST's name expands to the same key at the end of parsing and replaces mine.
FLASHLAYOUT_PARTITION_SIZE:sdcard:${STM32MP_ROOTFS_LABEL} = "524288"
FLASHLAYOUT_PARTITION_SIZE:sdcard:${STM32MP_VENDORFS_LABEL}:stm32mp2common = "32768"
# Largest filesystem image the build accepts in each partition (same sizes), so a too big
# rootfs fails at build time instead of when the .raw is created.
STM32MP_ROOTFS_SIZE = "524288"
STM32MP_VENDORFS_SIZE = "32768"
# userfs is the last partition: ST's script stretches it to the end of the .raw, whose size is
# set when the script runs: SDCARD_SIZE=768 ./scripts/create_sdcard_from_flashlayout.sh <tsv>

# ST's partition sizing has no minimum: give every partition 8 MB of free room,
# so small partitions (userfs) still fit ext4's journal and metadata.
IMAGE_ROOTFS_EXTRA_SPACE = "8192"

# ST's image_types-stsplitpartitions uses ${splitted_directory} without setting it, so "du"
# scans the whole build directory (about an hour). Its result is overwritten anyway.
export splitted_directory = "${WORKDIR}/rootfs"
