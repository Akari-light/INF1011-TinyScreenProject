# ICT1011 TinyCircuits Team Exploration Project

This repository will contain the team's TinyCircuits application, hardware information, tests, and delivery documentation. The application has not been selected yet.

**Current phase:** foundation setup, before brainstorming. No application code or hardware configuration has been selected.

See [PROJECT_PLAN.md](PROJECT_PLAN.md) for requirements, architecture decisions, responsibilities, milestones, task status, and team working conventions.

## Hardware and software

| Item | Current value |
| --- | --- |
| Processor board, revision, and accessories | TBD: inventory the supplied kit |
| Development environment and version | TBD: verify the vendor-supported workflow |
| Board package and library versions | TBD: record after a successful baseline build |

## Setup and operation

1. Clone the repository and open its root folder.
2. Check the current milestone and prerequisites in the project plan.
3. Once the board and toolchain are confirmed, follow the build and upload instructions added here.

Build, upload, operation, and test commands are not available yet. Do not infer a board target or install an assumed toolchain from this skeleton.

## Repository layout

The initial shared files are this README, the project plan, and repository configuration. Add these directories when their first real files are needed:

| Directory | Purpose |
| --- | --- |
| `firmware/` | Device firmware and the selected toolchain's build configuration |
| `hardware/` | Component list, pin assignments, wiring diagrams, and editable design files |
| `tests/` | System test procedures and selected reproducible evidence |
| `docs/` | Supporting diagrams, source acknowledgements, and demo/submission material |

Use the chosen toolchain's native layout inside `firmware/`. Add a companion application directory only if the chosen application requires one.

## Verification and reused material

No firmware has been built, uploaded, simulated, or physically tested yet. Test procedures and expected results will be defined alongside the selected application. Record a successful build separately from a successful hardware test.

Preserve upstream license notices. Attribute reused or adapted code in the relevant source files and delivery material; record source URLs, versions or revisions, and modifications. A project-wide license has not been selected.
