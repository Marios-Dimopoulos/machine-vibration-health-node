# Look for my files (the config fragment) in linux-stm32mp/ next to this append.
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://fragment-vibration-drop.cfg"

# ST's recipe merges every file listed here into the kernel .config, in order (linux-stm32mp.inc).
# A bbappend is parsed after the recipe, so mine comes last and wins over ST's fragments.
KERNEL_CONFIG_FRAGMENTS:append = " ${WORKDIR}/fragment-vibration-drop.cfg"
