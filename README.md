# machine-vibration-health-node

A secure edge vibration monitor for predictive maintenance, built on the STM32MP215F-DK.

The Cortex-M33 (Zephyr) samples an ADXL345 accelerometer and computes an FFT of the vibration.
The Cortex-A35 (Linux) receives the spectrum over RPMsg, detects anomalies and publishes alerts
over MQTT/TLS. Underneath: secure boot, A/B updates with RAUC, a read-only dm-verity rootfs,
a custom KiCad HAT, and a hardware-in-the-loop test rig.

This is an educational portfolio project, a work in progress.

- [Project plan](docs/plan.md): phases and the proof that ends each one
- [Architecture](docs/architecture.md): the current state of the system
- [Decisions](docs/decisions/): ADRs
