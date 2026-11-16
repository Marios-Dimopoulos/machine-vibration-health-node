# Look for our files (the config fragment) in u-boot-stm32mp/ next to this append, before ST's.
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Every .cfg file in SRC_URI is merged into U-Boot's configuration (OE-Core u-boot-configure.inc).
SRC_URI += "file://fragment-bootdelay.cfg"
