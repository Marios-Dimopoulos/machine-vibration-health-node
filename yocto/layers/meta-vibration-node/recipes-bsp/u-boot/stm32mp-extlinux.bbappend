# Boot without U-Boot's extlinux menu (saves 2 s per boot).
# U-Boot only asks the user, and waits TIMEOUT, when extlinux.conf contains a "menu" line.
# ST's class writes one for more than one boot entry, and one for a splash background.

# Keep only the normal entry: drop the M.2 Wi-Fi/Bluetooth entry (no M.2 card on this board).
UBOOT_EXTLINUX_TARGETS_EXTRA_CONFIG[stm32mp215f-dk] = ""

# No menu background picture: it is drawn on a display, and I have none.
# Only this recipe's copy of MACHINE_FEATURES changes; the machine keeps the feature.
MACHINE_FEATURES:remove = "splashscreen"

# No INITRD line: the kernel mounts the rootfs itself (SD card and ext4 drivers are built in).
# ST's initrd loads and probes every driver before systemd starts, and systemd does it again.
UBOOT_EXTLINUX_INITRD = ""
