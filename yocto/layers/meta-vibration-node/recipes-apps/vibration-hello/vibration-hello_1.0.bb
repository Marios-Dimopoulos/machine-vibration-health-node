SUMMARY = "First program of meta-vibration-node"
DESCRIPTION = "Prints the machine it runs on. Proves the layer can build and install a program."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Local source file, found in files/ next to this recipe
SRC_URI = "file://vibration-hello.c"

# In this Yocto release (scarthgap) do_unpack copies local files straight into WORKDIR; build there.
# (Newer releases use UNPACKDIR instead.)
S = "${WORKDIR}"

# No build system (no Makefile, no CMake): call the cross-compiler directly.
# ${CC} is the cross-compiler for the Cortex-A35, with its sysroot already set.
# ${LDFLAGS} matters: without it the "no GNU_HASH" QA check fails.
do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${S}/vibration-hello.c -o ${B}/vibration-hello
}

# Install into the fake root (${D}) at the path it will have on the board
do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/vibration-hello ${D}${bindir}/vibration-hello
}
