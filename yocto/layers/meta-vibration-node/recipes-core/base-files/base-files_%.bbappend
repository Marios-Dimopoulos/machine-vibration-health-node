# Fixed hostname, written to /etc/hostname at build time (the default is ${MACHINE}).
# There is only one board, so it does not need ST's st-hostname service, which appends the MAC
# address at boot and raced with the Ethernet driver ("unknown" in the name).
hostname = "vibration-node"
