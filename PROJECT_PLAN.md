# TraceDeck project plan

Last updated 2026-11-12 (Asia/Singapore).

TraceDeck demonstrates USB and Wi-Fi interactions on an isolated ICT1011 lab network using owned equipment and synthetic data. The USB demo creates a test file on the Windows target laptop and uploads it to the TraceHub server. The planned Wi-Fi demo sends a request directly to a mock IoT service on that server.

## Current checkpoint

- **BadUSB firmware.** The refactored sketch has a payload picker, a 60-second arming gauge, and injection progress. Drop & Upload is the implemented payload. Passive requires manual ARM then INJECT. Active is a status-only connection preview.
- **TraceHub.** The redesigned Ops Console and flattened Flask layout merged to main at `4949912`. The dashboard has a collection overview and a searchable Loot Log with hostname status, sorting, and live refresh. The console and both intake profiles share local SQLite storage.
- **Verification.** An earlier Passive run produced the expected 11-byte upload in TraceHub. Both firmware variants compile. Repeat the demo with the current firmware and redesigned Ops Console.
- **Integration.** The latest Drop & Upload changes are on the BadUSB branch. The redesigned console is on `main`. See [README.md](README.md) for setup and the build flag for each revision.

## System layout

| Component | Responsibility |
| --- | --- |
| TinyScreen+ USB node | Sends keystrokes after manual arming and injection in Passive mode. Its OLED shows injection progress. |
| Windows target laptop | Receives the keystrokes and saves the test file. With the upload build, its browser submits the file to TraceHub. |
| TraceHub server | Accepts the browser upload, stores it in SQLite, and shows the receipt in the local Ops Console. A separate HTTPS listener accepts uploads from enrolled clients. |
| Planned Wi-Fi node | A second TinyScreen+ with the borrowed ASD2123-R Wi-Fi TinyShield calls the mock IoT service and displays the result or timeout. |

The TraceHub receipt confirms the upload. It records the file text and metadata, observed source IP, UTC receipt time, and loot ID. An identical retry keeps the same ID.

## Requirements and evidence

| ID | Requirement | Current evidence and remaining check |
| --- | --- | --- |
| H01 | Require a physical arm and a deliberate trigger. Consume the arm once. Cancel and expiry disarm the board. | **V01 partial.** A prior Passive run passed. Recheck the configured build for repeat, unarmed, cancelled, expired, and held-INJECT cases. Active remains in diagnostic review. |
| H02 | Have the Wi-Fi node send one direct lab request and display its result or timeout. | **V02 planned.** Verify with the borrowed shield and the server's mock IoT service. |
| H03 | Store the fixed USB upload and independently record the mock IoT request and outcome. | **V03 partial.** Flask checks passed for the browser intake and identical retries. The mock service and live Wi-Fi evidence are pending. |
| H04 | Present evidence with its source, identifier, and server timestamp. | **V03 partial.** The merged dashboard passed local checks for search, receipt rendering, and loopback assets. Verify a new receipt and its detail view in the Ops Console. |
| H05 | Repeat both paths on the assessment setup and recover saved evidence after restart. | **V04 planned.** Rehearse from the documented setup and check previous receipts after restarting TraceHub. |

Record the revision, tester, date, and outcome for physical tests.

## Next work

| Work | Status | Exit check |
| --- | --- | --- |
| BadUSB and Ops Console rehearsal | Ready for physical review | Check the OLED menu and gauge. Run Passive and match the target browser's loot ID to the Ops Console receipt. Repeat the button and expiry checks. |
| Active connection diagnostic | In review | Compare a battery-powered USB reconnect without reboot to sleep and wake on the target laptop with the cable attached. Keep keyboard output gated if wake and attachment are ambiguous. |
| Wi-Fi path | Planned | Implement the mock service and board request, verify success and timeout, and rehearse the combined demo. |
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
