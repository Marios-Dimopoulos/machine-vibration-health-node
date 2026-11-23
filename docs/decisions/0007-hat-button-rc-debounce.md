# ADR-0007: Debounce the HAT button in hardware with an RC filter

**Status:** Accepted
**Date:** 2026-11-22
**Phase:** 2

## Context

The HAT has a push button on CN12 pin 18 (PB14). A mechanical contact bounces: for a few
milliseconds after a press or a release it opens and closes several times, so the input can see
several edges instead of one.

The input decides with thresholds: high above VIH min 1.61 V, low below VIL max 1.05 V, with
0.44 V of hysteresis between them; an input leaks up to 10 µA (DS14556 Rev 2, p. 117, at
VDDIO = 3.3 V). The internal pull-up is 30 to 50 kΩ (DS14556 Rev 2, p. 118).

With only a pull-up and the switch to GND, every bounce reaches the pin and only software can hide
it.

## Options considered

### Option A: Pull-up only, debounce in software
- How it works: a pull-up to 3V3, the switch to GND. The firmware ignores new edges for about
  50 ms after the first one.
- Pros: two parts (or none, with the internal pull-up). Simple layout.
- Cons: every bounce still raises an interrupt. The behaviour depends on the firmware being right.

### Option B: RC filter in hardware, software as a safety net
- How it works: R4 47 kΩ from 3V3 to the pin, C2 100 nF from the pin to GND, and R5 10 kΩ between
  the pin and the switch. On a press, C2 discharges through R5 to about
  3.3 V x 10 / (47 + 10) = 0.58 V, below VIL. On a release, C2 recharges through R4 (time
  constant 47 kΩ x 100 nF = 4.7 ms) and needs about 2.2 ms to climb from 0.58 V to VIH 1.61 V.
- Pros: a contact that bounces open for less than about 2.2 ms never reaches VIH, so the pin sees
  one clean edge. R5 limits the discharge current of C2 through the switch. The worst-case
  leakage drops only 10 µA x 47 kΩ = 0.47 V across R4, so the released level stays about 2.8 V,
  above VIH.
- Cons: three parts instead of one or two. The button reacts a few milliseconds later, which a
  person cannot notice.

## Decision

Option B. The parts were already in stock and fit on the board, and a clean signal at the pin
means the interrupt fires once per press instead of trusting the firmware to filter every bounce.
A software debounce of about 50 ms stays as a safety net for longer bounces.

## Consequences

- Easier: the firmware sees one edge per press in normal use. Short bounces are filtered at the
  pin.
- Harder: the internal pull-up must stay off, so it does not run in parallel with R4. A bounce
  longer than about 2.2 ms still passes, so the software debounce is still needed.

## Interview angle

I filtered the button at the pin with an RC network sized against the input's own thresholds and
leakage from the datasheet, and kept a software debounce only as a safety net.
