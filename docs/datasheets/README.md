# Datasheets and manuals

The PDFs are not in git (vendor copyright, large binaries). This index records exactly which
file every citation in the project refers to. Download each one from its source, save it under
the file name below, and check it with `sha256sum *.pdf`.

Citations use the form "UM3571 Rev 1, p. 13". A new revision gets a new file and a new row;
the old row stays until nothing cites it anymore.

## Board: STM32MP215F-DK (MB2059, variant MP215F, revision B-02)

| File | Document | Revision | Source |
|---|---|---|---|
| `um3571-rev1.pdf` | UM3571, Discovery kit with STM32MP215F MPU, user manual | Rev 1, Nov 2025 | [link](https://www.st.com/resource/en/user_manual/um3571-discovery-kit-with-stm32mp215f-mpu-stmicroelectronics.pdf) |
| `mb2059-mp215f-b02-schematic.pdf` | MB2059 schematic, variant MP215F | B-02, 08-Jul-2025 | [link](https://www.st.com/en/evaluation-tools/stm32mp215f-dk.html#cad-resources) |

## Chip: STM32MP215FAN3 (VFBGA273; UM3571 lists silicon revision "Y", my chip reports "Z")

| File | Document | Revision | Source |
|---|---|---|---|
| `ds14556-rev2.pdf` | DS14556, STM32MP211C/F, STM32MP213C/F, STM32MP215C/F datasheet | Rev 2, Sep 2026 | [link](https://www.st.com/resource/en/datasheet/stm32mp215c.pdf) |
| `es0628-rev1.pdf` | ES0628, STM32MP21xA/C/D/F device errata | Rev 1, Aug 2025 | [link](https://www.st.com/resource/en/errata_sheet/es0628-stm32mp21xacdf-device-errata-stmicroelectronics.pdf) |
| `rm0506-rev2.pdf` | RM0506, STM32MP21xx reference manual | Rev 2, Aug 2026 | [link](https://www.st.com/en/microcontrollers-microprocessors/stm32mp215f.html#documentation) |

## Accelerometer: ADXL345 on the DFRobot SEN0032 breakout

| File | Document | Revision | Source |
|---|---|---|---|
| `adxl345-revg.pdf` | ADXL345, 3-axis ±2 g/±4 g/±8 g/±16 g digital accelerometer datasheet | Rev. G, May 2022 | [link](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf) |
| `sen0032-schematic-rev2.pdf` | SEN0032, triple axis accelerometer breakout (ADXL345), schematic | 2.0, 7-May-2011 | [link](https://dfimg.dfrobot.com/wiki/18629/SEN0032_adxl345-digital-triaxial-acceleration-sensor_schematics_V1.0.pdf) |

## Relay: DFRobot DFR0251 relay module (Hongfa HF152FD)

| File | Document | Revision | Source |
|---|---|---|---|
| `hf152fd-rev1.pdf` | HF152FD, subminiature high power relay datasheet | Rev. 1.00, 2013 | [link](https://gr.mouser.com/datasheet/3/1500/1/HF152FD_Datasheet.pdf) |

## EEPROM: Microchip 24LC256

| File | Document | Revision | Source |
|---|---|---|---|
| `24xx256-revm.pdf` | 24AA256/24LC256/24FC256, 256K I2C CMOS serial EEPROM datasheet (DS21203) | Rev. M, 2004 | [link](https://grobotronics.com/images/companies/1/21203M.pdf) |

## SHA-256 checksums

```
420b39cabf340c535cd60d0544a68ff4e8e03e7020291d41798056ba495833ff  um3571-rev1.pdf
705d0d964edb3351b3d800a7b4a4e32eaca3bfe95b7934a6ceb762fd696dae88  mb2059-mp215f-b02-schematic.pdf
6aa689d111ddc8575990bbb2eee55c96664be90bcf590c599b02dc5b9220d9a0  ds14556-rev2.pdf
d9b28dea43bfd41e30d6938caf020e366f8f770f7b3962b999de33410edcca4c  es0628-rev1.pdf
2e8a6f8eb676fae1c70768e9f3e49b4f111e3b94aadf58bf172624a5920c04e0  rm0506-rev2.pdf
87ae2212498c35a6759d8732adee0ec9b9d8d60fa95688bc5904f1f07ceb8ff6  adxl345-revg.pdf
c9cc93bf3e46ab1103863963aba46b03266a183e4aebbd0a339cfc77920cb4e5  sen0032-schematic-rev2.pdf
38dd9b38da342a159fe2da6e82c8b0ebc403584907947c27e1badb0987c05f08  hf152fd-rev1.pdf
f7175c8a7acbef964cf412762aa6fedc968b674a7491b2d0f0845ae38f94a6c2  24xx256-revm.pdf
```

A different checksum means a different file (usually a silent update by the vendor):
re-read the revision on the cover and update this index.
