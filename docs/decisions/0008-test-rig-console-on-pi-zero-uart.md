# ADR-0008: Read the board's console with the Pi Zero's own UART

**Status:** Accepted
**Date:** 2026-11-26
**Phase:** 3

## Context

In the test rig (phase 6), labgrid on the Raspberry Pi Zero 2 W has to read the Cortex-A35 console
and type into it without me: see the boot messages, log in, start an update, check which slot
booted, a hundred times in a row with power cuts in between.

The console is on CN2 "VCPA35", a 3-pin header: pin 1 = board RX, pin 2 = GND, pin 3 = board TX,
USART2 at 115200 baud (UM3571 Rev 1, p. 9, p. 28, p. 30). On the schematic, CN2 pins 1 and 3 go
straight to the MPU's USART2_RX and USART2_TX, with no level shifter (schematic MB2059 B-02,
sheet 3). Those pins are on VDDIO, which is 3.3 V on this board (UM3571 Rev 1, p. 11; schematic
B-02, sheets 4 and 5).

I own one USB-UART adapter (3-wire end to the board, USB-C socket to the host). During development
it is also the only way to see the Cortex-M33 console on CN1, where Zephyr prints.

## Options considered

### Option A: My USB-UART adapter, plugged into the Pi Zero
- How it works: the adapter stays wired to CN2, and a micro-USB to USB-C data cable connects it to
  the Pi Zero's data port. Linux on the Pi sees a USB serial device.
- Pros: the adapter already works on CN2. The Pi Zero's GPIO pins are not wired to the board.
- Cons: the only adapter is taken by the rig, so the M33 console needs it moved back and forth.
  One more cable to buy, one more USB device that has to enumerate.

### Option B: The Pi Zero's own UART on its GPIO header
- How it works: three jumper wires from the Pi Zero's header to CN2: GPIO14 (TX, header pin 8) to
  CN2 pin 1, GPIO15 (RX, header pin 10) to CN2 pin 3, GND to CN2 pin 2. The Pi's GPIO pins use
  3.3 V logic (Raspberry Pi documentation, "Raspberry Pi hardware", section "40-pin GPIO header",
  https://www.raspberrypi.com/documentation/computers/raspberry-pi.html, read 2026-11-26), the same
  level as CN2.
- Pros: nothing to buy. The USB-UART adapter stays free for the M33 console.
- Cons: on the Zero 2 W, the UART on GPIO14/15 is the mini UART by default, whose baud rate follows
  the VPU clock; the full PL011 UART is connected to Bluetooth. Linux on the Pi also uses that UART
  as its own login console by default (Raspberry Pi documentation, "Configuration", section
  "Configure UARTs", https://www.raspberrypi.com/documentation/computers/configuration.html, read
  2026-11-26). Both have to be changed on the Pi. The pins are not 5 V tolerant (same page).

## Decision

Option B. It needs no extra hardware and leaves my only USB-UART adapter for the M33 console, which
I need for debugging Zephyr; the cost is two settings on the Pi Zero, made once.

## Consequences

- Easier: the console link is three wires with no USB device in between, and the adapter stays on
  CN1 while the rig runs.
- Harder: the Pi Zero must be configured before phase 6. Put the PL011 on GPIO14/15 with
  `dtoverlay=disable-bt` in `config.txt` and `systemctl disable hciuart` (Bluetooth on the Pi is
  not used), and disable the Pi's serial login console while keeping the UART enabled
  (`raspi-config`: Interface Options, Serial Port: login shell No, hardware Yes). Without the second
  step the Pi sends its own boot messages and login prompt into the board's console.
- Harder: the Pi's TX idles high at 3.3 V into the board's RX pin, also while the relay has cut the
  board's 5 V. That can feed a little current into an unpowered chip through its pin. The same is
  true for any console adapter; to check before the power-cut tests (a series resistor on that line
  limits it).

## Interview angle

I connected the test rig's console straight to the Pi's own UART instead of a USB adapter, after
checking that both sides use 3.3 V logic, and freed my only adapter for the second core's console.
