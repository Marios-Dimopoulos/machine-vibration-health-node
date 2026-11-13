# machine-vibration-health-node

A secure edge vibration monitor for predictive maintenance.
An M33 core (Zephyr) samples a vibration sensor (ADXL345, SPI + DMA), computes an FFT and
sends the spectrum over RPMsg to Linux. The A35 core (Linux) detects anomalies statistically
and publishes alerts over MQTT/TLS. Underneath: secure boot, A/B updates with RAUC and rollback,
a read-only dm-verity rootfs, a custom 2-layer KiCad HAT, and an automated hardware-in-the-loop
test rig that proves all of the above on the real board.

Board: STM32MP215F-DK (1x Cortex-A35 1.5 GHz, Cortex-M33 300 MHz, 2 GB LPDDR4, boot from
microSD, no eMMC, no NPU, 100 Mbit Ethernet, USB-C power, 40-pin GPIO header CN12).
Test rig: Raspberry Pi Zero 2 W + labgrid + pytest, relay on the 5 V DC line only,
USB-UART console, Raspberry Pi Pico as logic analyzer.

## Purpose of this project (most important section)

This is an educational portfolio project. Finishing it is NOT the main goal.
The main goal is that the user fully understands everything involved in developing it,
well enough to work as an embedded / Linux / hardware engineer.
- There is no deadline. Go slowly. Never rush or skip a concept to move faster.
- Prefer understanding over speed, and a correct small step over a big unexplained one.
- If a shortcut would hide something the user should learn, take the slow path and explain.

## Language

Everything is in English: the conversation with the user, README, ADRs, code, comments,
commit messages and the comprehension checks.

## How to work with the user

The user does not yet know most of what this project involves (boot chain, device tree, RAUC,
Zephyr, Yocto, secure boot). So you write the working draft, but always teach it:
1. Explain the concept first: what it is, why it exists, where it sits in the system.
2. Give a small, working, ready-to-use example with the explanation next to it.
3. For new code or config, explain each important part, not just a summary.
4. Tell the user how to verify that it worked (a command, a log line, a measurement).
5. Let the user run it, change something, and report what they observed.
Work one small step at a time and wait for the user before moving on. Do not dump large
amounts of code or many steps at once. Mention common mistakes and how to recognize them.
When useful, link a skill to why it matters for a job in embedded / Linux / hardware work.

## Comments in files

Files stay simple, plain and readable. Comment wherever a reader needs an explanation (why
something is there, a non-obvious choice, a trap to avoid), and lean towards more comments rather
than fewer, without over-doing it: no long essays, no decorative lines such as "# ----".
The full teaching goes in the conversation, not in the file.

## Rules

- Every claim about the hardware (a register, a pin, a bit, a fuse, a boot switch) is checked
  in the manual or datasheet first. Cite the document, revision and page (e.g. "UM3571 Rev 1,
  p. 13"), using the files listed in docs/datasheets/README.md. If unsure, say so, do not guess.
- Never close the chip (fuses, secure boot lock) on the only board. Set up and test the whole
  chain without closing. Any such decision is written as an ADR first.
- Before `dd`, flashing, partitioning or ANY command that deletes or overwrites data, stop and
  ask the user, and confirm the exact target (a wrong disk can destroy their data).
- Do not run `git commit` unless the user asks. Propose the commit message first.
- Do not create new top-level directories (e.g. for the KiCad HAT or the pytest rig) before the
  corresponding phase starts, and only with the user's approval.
- The relay switches only the 5 V DC supply line, never mains voltage.
- Signing keys, passwords and Wi-Fi credentials never go in the repo or in a prompt.
  Keys stay outside the repo and are git-ignored.

## Environment

- Development is inside WSL2 (Ubuntu), in the Linux filesystem (~/...), never under /mnt/c.
  Yocto cannot run on Windows and needs a case-sensitive, fast filesystem.
- The Yocto workspace (Poky, ST layers, build, downloads, sstate-cache) lives inside yocto/,
  and the west workspace (Zephyr tree, modules) inside firmware/. Both are ignored by
  .gitignore with an allow-list: only our own layer and app are re-included.
- Never run `git clean -x` (or `-X`): it deletes ignored files, which here means the Yocto
  downloads and sstate-cache (hours of rebuilding). Plain `git clean -fd` is safe.
- Windows is used only for tools that fit there (KiCad, STM32CubeProgrammer, a serial terminal).

## Stack

Yocto / OpenSTLinux, TF-A, OP-TEE, U-Boot, RAUC (A/B, dm-verity), Zephyr (west, CMake),
KiCad, Python with pytest and labgrid.

## Repository layout

machine-vibration-health-node/
- yocto/          Yocto layer(s), recipes and configuration for the Linux image
                  (own layer: yocto/meta-vibration-node/)
- firmware/       Zephyr application for the M33
- docs/
  - captures/     evidence: serial logs, logic analyzer captures, screenshots, measurements
  - decisions/    all ADRs, plus adr-template.md (written by the user, always follow it)
  - datasheets/   manuals and datasheets (PDFs git-ignored); README.md is the index
                  with document ID, revision, source and SHA-256
  - architecture.md   the CURRENT state of the whole system, including open and unverified items
  - plan.md       the project plan (phases and the proof that ends each one)
- tools/          helper scripts
- CLAUDE.md, README.md, .gitignore

Keep docs up to date as the work moves:
- Update docs/architecture.md whenever the state of the system changes.
- "Open items and unverified facts" in architecture.md holds only items that are still open.
  When one is resolved, record the fact in its proper section (with its source) and delete the
  item from the list. No "resolved" entries.

## Decisions and learning checks

- Every decision gets an ADR in docs/decisions/, following docs/decisions/adr-template.md exactly.
  Do not invent or change the template. If the template is missing, ask the user for it.
  Examples: choice of accelerometer, boot chain options, fuses and secure boot.
  Every ADR cites the source (document and page) that supports it.
- At the end of each phase, before moving to the next one, quiz the user with 5-10 questions to
  check real understanding. If an answer is weak, go back and explain again.
  Do not start the next phase until the user is comfortable with the current one.
