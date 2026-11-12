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

## Chip: STM32MP215FAN3 (VFBGA273, silicon revision "Y")

| File | Document | Revision | Source |
|---|---|---|---|
| `ds14556-rev2.pdf` | DS14556, STM32MP211C/F, STM32MP213C/F, STM32MP215C/F datasheet | Rev 2, Sep 2026 | [link](https://www.st.com/resource/en/datasheet/stm32mp215c.pdf) |
| `es0628-rev1.pdf` | ES0628, STM32MP21xA/C/D/F device errata | Rev 1, Aug 2025 | [link](https://www.st.com/resource/en/errata_sheet/es0628-stm32mp21xacdf-device-errata-stmicroelectronics.pdf) |
| `rm0506-rev2.pdf` | RM0506, STM32MP21xx reference manual | Rev 2, Aug 2026 | [link](https://www.st.com/en/microcontrollers-microprocessors/stm32mp215f.html#documentation) |

## SHA-256 checksums

```
420b39cabf340c535cd60d0544a68ff4e8e03e7020291d41798056ba495833ff  um3571-rev1.pdf
705d0d964edb3351b3d800a7b4a4e32eaca3bfe95b7934a6ceb762fd696dae88  mb2059-mp215f-b02-schematic.pdf
6aa689d111ddc8575990bbb2eee55c96664be90bcf590c599b02dc5b9220d9a0  ds14556-rev2.pdf
d9b28dea43bfd41e30d6938caf020e366f8f770f7b3962b999de33410edcca4c  es0628-rev1.pdf
2e8a6f8eb676fae1c70768e9f3e49b4f111e3b94aadf58bf172624a5920c04e0  rm0506-rev2.pdf
```

A different checksum means a different file (usually a silent update by the vendor):
re-read the revision on the cover and update this index.
