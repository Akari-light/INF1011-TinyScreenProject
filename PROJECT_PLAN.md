# TraceDeck project plan

Last updated 2026-11-24 (Asia/Singapore).

TraceDeck demonstrates USB and Bluetooth LE observations using owned equipment and synthetic data. The USB demo uses an isolated lab network and uploads a synthetic test file to TraceHub. The Bluetooth menu contains the walk-away lock audit, the desk beacon transmitter and the nearby advertisement scanner.

## Current checkpoint

- **BadUSB firmware.** The refactored sketch has a payload picker, a 60-second arming gauge, and injection progress. Drop & Upload is the implemented payload. Passive requires manual ARM then INJECT. Active consumes a physical arm and executes the selected payload once on USB enumeration. The Active execution change awaits physical verification.
- **TraceHub.** The redesigned Ops Console and flattened Flask layout merged to main at `4949912`. The dashboard has a collection overview and a searchable Loot Log with hostname status, sorting, and live refresh. The console and both intake profiles share local SQLite storage.
- **Verification.** An earlier Passive run produced the expected 11-byte upload in TraceHub. Both firmware variants compile. Repeat the demo with the current firmware and redesigned Ops Console.
- **24 November ZIP update.** Drop & Upload now saves the Desktop message and closes Notepad before starting a visible PowerShell ZIP upload of the fixed dummy folder. TraceHub retains the archive locally, indexes text, and provides an Ops Console download. The sender removes its temporary ZIP after acknowledgement. Loopback PowerShell upload, download integrity, and failure retention passed. The physical Notepad/save/close sequence and integrated board run require verification.
- **Integration.** The latest Drop & Upload changes are on the BadUSB branch. The redesigned console is on `main`. See [README.md](README.md) for setup and the build flag for each revision.
- **BLE Nearby.** The integrated TinyScreen+ build received live advertisement reports. The standalone bench captured the owned reference beacon's advertisement and scan response and verified a stale-to-fresh transition. Phone attribution, lock/unlock observations and a complete OLED walkthrough are separate checks.
- **Walk-away lock audit.** The handheld records the tester's doorway press and desk-beacon freshness. A local Windows observer records the workstation session lock. TraceHub joins them by trial ID. On 24 November the physical Win+L control produced a lock and unlock in session 1, and COM8 armed on the PC's reference advertisement. The clean-session interval at the assessment doorway still needs a paired-phone walk. See the Walk-away lock audit section in the README.

## System layout

| Component | Responsibility |
| --- | --- |
| TinyScreen+ USB node | Sends keystrokes after manual arming and injection in Passive mode. Its OLED shows injection progress. |
| Windows target laptop | Receives the keystrokes, saves the Desktop message, and runs the fixed PowerShell ZIP upload and acknowledgement-based cleanup. |
| TraceHub server | Accepts the browser upload, stores it in SQLite, and shows the receipt in the local Ops Console. A separate HTTPS listener accepts uploads from enrolled clients. |
| BLE Nearby | Choose Nearby scan or Walk-away under Home → Bluetooth. A second TinyScreen+ running the same firmware can advertise the team marker from Desk beacon. |

The TraceHub receipt confirms the upload. It records the file text and metadata, observed source IP, UTC receipt time, and loot ID. An identical retry keeps the same ID.

## Requirements and evidence

| ID | Requirement | Current evidence and remaining check |
| --- | --- | --- |
| H01 | Require a physical arm and a deliberate trigger. Consume the arm once. Cancel and expiry disarm the board. | **V01 partial.** A prior Passive run passed. Recheck the configured build for repeat, unarmed, cancelled, expired, and held-INJECT cases. Verify Active execution and disabled-payload behavior on connection. |
| H02 | Observe nearby legacy BLE advertisements and show live activity and freshness on the handheld. | **V02 partial.** The integrated scanner received reports. Verify the full OLED flow and identify the owned phone's observation before comparing lock states. |
| H03 | Store the fixed USB upload and report its receipt independently. | **V03 partial.** Flask checks passed for the browser intake and identical retries. |
| H04 | Present evidence with its source, identifier, and timestamp. | **V03 partial.** The merged dashboard passed local checks for search, receipt rendering, and loopback assets. Match a new USB receipt in the Ops Console. Compare the OLED and USB serial records for an owned BLE test device. |
| H05 | Repeat both paths on the assessment setup and recover saved evidence after restart. | **V04 planned.** Rehearse from the documented setup and check previous receipts after restarting TraceHub. |

Record the revision, tester, date, and outcome for physical tests.

## Next work

| Work | Status | Exit check |
| --- | --- | --- |
| BadUSB and Ops Console rehearsal | Ready for physical review | Check the OLED menu and gauge. Run Passive and match the target browser's loot ID to the Ops Console receipt. Repeat the button and expiry checks. |
| Active execution | Physical verification pending | Verify one execution after a physically armed connection, disabled-payload routing, and no execution after cancellation or expiry. Compare reconnect with sleep and wake, since wake can also re-enumerate USB. |
| BLE Nearby | Integrated | Rehearse Home, live list, details and back navigation. Compare verified phone advertisements before and after lock. Keep ambiguous observations inconclusive. |
| E04 hostname attribution | Deferred | Map browser source IPs using reverse DNS or a time-valid, operator-verified record. Preserve the original IP and receipt time. Keep this provenance distinct from the client-reported hostname already stored by the authenticated HTTPS receiver. Verify two known devices and an unresolved device. |

Use descriptive branch names, review each change and its checks before committing, and keep runtime data and credentials out of Git. Record team ownership and contributions before rehearsal.

## Assessment and delivery

The requirements come from the ICT1011 Project Briefing (AY26), pages 2–8, and Assessment Rubrics, page 1. Creativity, technical work, YouTube video, and demonstration each carry five points. Peer assessment affects individual marks.

| ID | Requirement | Acceptance evidence |
| --- | --- | --- |
| A01 | Use TinyCircuits hardware for an innovative application; additional devices are permitted | Working application and documented architecture |
| A02 | Attribute reused code in source files and the video | Source notices and video citations reviewed |
| A03 | Produce a 2-3 minute YouTube video covering the application, working demonstration, distribution of work, and sources | Final duration and content checked |
| A04 | Submit source, project files, and documentation as a ZIP through the designated course dropbox; post the video through the designated course channel | Archive extracted and checked; submission confirmations recorded |
| A05 | Submit by 25 Nov 2026, 23:59 (Singapore time) | Submission confirmation |
| A06 | Demonstrate on 26 Nov 2026, Week 13, with behavior matching the video; all members attend as required | Rehearsal and live demonstration |

Verify the extracted submission ZIP on a clean setup and confirm the assigned live demo slot.
