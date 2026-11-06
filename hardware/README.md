# RFLink Universal — Hardware

This part of the repository will contain the **real circuit board design**, a verified **Bill of Materials (BOM)** and manufacturing files. A BOM is the list of parts needed to assemble the board.

> **Not manufacturing-ready:** no verified PCB design, part list or production package has been published here yet. Please do not order or assemble a board from these placeholders.

## Where will everything go?

| Folder | What you will find once available |
| --- | --- |
| [`kicad/`](kicad/README.md) | Editable circuit schematic and PCB layout in KiCad |
| [`bom/`](bom/README.md) | Parts list with quantities, values, footprints and confirmed part numbers |
| [`manufacturing/`](manufacturing/README.md) | Gerbers, drill files and optional assembly outputs |
| `images/` (planned) | Verified photos or renders showing the physical board and connectors |

## What will be documented for every board revision?

1. **Board revision** (for example, Rev A) so you can identify your PCB.
2. **Pinout**: which ESP32-S3 pin connects to each CC1101 radio.
3. **Verified BOM**: exactly which components are required.
4. **Assembly instructions**: where parts go, what direction they face and what to check before applying power.
5. **Matching firmware board profile**: the pinout and parameters that belong to that exact PCB revision.

Default pinouts and SPI settings may be published. Your own hardware overrides and credentials stay in local RFLink Builder storage.

## Verification before release

We must confirm that the KiCad schematic matches the physical tested board; radio module variants, voltages, connectors and component footprints are correct; BOM references match the PCB; and all manufacturing outputs use the same revision.

Never invent a manufacturer part number, a quantity, a price, or a claim that the design has passed manufacturing checks.
