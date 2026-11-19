# ADR-0005: Measure the board's current on the test rig, not on the HAT

**Status:** Accepted
**Date:** 2026-11-19
**Phase:** 2

## Context

The plan puts a current-measurement chip (e.g. INA219) on the HAT, so that phase 7 can show the
board's power at every stage of an update. For that, the board's supply current has to flow through
the chip's shunt resistor.

Where the board's power comes from (schematic MB2059 B-02):
- The board is "designed to be powered by a 5-V USB Type-C power source" (UM3571 Rev 1, p. 11).
- VBUS from the USB-C connector CN10 goes through U14, an STEF05SGR electronic fuse, and comes out
  as `5V_VIN` (sheet 8 of 14).
- CN12 pins 2 and 4 are connected straight to `5V_VIN` (sheet 12 of 14).

So the 5 V pins of CN12 are an output of the board's supply, not its input. A chip on the HAT would
only see the HAT's own current.

## Options considered

### Option A: Power the board through CN12, with the sensor on the HAT
- How it works: the supply goes to the HAT, through the sensor, into CN12 pins 2 and 4.
- Pros: the sensor stays on the HAT, as in the plan.
- Cons: the supply enters after U14, so the board loses that protection, and the board is not
  documented to be powered this way. A mistake could damage my only board.

### Option B: Keep the sensor on the HAT anyway
- How it works: the sensor measures what the HAT draws from CN12.
- Pros: no change to the plan.
- Cons: it measures only the HAT, not the board, so the phase 7 graph would be meaningless.

### Option C: Measure in the USB-C power path, on the test rig
- How it works: two USB-C female breakouts wired back to back. GND, CC1 and CC2 pass straight
  through. VBUS goes through the relay and an INA219 breakout in series. The Pi Zero reads the
  INA219 over I2C.
- Pros: measures the whole board, keeps U14 in the path, and the relay already sits on this line.
- Cons: more test rig wiring. The CC lines must pass through, otherwise the charger does not turn on
  VBUS: the board has pull-downs on CC1 and CC2 (1.2 kΩ + 3.9 kΩ, sheet 8) that the charger
  looks for.

## Decision

Option C. It is the only option that measures the board's real current without bypassing its
protection, and it puts the measurement on the same line as the relay that already controls the
power.

## Consequences

- Easier: the HAT gets simpler. It keeps the sensor connector, the EEPROM, the LED and the button.
- The HAT's I2C need shrinks to the EEPROM only.
- The phase 7 proof (a graph of the board power at every stage of an update) stays, measured by the
  test rig.

## Interview angle

Before putting a current sensor on my HAT, I traced the board's power on the schematic and found
that the header's 5 V pins are an output, so I moved the measurement to the one place the whole
current flows: the USB-C cable.
