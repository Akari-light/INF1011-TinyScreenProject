# Project plan

Last updated: 2026-10-29 (Asia/Singapore).

The team's central implementation plan. Keep requirements, tasks, responsibilities, milestones, decisions, and progress here; use README.md for setup and operation.

## Current position

- Foundation documents prepared for review; brainstorming and implementation have not started.
- Application, exact board, accessories, toolchain, and feature scope: TBD.
- Seven participants reported by the project owner; names and responsibilities: TBD.
- Next: review the foundation and inventory the hardware, then brainstorm.
- GitHub publication, automated checks, and review enforcement are not configured yet.

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

The briefing gives equal assessment weight to creativity, technical work, video, and demonstration. Individual marks also depend on peer assessment. Record real contributions and reviews throughout the project.

It specifies six-member proposed teams and permits an instructor-added member. Record the confirmed seven-person roster rather than inferring a new allocation.

## Product scope

Complete after brainstorming and team selection:

- Application, intended user, and intended outcome: TBD.
- Required inputs, outputs, and behavior: TBD.
- Hardware, power, memory, timing, budget, and connectivity constraints: TBD where applicable.
- Minimum working demonstration and required features: TBD.
- Optional features and explicit exclusions: TBD.

Give each selected requirement an ID, measurable behavior, verification method, and expected result. Link tasks and tests to those IDs. Ideas are not accepted requirements until selected.

## Architecture decisions

Initial working conventions selected for this foundation; team review pending.

| ID | Decision | Reason / revisit condition |
| --- | --- | --- |
| D01 | One repository for the complete system | Keep firmware, wiring, tests, and delivery information consistent at a revision |
| D02 | README.md plus PROJECT_PLAN.md | Separate project use from implementation coordination without extra planning files |
| D03 | Add firmware/, hardware/, tests/, and docs/ when content exists | Avoid empty scaffolding |
| D04 | Separate firmware entry point, application logic, and hardware access | Support focused changes and testing with ordinary modules |
| D05 | Verify the actual kit using its vendor-supported workflow before selecting the toolchain | Demonstrate board/library compatibility first |
| D06 | Short-lived task branches and reviewed pull requests into main | Integrate small changes and preserve review history |
| D07 | Track tasks here initially | Reconsider GitHub Issues only if this table becomes unwieldy |

Keep initialization and the execution loop small. Put rules, calculations, and state transitions in application modules; isolate vendor-library calls in hardware modules where practical. Prefer responsive timed updates when behavior requires them. Add classes, scheduling, or other layers only when justified.

Record toolchain, board-package, and dependency versions in shared configuration or setup instructions. Keep machine-specific ports and absolute paths out of shared build configuration. Preserve upstream licenses and record reused sources and modifications.

When a decision changes, retain its ID and record the date, replacement, reason, and participants in the log below.

## Responsibilities

Names and assignments require team agreement. Redistribute responsibilities as needed; everyone contributes technical work and reviews another person's work.

| Responsibility | Owner | Backup / reviewer |
| --- | --- | --- |
| Coordination, scope, and milestones | TBD | TBD |
| System architecture and interfaces | TBD | TBD |
| Implementation area 1 | TBD | TBD |
| Implementation area 2 | TBD | TBD |
| Verification and defects | TBD | TBD |
| Documentation and attribution | TBD | TBD |
| Integration, video, and demonstration | TBD | TBD |

## Milestones

Internal dates require agreement; assignment deadlines are recorded above.

| ID | Milestone | Exit condition | Owner | Target | Status |
| --- | --- | --- | --- | --- | --- |
| M0 | Foundation | Documents reviewed; inventory, workflow, and baseline toolchain recorded | TBD | TBD | In progress |
| M1 | Scope agreed | Application selected; behavior and acceptance criteria reviewed | TBD | TBD | Todo |
| M2 | First working version | Smallest end-to-end behavior runs on the actual device | TBD | TBD | Todo |
| M3 | Feature complete | Required behavior integrated; optional scope decided | TBD | TBD | Todo |
| M4 | Verified | Acceptance checks pass; second teammate reproduces setup | TBD | TBD | Todo |
| M5 | Submission ready | Video, attribution, archive, and rehearsal checked | TBD | Before 25 Nov deadline | Todo |

## Task tracker

Statuses: Todo, In progress, Blocked, Review, Done. Give each task one accountable owner, a reviewer, due date, and completion condition before implementation.

| ID | Task / completion condition | Owner / reviewer | Due | Dependencies | Status | Evidence / PR |
| --- | --- | --- | --- | --- | --- | --- |
| T01 | Review foundation and record agreement | TBD / TBD | TBD | - | Review | Initial documents prepared |
| T02 | Record board, revisions, accessories, and available hardware | TBD / TBD | TBD | - | Todo | - |
| T03 | Verify vendor example build/upload; record exact versions and setup | TBD / TBD | TBD | T02 | Todo | - |
| T04 | Configure GitHub collaboration; demonstrate a reviewed PR | TBD / TBD | TBD | T01 | Todo | - |
| T05 | Agree roster, responsibilities, and internal milestone dates | TBD / TBD | TBD | T01 | Todo | - |
| T06 | Brainstorm, select application, and record requirements | TBD / TBD | TBD | T01, T02 | Todo | Not started |

## Team workflow and completion rules

- Keep main usable. Use a task branch such as feat/<task-id>-<description>, fix/<task-id>-<description>, or docs/<task-id>-<description>.
- Make focused commits with descriptive messages. Describe problem, change, task ID, and validation in each pull request.
- Obtain at least one teammate's approval before merging. Configure branch protection and checks where available; this document does not enable them.
- Follow the chosen toolchain's naming conventions and shared editor settings. Coordinate edits to design files that cannot merge cleanly.
- Integrate regularly. Update this plan alongside changes to scope, status, or decisions.
- Keep required source, assets, configuration, and test evidence shared. Exclude generated caches, machine-specific settings, and credentials.
- Record actual contribution and review evidence; commit counts alone do not measure contribution.

A task is Done when its acceptance criteria pass, relevant checks and limitations are recorded, affected documentation is updated, and a teammate has reviewed the result (and it is merged where applicable).

## Verification

Use automated tests for independent logic, integration checks for connected modules, and physical acceptance checks for device behavior. Add automatic compilation once the board/toolchain is established. A successful build or simulation does not prove physical correctness.

| Test ID | Requirement ID | Procedure / hardware | Expected result | Actual result | Revision / tester / date |
| --- | --- | --- | --- | --- | --- |
| TBD | TBD | Define after scope selection | TBD | Not run | - |

Before submission, test an extracted archive on a clean setup. Check it contains everything needed to reproduce the demonstrated application; record the tested revision/tag and actual submission confirmation.

## Risks and open questions

| Item | Next action | Owner | Status |
| --- | --- | --- | --- |
| Kit and peripherals unknown | Inventory physical hardware | TBD | Open |
| Toolchain compatibility unverified | Build/upload vendor example | TBD | Open |
| Application and interfaces undecided | Select scope after foundation review | TBD | Open |
| Shared hardware access and integration time | Agree access schedule and early integration | TBD | Open |
| Later course announcements | Confirm current instructions before submission | TBD | Open |

## Decision and progress log

| Date | Record | Evidence / participants |
| --- | --- | --- |
| 2026-10-29 | Foundation prepared; brainstorming and implementation remain open | Owner's requested structure; team review pending |
