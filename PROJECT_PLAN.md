# TraceDeck project plan

Last updated 2026-11-10 (Asia/Singapore).

TraceDeck is a controlled ICT1011 security lab. An owned TinyScreen+ demonstrates a USB keyboard interaction on PC2, and the TraceHub Ops Console on PC1 shows the evidence it received. The next system path is a direct Wi-Fi test from a second TinyScreen+ to a simulated IoT device.

## Current checkpoint

- **BadUSB base verified once.** Arvin reported a deliberate run on PC2. PC1 stored one browser upload for the expected 11-byte synthetic file. Repeat and button regression checks are pending.
- **Firmware layout committed.** The Arduino sketch remains at firmware/TraceDeck/TraceDeck.ino. Shared input and UI code live under src/input/ and src/ui/. BadUSB code lives under src/badusb/ with Drop & Upload in its own payload folder. The reorganization built in both variants at 024c89d.
- **Payload menu committed.** Home opens the BadUSB payload menu, which lists Drop & Upload and two disabled slots. Both firmware variants built and simulated navigation and arming checks passed at ba2a446.
- **60-second gauge committed.** The firmware uses manual ARM → INJECT. The countdown shows exact seconds alongside ten segments, fades from amber to red-orange, and pulses for the final six seconds. During injection, the OLED shows a spinner and progress bar.
- **USB diagnostic in review.** On 10 Nov 2026, the probe flashed and startup with the cable attached reported live frames and `IDLE`. Arvin reported a battery-only start with quiet frames, `CFG 0`, and address `0`. A fresh plug-in gave `ENUM + LIVE` at address `22`. Startup with the cable attached gave `IDLE` at address `23`, and `WATCH` gave `WAIT FOR QUIET`. Reconnect without a board restart and sleep or wake with the cable left attached remain pending.
- **Active waiting screen observed.** Arvin reported that arming on an existing USB connection kept the screen waiting. Arming on battery and then connecting USB returned to the disarmed payload screen with `USB seen (test)`. Those results cover the existing-link guard and first plug-in in the payload UI.

Setup, toolchain versions, and detailed test steps are in [README.md](README.md).

## System layout

| Component | Responsibility |
| --- | --- |
| USB TinyScreen+ and PC2 | On an owned, unlocked Windows lab PC, the Drop & Upload payload creates the fixed synthetic file in Notepad and uses the default browser to submit it to PC1. The local build address enables upload. The default build exercises the local save. |
| PC1 TraceHub | One Flask codebase and one local SQLite database. The loopback Ops Console refreshes the latest 50 receipts. The isolated-LAN browser listener accepts the fixed demo file. A separate authenticated HTTPS profile serves text-file clients. |
| Wi-Fi TinyScreen+ | Planned second node using the borrowed ASD2123-R Wi-Fi TinyShield. Send a bounded request directly to a mock IoT device on PC1 and show the matching result or timeout on the OLED. |

Use owned equipment, synthetic data, and an isolated lab network. Keep the fixed browser upload listener on the demo LAN. Keep credentials, database files, and machine-specific settings local.

TraceHub stores the decoded text, filename, size, server-assigned loot ID, observed peer IP, and UTC receipt time. An identical retry of the same browser form preserves its loot ID. The OLED reports keys sent. A stored TraceHub record confirms receipt.

## Requirements and evidence

| ID | Requirement | Current evidence and remaining check |
| --- | --- | --- |
| H01 | Require physical arming before either USB trigger. Consume the arm once. Cancel and expiry disarm the board. | **V01 partial.** One deliberate integrated run passed. Verify repeat, unarmed, cancelled, expired and held-INJECT cases on the current build. Validate Active separately before using it live. |
| H02 | Have the Wi-Fi node send one direct lab request and display its result or timeout. | **V02 planned.** Verify with the shield, the actual lab network and PC1 service, then stop the service and check the timeout. |
| H03 | Accept the fixed USB demo file and independently record the mock IoT request and outcome. | **V03 partial.** Browser upload and retry validation passed in Flask checks. The mock IoT endpoint and the combined live evidence check are pending. |
| H04 | Present real evidence with its source, identifier and server timestamp. | **V03 partial.** The browser receipt is visible in the console. Check the refreshed console, fresh IDs for separate attempts, and actual results from both paths. |
| H05 | Repeat both paths on the assessment setup and recover saved evidence after restarting TraceHub. | **V04 planned.** Have a second teammate follow the setup, repeat both paths, and verify that prior evidence is still visible after restart. |

Track build, simulation and physical results separately. Record the revision, tester, date and outcome for each physical check.

## Active work

| Work | Status | Exit check |
| --- | --- | --- |
| BadUSB payload menu | Review | On the OLED, open and back out of Drop & Upload, verify the disabled slots and held-button transitions, and run the configured demo once. |
| 60-second arming gauge | Review | Verify the `60s` to `59s` change, one-sixth of a segment draining per second from right to left, the final red-orange pulse through `01s`, cancel, and expiry on the OLED. |
| Passive and Active triggers | Diagnostic review | The payload opens in Passive and Disarmed. R1 changes mode while disarmed. Armed Active waits for a fresh USB observation, then reports a status-only result. Verify battery-powered reconnect and sleep or wake before enabling Active keyboard output. |
| Ops Console redesign | Planned | Make the latest receipt, source IP, time and content preview easy to scan in the dark terminal layout. Verify live updates and access to full file metadata. |
| Wi-Fi path and combined rehearsal | Planned after the BadUSB blocks | Implement the PC1 mock IoT endpoint and the direct board request. Rehearse both paths using the same setup for the video and live demo. |
| E04 hostname attribution | Deferred | Resolve browser source IPs through reverse DNS or a time-valid, operator-verified mapping. Preserve the original IP and receipt time. Keep these names and their provenance separate from client-reported hostnames already stored by the authenticated HTTPS receiver. Allow unknown names and test two known devices plus an unresolved device. |

Confirm team ownership and internal dates before the combined rehearsal. Record each member's actual contribution and the sources they used.

## Team workflow

- Use short descriptive branch names such as feat/badusb-payload-menu. Keep related edits together and review the diff and checks before committing.
- Keep main usable and review changes before merging. Use teammate review when team development resumes. Record setup changes and results in the README and progress in this plan.
- Keep local runtime data and credentials out of the repository. Preserve upstream licenses and attribute adapted work in source and video.

## Assessment and delivery

The assessment requirements come from ICT1011 Project Briefing (AY26), pages 2–8, and Assessment Rubrics, page 1. The four rubric categories are creativity, technical work, YouTube video, and demonstration. Each is worth five points. Individual marks also depend on peer assessment.

| ID | Requirement | Acceptance evidence |
| --- | --- | --- |
| A01 | Use TinyCircuits hardware for an innovative application; additional devices are permitted | Working application and documented architecture |
| A02 | Attribute reused code in source files and the video | Source notices and video citations reviewed |
| A03 | Produce a 2-3 minute YouTube video covering the application, working demonstration, distribution of work, and sources | Final duration and content checked |
| A04 | Submit source, project files, and documentation as a ZIP through the designated course dropbox; post the video through the designated course channel | Archive extracted and checked; submission confirmations recorded |
| A05 | Submit by 25 Nov 2026, 23:59 (Singapore time) | Submission confirmation |
| A06 | Demonstrate on 26 Nov 2026, Week 13, with behavior matching the video; all members attend as required | Rehearsal and live demonstration |

Show a fresh USB receipt and the tested Wi-Fi response in the video and live demo. Verify the extracted submission ZIP on a clean setup, and confirm the demo slot and any later course announcements before submitting.
