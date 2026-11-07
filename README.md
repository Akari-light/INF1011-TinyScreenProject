# TraceDeck

TraceDeck is an ICT1011 TinyCircuits security lab. Two TinyScreen+ nodes run controlled USB keyboard and Wi-Fi interactions against a laptop-hosted IoT decoy. A live timeline links each lab run to its recorded events.

**Current phase:** scope review and hardware feasibility planning. Implementation is pending.

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for requirements, architecture decisions, responsibilities, milestones, task status, and team working conventions.

## Hardware and software

| Item | Current value |
| --- | --- |
| USB keyboard node | TinyScreen+ ASM2022 with a USB data cable |
| Wi-Fi node | TinyScreen+ ASM2022 with the borrowed ASD2123-R Wi-Fi TinyShield |
| Lab host | Team laptop running the input app, IoT decoy, and dashboard on a private 2.4 GHz network |
| Toolchain and library versions | Pending a successful baseline build and hardware check |

## Setup and operation

1. Clone the repository and open its root folder.
2. Check the current milestone and prerequisites in the project plan.
3. Follow the build and upload instructions here once the toolchain has been verified.

Build, upload, and operation instructions are pending hardware feasibility checks.

## Repository layout

The initial shared files are this README, the project plan, and repository configuration. Add these directories when their first real files are needed:

| Directory | Purpose |
| --- | --- |
| `firmware/` | Device firmware and the selected toolchain's build configuration |
| `hardware/` | Component list, pin assignments, wiring diagrams, and editable design files |
| `tests/` | System test procedures and selected reproducible evidence |
| `docs/` | Supporting diagrams, source acknowledgements, and demo/submission material |

Use the chosen toolchain's native layout inside `firmware/`. Add a directory for the companion application when laptop development begins.

## Verification and reused material

The project plan lists the acceptance checks. Record build results separately from physical USB and Wi-Fi test results.

Preserve upstream license notices. Attribute reused or adapted code in the relevant source files and delivery material. Record source URLs, versions or revisions, and modifications. A project-wide license is pending.
