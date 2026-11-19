# ADR-0004: Keep the ADXL345 as the vibration sensor

**Status:** Accepted
**Date:** 2026-11-19
**Phase:** 2

## Context

I need a sensor bolted onto the fan that measures its vibration at a fixed rate and sends the
samples to the M33 over SPI. I already own one: an ADXL345 on a DFRobot SEN0032 breakout. This ADR
records whether it is good enough, and its limits.

The fan is a Waveshare Fan-4010-5V with 9 blades. Waveshare does not publish its speed. Assuming a
typical 5000 to 7000 rpm for a 40 mm fan (not checked), it vibrates at about 83 to 117 Hz from
rotation, and about 750 to 1050 Hz from the blades passing (9 x rotation). The real speed is
measured in phase 3, from the rotation peak in the spectrum.

What the ADXL345 gives (ADXL345 Rev. G):
- Output data rate up to 3200 Hz (p. 3). Bandwidth is half of it, 1600 Hz (p. 4, note 3).
- SPI up to 5 MHz, CPOL = 1, CPHA = 1 (p. 14).
- A 32-sample FIFO (p. 21), with DATA_READY, watermark and overrun interrupts on INT1 or INT2
  (p. 20, p. 27).
- Range up to ±16 g, 13-bit in full resolution (p. 3).

The breakout (SEN0032 schematic, rev 2.0) has an on-board regulator between VCC and the chip, and
no level shifting or pull-ups on the signals. Connector J1: 1 GND, 2 VCC, 3 CS, 4 INT1, 5 INT2,
6 SDO, 7 SDA, 8 SCL. The schematic names the regulator BL8555-30 and labels its output 3.3V; I
measured 3.30 V on C2 with VCC = 5 V (2026-11-19).

## Options considered

### Option A: Keep the ADXL345 on the SEN0032 breakout
- How it works: the breakout is bolted onto the fan and wired to the HAT over SPI.
- Pros: already in hand, SPI with a FIFO and interrupts, 1600 Hz covers the fan's rotation and blade
  frequencies, widely used and documented.
- Cons: nothing above 1600 Hz. A general-purpose part, not designed for vibration analysis.

### Option B: A sensor designed for vibration monitoring
- How it works: buy an accelerometer made for machine monitoring, with a much wider bandwidth.
- Pros: would also see the high frequencies of early bearing wear.
- Cons: not needed for a fan, has to be bought, and such parts usually come in small packages that
  are hard to hand-solder. I did not evaluate a specific part.

## Decision

Option A. Everything the fan produces lies below 1600 Hz, and the sensor is already on my desk, so a
new part would add cost and risk without changing what the project can show.

## Consequences

- Easier: phase 3 can start with the breakout on wires right away.
- Limit: nothing above 1600 Hz is visible, so the project detects imbalance and gross faults (the
  tape on a blade), not early bearing wear.
- Firmware (phase 3):
  - SPI between 2 MHz (needed for 3200 Hz, p. 14) and 5 MHz.
  - One sample is a 7-byte burst read: the command, then DATAX0 to DATAZ1 (p. 27).
  - Above 1.6 MHz, CS goes high between FIFO reads to leave the 5 µs the FIFO needs (p. 21).
  - The FIFO holds 10 ms of data at 3200 Hz (32 / 3200), and the overrun bit shows lost samples
    (p. 20).
  - The sensor's own clock sets the sample timing, so the M33 reads when INT1 says data is ready,
    not on its own timer.
- HAT (phase 2):
  - The sensor connector carries GND, VCC, CS, INT1, INT2 and the SPI lines, which the breakout
    labels with their I2C names: SCL is SCLK, SDA is the data input (MOSI), SDO is the data output
    (MISO) (p. 6).
  - VCC comes from 5 V, so the regulator has headroom.
  - The signals are 3.3 V, inside the chip's limit of its supply + 0.3 V (p. 5).
- Follow-up: in phase 3, measure the fan's real speed and check that the blade frequency is below
  1600 Hz.

## Interview angle

I checked that every frequency my fan produces fits under the sensor's 1600 Hz limit, and I can say
exactly what the sensor cannot see: early bearing wear.
