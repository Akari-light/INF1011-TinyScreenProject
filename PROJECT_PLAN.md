# TraceDeck project plan

Last updated: 2026-11-10 (Asia/Singapore).

The team's central implementation plan. Keep requirements, tasks, responsibilities, milestones, decisions, and progress here. Use README.md for setup and operation.

## Current position

- The team selected **TraceDeck**, a pocket attack-and-deception lab. The vendor OLED baseline and the USB keyboard test in Windows Notepad passed. Arvin is developing TraceDeck solo for now.
- **Optional if time allows:** AuthenticConfig, the expanded honeypot + Rubber Ducky-style USB + IoT toolkit, browser-trusted HTTPS, and hostname attribution for BadUSB loot.
- Seven members have TinyScreen+ kits and laptops. One Wi-Fi TinyShield is already on loan. Owners and internal dates are pending. The verified USB baseline toolchain versions are in [README.md](README.md).
- The PC1 browser upload page is implemented. The TinyScreen+ BadUSB screen can emit the fixed Notepad save sequence. Physical validation of the save on PC2 is pending. Browser upload keystrokes and the integrated LAN check follow. Direct Wi-Fi and the mock IoT service follow later.

## Assignment baseline

Source: ICT1011 Project Briefing (AY26), pages 2-8. This summary is self-contained; check subsequent instructor announcements for changes.

| ID | Requirement | Acceptance evidence |
| --- | --- | --- |
| A01 | Use TinyCircuits hardware for an innovative application; additional devices are permitted | Working application and documented architecture |
| A02 | Attribute reused code in source files and the video | Source notices and video citations reviewed |
| A03 | Produce a 2-3 minute YouTube video covering the application, working demonstration, distribution of work, and sources | Final duration and content checked |
| A04 | Submit source, project files, and documentation as a ZIP through the designated course dropbox; post the video through the designated course channel | Archive extracted and checked; submission confirmations recorded |
| A05 | Submit by 25 Nov 2026, 23:59 (Singapore time) | Submission confirmation |
| A06 | Demonstrate on 26 Nov 2026, Week 13, with behavior matching the video; all members attend as required | Rehearsal and live demonstration |

The rubric gives 5 points each to creativity, technical work, YouTube video, and demonstration. Individual marks also depend on peer assessment. Record real contributions and reviews throughout the project. (Assessment Rubrics, p. 1. Project Briefing, p. 4.)

It specifies six-member proposed teams and permits an instructor-added member. Record the confirmed seven-person roster rather than inferring a new allocation.

## Product scope

**TraceDeck** lets students trace evidence from controlled USB and Wi-Fi interactions. In the USB base, a physically armed TinyScreen+ opens Windows Run on PC2, launches Notepad, types `hello world`, and saves `hello-world.txt` to the designated lab folder. The board then opens TraceHub's upload page in the existing browser and selects the saved file. The browser sends it to PC1 over the isolated LAN. The Ops Console shows the decoded text and loot metadata. In the later network path, a second TinyScreen+ sends a request over local Wi-Fi to the mock IoT service and displays the service result.

### Core requirements for team review

| ID | Requirement | Acceptance evidence |
| --- | --- | --- |
| H01 | The USB node starts unarmed and emits one fixed lab interaction only after explicit physical arming and execution. It returns to a safe state on completion, cancellation, or arming timeout. | The expected Notepad file is saved on the tested PC2 laptop. Unarmed, cancelled-before-execution, and expired attempts emit no keystrokes. |
| H02 | The network node sends a bounded request directly to the owned decoy and displays the actual matching result or a timeout/unknown state. | A live request and response agree with the service log and the OLED. With the service unavailable, the OLED reports the failure to confirm the result. |
| H03 | TraceHub accepts the fixed USB demo file through the PC2 browser while the demo listener is running. The mock IoT service simulates one device with dummy data and independently records Wi-Fi requests and outcomes. | Each deliberate USB trigger produces one loot entry with a source and timestamp. A direct Wi-Fi request produces the expected service result. |
| H04 | The dashboard shows saved evidence with its ID, source, and server timestamp. Board status distinguishes keys sent from a matching service response. | The manual control and repeated USB and Wi-Fi interactions produce distinct evidence. Retried submissions preserve their original loot IDs. |
| H05 | The team can repeat both paths on the assessment setup and recover the evidence after a server restart. | A second teammate follows setup, completes both paths, and verifies that the dashboard shows each new result and retains previous records. |

The core is one decoy, one fixed USB scenario, and one direct Wi-Fi scenario. Use team-owned equipment, an isolated local network, and synthetic data. The browser demo uses cleartext HTTP for its fixed file and sends no credential. TraceHub records the peer IP observed by PC1. HTTP logs show service interactions and the honeypot records them. The project scope excludes general payload scripting, shell commands, external targets, broad scanning, public honeypot hosting, real credential collection, and locked-session or privilege bypass.

### Possible extensions if time allows

Keep the core release runnable independently. Consider extensions only after M3, with both paths rehearsed and sufficient time reserved for verification, video, and submission. Include an extension in the video and live demo only when its acceptance checks pass.

- **E01 — AuthenticConfig.** The laptop issues a bounded, authenticated application profile. One TinyScreen+ validates the schema and parameter bounds, checks the HMAC, and applies a newer valid profile with a visible effect such as a changed label or request interval. Altered, out-of-bounds, or replayed profiles must leave the active profile unchanged. A borrowed ASD2202-R-E EEPROM is a candidate for retaining the accepted profile and version after a normal restart. Verify retention and rollback rejection after restart before claiming persistence. Invalid stored state must produce an explicit recovery condition. If storage is unavailable, demonstrate session-only version checking. [TinyCircuits EEPROM guide](https://learn.tinycircuits.com/Memory/EEPROM_TinyShield_Tutorial/).
- **E02 — Expanded honeypot + Rubber Ducky-style USB + IoT toolkit.** Reuse the core nodes, decoy, and event timeline. Add selectable, predefined lab scenarios with bounded USB actions and direct board-originated IoT checks. Start with one additional scenario, such as comparing how the same mock IoT control request is handled under permissive and guarded policies. The operator selects the preset, runs the relevant USB or Wi-Fi interaction, and sees the actual response and correlated evidence. Verify that the selected preset executes, both policy outcomes are observable, and the core scenario still runs after reset. Pick the final added scenario during extension review.
- **E03 — Browser-trusted HTTPS for BadUSB.** Use a hostname the team controls and resolve it to PC1's private address on the demo network. Obtain a publicly trusted certificate with DNS-01 validation so the upload service can stay reachable only on the LAN. Verify that the stock PC2 browser opens the page and uploads the demo file without a certificate warning or a custom trust installation. [Let's Encrypt DNS-01 validation](https://letsencrypt.org/docs/challenge-types/#dns-01-challenge).
- **E04 — Hostname attribution for loot.** Look up the observed source IP on the isolated demo network using reverse DNS or an operator-verified DHCP mapping. Store the hostname and how it was obtained alongside the original IP and receipt time. Use a nullable hostname field with `NULL` for an unknown or unverified name. When labeling older loot, use an address assignment valid at the original receipt time. Verify that two known devices resolve to their own names and that loot from an unresolved device still saves. [Microsoft reverse DNS documentation](https://learn.microsoft.com/en-us/windows-server/networking/dns/reverse-lookup).

## Architecture decisions

Working conventions and proposed TraceDeck architecture. Team review is pending.

| ID | Decision | Reason / revisit condition |
| --- | --- | --- |
| D01 | One repository for the complete system | Keep firmware, wiring, tests, and delivery information consistent at a revision |
| D02 | README.md plus PROJECT_PLAN.md | Separate project use from implementation coordination without extra planning files |
| D03 | Add firmware/, hardware/, tests/, and docs/ when content exists | Avoid empty scaffolding |
| D04 | Separate firmware entry point, application logic, and hardware access | Support focused changes and testing with ordinary modules |
| D05 | Verify the actual kit using its vendor-supported workflow before selecting the toolchain | Demonstrate board/library compatibility first |
| D06 | Short-lived task branches and reviewed pull requests into main | Integrate small changes and preserve review history |
| D07 | Track tasks here initially | Reconsider GitHub Issues only if this table becomes unwieldy |
| D08 | PC1 runs TraceHub. The USB node drives PC2's existing apps and browser through USB HID. The later Wi-Fi path uses a second TinyScreen+. | Use an unlocked laptop on the demo network without installing a helper. |
| D09 | Collect loot continuously and identify each saved entry by its server-assigned ID. | A submission ID identifies a retry of the same browser form. The authenticated API scopes its submission IDs to each enrolled helper. |
| D10 | Use one PC1 Flask codebase in `tracehub/` with SQLite and separate dashboard, fixed-file demo, and authenticated API profiles. | Keep the dashboard local to PC1, scope cleartext traffic to the synthetic demo, and require HTTPS for credentialed uploads. |

The integrated lab uses two ASM2022 TinyScreen+ boards, one borrowed ASD2123-R Wi-Fi TinyShield on the network node, PC1 and PC2, working USB data/power connections, and a reachable private 2.4 GHz network. The other kits are available for development. Leave the ST BLE shield off the Wi-Fi stack. Confirm the USB layout, Wi-Fi reachability, power, and venue setup early. Arduino Keyboard and WiFi101 are documented starting points. Record the exact versions after testing the actual hardware. [TinyScreen+ SAMD package](https://files.tinycircuits.com/ArduinoBoards/tinycircuits-samd-1.1.0.zip), [Arduino Keyboard](https://docs.arduino.cc/language-reference/en/functions/usb/Keyboard/), [TinyCircuits Wi-Fi guide](https://learn.tinycircuits.com/Communication/WiFi_TinyShield_Tutorial/).

Each new browser form gets a submission ID that it sends with the file. TraceHub saves each new loot entry with a unique loot ID, decoded text, filename, original byte count, the peer IP observed by PC1, and the UTC receipt time. The Ops Console uses the IP and time to distinguish browser sources in the base. Hostname lookup is optional under E04. Future Wi-Fi results retain their own evidence fields. C2 tasking follows the core lab.

Keep initialization and the execution loop small. Put rules, calculations, and state transitions in application modules; isolate vendor-library calls in hardware modules where practical. Prefer responsive timed updates when behavior requires them. Add classes, scheduling, or other layers only when justified.

Record toolchain, board-package, and dependency versions in shared configuration or setup instructions. Keep machine-specific ports and absolute paths out of shared build configuration. Preserve upstream licenses and record reused sources and modifications.

When a decision changes, retain its ID and record the date, replacement, reason, and participants in the log below.

## Responsibilities

Names and assignments require team agreement. Redistribute responsibilities as needed; everyone contributes technical work and reviews another person's work.

| Responsibility | Owner | Backup / reviewer |
| --- | --- | --- |
| Board controls, OLED, and arming state machine | TBD | TBD |
| USB keyboard sequence, cancellation, and host compatibility | TBD | TBD |
| Wi-Fi request handling, response parsing, and OLED feedback | TBD | TBD |
| Laptop IoT honeypot, synthetic state, and service logging | TBD | TBD |
| BadUSB browser upload, loot schema, and evidence correlation | TBD | TBD |
| Live dashboard and display of controls and errors | TBD | TBD |
| System integration, reset and failure testing, and demo rehearsal | TBD | TBD |

Each owner records their implementation, verification, and sources. Assign coordination, submission, and video editing alongside these technical packages.

## Milestones

Internal dates require agreement; assignment deadlines are recorded above.

| ID | Milestone | Exit condition | Owner | Target | Status |
| --- | --- | --- | --- | --- | --- |
| M0 | Foundation | Record inventory, collaboration workflow, and a successful baseline build/upload | TBD | TBD | In progress |
| M1 | TraceDeck scope review | Record the team selection and review H01–H05, interfaces, owners, and internal dates | TBD | TBD | Review |
| M2 | Hardware feasibility | Demonstrate one harmless USB marker and a separate board-to-service Wi-Fi round trip on the actual hardware | TBD | TBD | Todo |
| M3 | Core integration | Show USB loot and Wi-Fi decoy results in the live console, demonstrate board feedback, and repeat both paths. Review whether time remains for an extension. | TBD | TBD | Todo |
| M4 | Verified release | Pass the core checks and any checks for included extensions. A second teammate reproduces the demo setup. | TBD | TBD | Todo |
| M5 | Submission ready | Check the video, attribution, source archive, and live rehearsal against A01–A06 | TBD | Before 25 Nov deadline | Todo |

## Task tracker

Statuses: Todo, In progress, Blocked, Review, Done. Give each task one accountable owner, a reviewer, due date, and completion condition before implementation.

| ID | Task / completion condition | Owner / reviewer | Due | Dependencies | Status | Evidence / PR |
| --- | --- | --- | --- | --- | --- | --- |
| T01 | Review foundation and record agreement | TBD / TBD | TBD | - | Review | Initial documents prepared |
| T02 | Confirm board revisions, Wi-Fi loan access, data cables, and the test network | TBD / TBD | TBD | - | In progress | Kit models and one Wi-Fi loan reported |
| T03 | Verify vendor example build/upload; record exact versions and setup | TBD / TBD | TBD | T02 | Done | Flash verified and OLED loop observed by Arvin on Windows, 8 Nov 2026. See [README.md](README.md). |
| T04 | Configure GitHub collaboration; demonstrate a reviewed PR | TBD / TBD | TBD | T01 | Todo | - |
| T05 | Agree roster, responsibilities, and internal milestone dates | TBD / TBD | TBD | T01 | Todo | - |
| T06 | Record TraceDeck selection and review H01–H05 and the loot interface | TBD / TBD | TBD | T05 | Review | Team selected TraceDeck |
| T07 | Verify arming, cancellation, and timeout, then test the fixed marker over USB HID (H01) | TBD / TBD | TBD | T03, T06 | Done | SAFE startup, button mapping, state transitions, and the one-marker Notepad test verified by Arvin on Windows, 8 Nov 2026. See [README.md](README.md). |
| T08 | Verify direct Wi-Fi reachability, then implement the request and feedback path (H02) | TBD / TBD | TBD | T03, T06 | Todo | - |
| T09 | Implement the BadUSB browser upload, mock IoT decoy, and independent event records (H03) | TBD / TBD | TBD | T06 | In progress | PC1 browser upload ready for code review. |
| T10 | Integrate both nodes and build the live correlation dashboard (H04) | TBD / TBD | TBD | T07, T08, T09 | Todo | - |
| T11 | Verify the core, review the extension decision at M3, and rehearse the release (H05) | TBD / TBD | TBD | T10 | Todo | - |
| T12 | Record the working demo and package, extract, and check the submission (A01–A06) | TBD / TBD | TBD | T11 | Todo | - |

## Team workflow and completion rules

- Keep main usable. Use a task branch such as feat/<task-id>-<description>, fix/<task-id>-<description>, or docs/<task-id>-<description>.
- Make focused commits with descriptive messages. Describe problem, change, task ID, and validation in each pull request.
- Obtain at least one teammate's approval before merging. Configure branch protection and checks where available; this document does not enable them.
- Follow the chosen toolchain's naming conventions and shared editor settings. Coordinate edits to design files that cannot merge cleanly.
- Integrate regularly. Update this plan alongside changes to scope, status, or decisions.
- Keep required source, assets, configuration, and test evidence shared. Exclude generated caches, machine-specific settings, and credentials.
- Record actual contribution and review evidence; commit counts alone do not measure contribution.

A task is Done when its acceptance criteria pass, relevant checks and limitations are recorded, affected documentation is updated, and a teammate has reviewed the result (and it is merged where applicable).

During solo USB feasibility work, Arvin can close tasks after passing the checks and recording the results in the affected documentation. Teammate review applies when team development resumes.

## Verification

Use automated tests for independent logic, integration checks for connected modules, and physical acceptance checks for device behavior. Add automatic compilation once the board/toolchain is established. A successful build or simulation does not prove physical correctness.

| Test ID | Requirement ID | Procedure / hardware | Expected result | Actual result | Revision / tester / date |
| --- | --- | --- | --- | --- | --- |
| V01 | H01 | With the PC1 demo listener running and the laptop unlocked on the isolated network, try unarmed, deliberate, cancelled, expired, and held-INJECT board attempts | A deliberate injection visibly saves the expected Notepad file and produces one matching loot entry on PC1. Holding INJECT produces one entry. Unarmed, cancelled, and expired attempts produce no new loot. | Not run | - |
| V02 | H02 | Send a request over the owned network, then repeat with the decoy stopped | Matching live result when reachable and timeout/unknown when unavailable | Not run | - |
| V03 | H03, H04 | Run USB, Wi-Fi, and manual controls. Retry the same browser submission and attempt to upload a file with a wrong name or contents. | The console shows the recorded sources and times. Retrying returns the original loot ID. The wrong file creates no entry. | Not run | - |
| V04 | H05 | A second teammate sets up the system, runs both paths, restarts TraceHub, and repeats | Prior evidence stays available and new interactions produce distinct records. | Not run | - |

For the 2–3 minute video and live demo, show the physical USB trigger, the resulting app and honeypot evidence, then the direct Wi-Fi interaction and its OLED result. Include a brief manual control, show each member's contribution in the video, and cite sources. Generate fresh records in the live assessment and use the same tested configuration as the video.

Before submission, test an extracted archive on a clean setup. Check it contains everything needed to reproduce the demonstrated application; record the tested revision/tag and actual submission confirmation.

## Risks and open questions

| Item | Next action | Owner | Status |
| --- | --- | --- | --- |
| USB enumeration, focus, layout, or timing | Test one fixed host setup first. If HID remains unreliable, review a reduced direct Wi-Fi and honeypot demo with the team. | TBD | Open |
| Wi-Fi shield access or local client isolation | Test the actual loaned shield and private network early. If reachability remains unreliable, review a reduced USB and local decoy demo with the team. | TBD | Open |
| Missing or misleading event correlation | Use separate app and server records and test missing/stale data before claiming a service result. | TBD | Open |
| Toolchain compatibility and integration time | Record the vendor baseline and schedule short integration sessions using the one Wi-Fi shield. | TBD | Open |
| Optional work disrupts the core or storage is unavailable | Preserve a runnable core revision. Timebox extensions after M3 and defer work that cannot be verified before recording. | TBD | Open |
| Later course announcements | Confirm current instructions before submission | TBD | Open |

## Decision and progress log

| Date | Record | Evidence / participants |
| --- | --- | --- |
| 2026-10-29 | Foundation prepared; brainstorming and implementation remain open | Owner's requested structure; team review pending |
| 2026-11-08 | Team selected TraceDeck as the primary project. AuthenticConfig and the expanded honeypot + Rubber Ducky-style USB + IoT toolkit are possible extensions. | Team decision reported by Arvin, who confirmed the TraceDeck name. Core acceptance criteria, owners, and internal dates are pending team review. |
| 2026-11-10 | D08 records the two-PC USB base approved by Arvin on 9 Nov. D09 replaces the proposed run IDs and manual run controls with continuous loot collection. D10 records the TraceHub architecture. | Arvin approved implementation. Team review of the integrated design is pending. |
| 2026-11-10 | Arvin approved the visible Notepad sequence and HTTPS text-file upload. PC1 stores decoded text and metadata as loot. | The PC hosts TraceHub and the laptop is the target. Physical verification follows sender and firmware setup. |
| 2026-11-10 | Arvin approved the browser-driven USB demo on the isolated LAN. D08 now uses the stock PC2 browser, D09 covers browser retries, and D10 separates the HTTP demo from the authenticated HTTPS API. Browser-trusted HTTPS is E03. | The PC1 block is ready for review. Firmware and physical verification are pending. |
| 2026-11-10 | Arvin selected the observed IP as the source identifier for the browser demo. Hostname lookup and nullable hostname storage are optional under E04. | The unique loot ID and UTC receipt time are already stored. Firmware and physical verification are pending. |
| 2026-11-10 | The firmware adds the BadUSB menu and the fixed Notepad save sequence behind physical ARM and INJECT. The initial destination is `%TEMP%\hello-world.txt`. | TinyScreen+ build and flash checksum passed. PC2 save behavior and OLED checks are pending. Browser automation is the next firmware block. |
