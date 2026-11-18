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

## Building the Linux image

How I build `vibration-image` for the A35, with Yocto (scarthgap) and ST's OpenSTLinux layers.

### Build host

- Ubuntu 24.04 in WSL2, with the project in the Linux filesystem (`~/...`), never under `/mnt/c`:
  Yocto needs a case-sensitive, fast filesystem.
- The host packages listed in the Yocto Project 5.0 manual ("Required Packages for the Build Host").
- `chrony` instead of `systemd-timesyncd`: WSL's clock drifted and jumped backwards under load
  ("Clock skew detected" in meson, probably also the cause of a failed OpenSSL build). chrony
  corrects it by slewing instead of jumping.

### 1. Get the layers

The layers are those of ST's manifest `openstlinux-6.6-yocto-scarthgap-mpu-v26.06.10`
(github.com/STMicroelectronics/oe-manifest), cloned by hand at its pinned commits. They live in
`yocto/layers/`, which git ignores, next to my layer `meta-vibration-node` (in this repository).

| Repository | Directory in `yocto/layers/` | Commit |
|---|---|---|
| https://git.openembedded.org/openembedded-core | `openembedded-core` | `52380df998b3a8fe6a091f8547434a3231320a8e` (yocto-5.0.17) |
| https://git.openembedded.org/bitbake | `openembedded-core/bitbake` | `b2404004135b669f8258c85c7b5aed4570a805c7` |
| https://git.openembedded.org/meta-openembedded | `meta-openembedded` | `5124ac4a658899158f4a7a2ddf1d2ca931ec7d0e` |
| https://github.com/STMicroelectronics/meta-st-stm32mp | `meta-st/meta-st-stm32mp` | `49046b2a0ad4dc29117025c94838b9befff86f23` |
| https://github.com/STMicroelectronics/meta-st-openstlinux | `meta-st/meta-st-openstlinux` | `b0316f706bcf34e8449d43aba3c630030f2cc366` |
| https://github.com/STMicroelectronics/meta-st-stm32mp-addons | `meta-st/meta-st-stm32mp-addons` | `ad667afe266eb1ca706bf92ea6d99cc9ec74d662` |
| https://github.com/STMicroelectronics/meta-st-scripts | `meta-st/scripts` | `8826802b453ef2ead61b07c7bc5ab525a405df04` |

For each one, from `yocto/`:

```
git clone <repository> layers/<directory>
git -C layers/<directory> checkout <commit>
```

Clone `openembedded-core` before `bitbake`, since bitbake goes inside it.

### 2. Create the build directory

From `yocto/`, with ST's configuration templates:

```
TEMPLATECONF=$PWD/layers/meta-st/meta-st-openstlinux/conf/templates/default \
    source layers/openembedded-core/oe-init-build-env build-openstlinuxweston-stm32mp21-disco
```

This creates `conf/local.conf` and `conf/bblayers.conf` and moves into the build directory. In a new
shell later, the same `source` line (without `TEMPLATECONF`) sets up the environment again.

The downloads and the shared state cache go to `yocto/oe-downloads/` and `yocto/oe-sstate-cache/`
(ST's template), outside the build directory, so other build directories can reuse them.

### 3. Configure `conf/local.conf`

`local.conf` is not in git: it holds the settings of this build directory on this PC. My changes to
ST's template:

```
MACHINE = "stm32mp21-disco"
DISTRO = "openstlinux-weston"
ACCEPT_EULA_stm32mp21-disco = "1"
BB_NUMBER_THREADS = "6"
PARALLEL_MAKE = "-j 6"
PARALLEL_MAKEINST:pn-openssl = "-j 1"
PARALLEL_MAKEINST:pn-openssl-native = "-j 1"
BUILDHISTORY_COMMIT = "0"
```

and the template's `INHERIT += "rm_work"` commented out.

- `ACCEPT_EULA_...`: accepts ST's license (SLA0048) for the firmware parts of the BSP.
- `BB_NUMBER_THREADS`, `PARALLEL_MAKE`: 6 of my 8 cores, which leaves the PC usable and needs less RAM.
- `PARALLEL_MAKEINST` for OpenSSL: its `make install` rebuilt the libraries in parallel and failed
  with undefined references.
- `BUILDHISTORY_COMMIT = "0"`: the template already enables buildhistory, which records the packages
  of each image and who pulled them in (`buildhistory/images/.../depends.dot`). With `0` it only
  writes the files and does not make a git commit at every build.
- `rm_work` off: it deleted bison-native's work directory, which bison's built-in m4 path points into,
  and glibc then failed with "m4 subprocess failed".

### 4. Add the layers

From the build directory:

```
bitbake-layers add-layer ../layers/meta-openembedded/meta-oe
bitbake-layers add-layer ../layers/meta-openembedded/meta-python
bitbake-layers add-layer ../layers/meta-vibration-node
```

ST's template already lists openembedded-core, the other meta-openembedded layers and ST's layers.

### 5. Build

```
bitbake vibration-image
```

The first build compiles everything (several hours); later builds reuse the shared state cache.

### 6. Create the SD card image

In the deploy directory, `tmp-glibc/deploy/images/stm32mp21-disco/`:

```
SDCARD_SIZE=768 ./scripts/create_sdcard_from_flashlayout.sh \
    flashlayout_vibration-image/optee/FlashLayout_sdcard_stm32mp215f-dk-optee.tsv
```

This writes `FlashLayout_sdcard_stm32mp215f-dk-optee.raw` (768 MB) next to the script.
`SDCARD_SIZE` sets the size of the `.raw`; without it, ST's script makes it 4915 MB. Every image
writes a `.raw` with this same name, so check its time before using it.

### 7. Write and boot

Write the `.raw` to a microSD card (I use Raspberry Pi Imager on Windows, "Use custom"). Check the
target disk twice: writing to the wrong disk destroys its data.

Boot switches for microSD boot: BOOT0 ON, BOOT1-3 OFF (UM3571 Rev 1, p. 13). The A35 console is on
CN2 at 115200 baud. The board logs in as root automatically on the console (a development setting)
and is reachable over SSH on Ethernet (by its IP address; its hostname is `vibration-node`).
