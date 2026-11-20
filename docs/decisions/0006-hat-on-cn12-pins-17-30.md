# ADR-0006: Put the HAT on CN12 pins 17-30 only, powered from 3V3

**Status:** Accepted
**Date:** 2026-11-20
**Phase:** 2

## Context

The HAT needs 10 signals from CN12: SPI (4 lines), INT1 and INT2 from the sensor, the EEPROM's I2C
(2 lines), an LED and a button, plus a supply and ground. A HAT on all 40 pins would also cover the
5 V pins 2 and 4 that the fan needs, and every pin left for later ideas.

What the board offers (UM3571 Rev 1, p. 26, Table 17; schematic B-02, sheets 3 and 12 of 14):
- 3V3 on pins 1 and 17, 5 V on pins 2 and 4, GND on pins 20, 25 and 30 among others.
- SPI1 on pins 19, 21, 23 and 24.
- Pins 27 and 28 are named `ID_SD` and `ID_SC` on the schematic, have no pull-ups on the board, and
  can be set to I2C3 (DS14556 Rev 2, Table 13, p. 77, 80).
- All these pins are on VDDIO, at 3.3 V, and go nowhere else on the board.

The sensor can run on 3.3 V: with VCC = 3.3 V I measured 3.30 V on the breakout's C2 (2026-11-19),
inside the ADXL345's 2.0 to 3.6 V (ADXL345 Rev. G, p. 1).

Each interrupt input needs its own EXTI line, and EXTI line n takes pin n of only one port
(RM0506 Rev 2, section 29.4.3, p. 1513).

## Options considered

### Option A: Full-size HAT on all 40 pins
- How it works: a 2x20 header, 5 V from pins 2 and 4, and a connector on the HAT for the fan.
- Pros: holds firmly on the board, 5 V available on the HAT.
- Cons: covers every pin, so the fan and any later addition must go through the HAT. Bigger board
  and more solder joints.

### Option B: Partial HAT on pins 17-30, sensor from 3V3
- How it works: a 2x7 header on pins 17 to 30, which hold every signal the HAT needs plus 3V3 and
  GND.
- Pros: the other 26 pins stay free (the fan takes 5 V and GND straight from pins 2/4 and 6).
  Smaller board, fewer joints.
- Cons: a 2x7 header holds less firmly. No 5 V on the HAT. The HAT covers the SPI pins, so the logic
  analyzer cannot clip onto them on the board.

## Decision

Option B. Every signal the HAT needs, 3.3 V and ground fit in 7 rows, and the sensor works on
3.3 V, so covering the whole header would only take pins away from the fan and from later ideas.

The pin plan:

| Pin | Signal | Chip pin | | Pin | Signal | Chip pin |
|---|---|---|---|---|---|---|
| 17 | 3V3 | - | | 18 | Button | PB14 (EXTI 14) |
| 19 | SPI1 MOSI | PI5 | | 20 | GND | - |
| 21 | SPI1 MISO | PD1 | | 22 | INT1 | PD13 (EXTI 13) |
| 23 | SPI1 SCK | PD11 | | 24 | SPI1 CS (NSS) | PD2 |
| 25 | GND | - | | 26 | INT2 | PC7 (EXTI 7) |
| 27 | EEPROM SDA (I2C3) | PG0 | | 28 | EEPROM SCL (I2C3) | PC12 |
| 29 | LED | PH13 | | 30 | GND | - |

## Consequences

- Easier: a small HAT, and 26 header pins free for the fan and later use.
- Harder: the HAT can wobble (a spacer under it helps). Probing SPI needs access points on the HAT.
- The EEPROM's two pull-up resistors go on the HAT, since the board has none on pins 27/28.
- The sensor's regulator has no headroom at 3.3 V and passes the supply through.
- The plan is only proven when the sensor works with jumper wires on exactly these pins, with SPI1
  and the interrupts owned by the M33 (open items in architecture.md). That test comes before the
  PCB order.

## Interview angle

I sized the HAT to the 14 pins it really needs, after checking on the board schematic that every
signal, 3.3 V and ground are inside them and that nothing else on the board uses those pins.
