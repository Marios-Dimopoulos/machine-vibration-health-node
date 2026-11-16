SUMMARY = "Vibration health node image"
DESCRIPTION = "Minimal image for the STM32MP215F-DK: only what the vibration node needs. \
Packages are added when a phase needs them, not by default."
LICENSE = "MIT"

inherit core-image

# Remote access for development and, later, the test rig
IMAGE_FEATURES += "ssh-server-dropbear"

# Install only what we list, instead of core-image's default
# (packagegroup-core-boot + packagegroup-base-extended).
# packagegroup-base-extended brings audio, Bluetooth, Wi-Fi and mDNS (avahi),
# because ST's machine and distro features enable them.
IMAGE_INSTALL = "packagegroup-core-boot ${CORE_IMAGE_EXTRA_INSTALL}"

#   packagegroup-optee-core Linux side of OP-TEE (TEE client), needed for secure storage later
#   vibration-hello         our first program
#   e2fsprogs-e2fsck        checks the ext4 partitions at boot (after a power cut)
#   kmod                    loads kernel modules (modprobe)
# No st-initrd: we boot without an initrd (see stm32mp-extlinux.bbappend), so the filesystems
# keep their build-time sizes and are not grown to fill their partitions on the first boot.
CORE_IMAGE_EXTRA_INSTALL += " \
    packagegroup-optee-core \
    vibration-hello \
    e2fsprogs-e2fsck \
    kmod \
"

# Never install ST's example firmware: recommended by the machine features m33copro/m0copro,
# not needed by us (the MP21 has no Cortex-M0). Our own M33 firmware comes in phase 3.
# BAD_RECOMMENDATIONS would be the usual tool, but it does not work with .deb packages.
PACKAGE_EXCLUDE += "m33projects-stm32mp2 m0projects-stm32mp2"

# ST's partition sizing has no minimum: give every partition 8 MB of free room,
# so small partitions (userfs) still fit ext4's journal and metadata.
IMAGE_ROOTFS_EXTRA_SPACE = "8192"

# ST's image_types-stsplitpartitions uses ${splitted_directory} without setting it, so "du"
# scans the whole build directory (about an hour). Its result is overwritten anyway.
export splitted_directory = "${WORKDIR}/rootfs"
