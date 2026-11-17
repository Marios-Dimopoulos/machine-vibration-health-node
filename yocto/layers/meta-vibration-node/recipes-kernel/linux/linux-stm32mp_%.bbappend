# Look for my files (the config fragment and the device tree patch) in linux-stm32mp/ next to this append.
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://fragment-vibration-drop.cfg"

# Device tree patch: disables the board's nodes for hardware I do not use.
# Every .patch in SRC_URI is applied to the kernel source in do_patch, after ST's own patch.
SRC_URI += "file://0001-stm32mp215f-dk-disable-unused-hardware.patch"

# ST's recipe merges every file listed here into the kernel .config, in order (linux-stm32mp.inc).
# A bbappend is parsed after the recipe, so mine comes last and wins over ST's fragments.
KERNEL_CONFIG_FRAGMENTS:append = " ${WORKDIR}/fragment-vibration-drop.cfg"
