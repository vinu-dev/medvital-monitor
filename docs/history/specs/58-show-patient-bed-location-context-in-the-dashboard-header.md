# Design Spec: Show patient bed/location context in the dashboard header

Issue: #58
Branch: `feature/58-show-patient-bed-location-context-in-the-dashboard-header`
Spec path: `docs/history/specs/58-show-patient-bed-location-context-in-the-dashboard-header.md`

## Problem

Issue #58 identifies a patient-context gap in the current dashboard. The
product shows patient name, ID, age, BMI, reading count, and live vitals, but
it does not show any bedside context such as bed or room/bed. That omission can
slow operator confirmation when a workstation is reused, a patient is moved, or
staff need to confirm that the displayed dashboard belongs to the expected
bedside context.

This is not a clinical-threshold or monitoring-logic defect. The risk is
indirect: if bedside context is absent, operators have one less cue for
confirming they are viewing the intended patient context; if it is added
poorly, a stale or ambiguous value could create false confidence.

## Goal

Add one narrow, display-only bedside-context capability for the active patient:

- allow an optional manually entered `Bed/Location` value during admit/refresh
- show that value in the existing patient-context header surface
- include the same value in the existing patient summary output
- keep the value informational only, with no effect on alerts, NEWS2, trends,
  alarm limits, authentication, persistence, or workflow automation

The intended outcome is a small UI/data-model addition that improves operator
orientation without changing any clinical behavior.

## Non-goals

- No change to physiologic classification, alert generation, aggregate alert
  status, NEWS2 scoring, trend logic, alarm limits, authentication, or role
  behavior.
- No ADT, EHR, RTLS, telemetry-assignment, bed-management, or transport
  integration.
- No persistence of bed/location to disk, config, export, or network payload.
- No new GUI summary panel, census view, multi-bed view, iconography, or
  competitor-style workflow.
- No use of bed/location in alert text, status banners, reading-history rows,
  logs, routing, or any computed output.
- No production-code changes in this Architect handoff; this spec defines the
  implementation scope only.

## Intended Use Impact, User Population, Operating Environment, and Foreseeable Misuse

- Intended use impact: trained clinical users gain an additional contextual cue
  for confirming the active bedside context of a single monitored patient.
- User population: bedside clinicians, ward nurses, and other trained operators
  already covered by the approved user needs.
- Operating environment: Windows workstations used in ward, bedside, and
  transport-staging contexts where a station may be reused for different
  patients.
- Foreseeable misuse:
  - a user treats the field as verified patient identity rather than
    informational context
  - a stale value survives patient-context changes and is trusted incorrectly
  - a typo or ambiguous abbreviation points to the wrong bed
  - an overlong value truncates into an ambiguous label
  - a later change reuses the field for workflow or routing logic without
    renewed risk review

## Current behavior

Current patient-context behavior is split across three relevant areas:

- `PatientInfo` in `include/patient.h` stores only ID, name, age, weight, and
  height.
- `create_dash_controls()` and `do_admit()` in `src/gui_main.c` collect only
  those existing patient demographics before calling `patient_init()`.
- `paint_patient_bar()` in `src/gui_main.c` is the current active-patient
  context surface beneath the navy header. It renders name, ID, age, BMI, and
  reading count, but no bedside context.
- `patient_print_summary()` in `src/patient.c` prints name, ID, age, BMI,
  readings, latest vitals, and alerts, but no bedside context.

Lifecycle behavior already relevant to stale-data control:

- `do_admit()` creates a fresh patient record.
- `do_clear()` zeroes the patient record and restores default form values.
- logout zeroes the patient record and destroys the dashboard.
- `apply_sim_mode()`, dashboard `WM_CREATE`, and demo scenarios create fresh
  patient contexts automatically.
- the `WM_TIMER` history-wrap path reinitializes the same patient record when
  the reading buffer fills; that path preserves patient identity fields today.

## Proposed change

Implement issue #58 as one bounded patient-context field with the following
design decisions.

### 1. Canonical field semantics

Use one canonical field named `bed_location` in the patient data model and one
user-facing label `Bed/Location`.

This field represents a short manually entered bedside context token such as:

- `Bed 12`
- `Ward A / Bed 12`
- `Room 3-B`

It is not a verified identity attribute, transport state, caregiver assignment,
or integration key.

### 2. Data-model contract

Extend `PatientInfo` with a bounded static string:

- recommended storage: `char bed_location[32]`
- maximum stored length: 31 visible characters plus null terminator
- initialization rule: default to empty string
- defensive copy rule: null-terminate always, consistent with existing
  `PatientInfo.name` handling

Preserve the repository's static-allocation constraint. No heap allocation or
indirect ownership should be introduced.

### 3. Input and validation behavior

Add one optional `Bed/Location` edit control to the existing dashboard admit
surface in `create_dash_controls()`. Keep the edit control in the same patient
demographics workflow as ID, name, age, weight, and height; do not introduce a
separate dialog or settings workflow.

Validation rules:

- blank is allowed
- stored blank value is the empty string, not `"Unknown"` or any fabricated
  location token
- GUI input should accept only printable ASCII bedside-context characters:
  letters, digits, spaces, `/`, and `-`
- GUI input longer than 31 characters should be rejected with the same
  `MessageBox` validation pattern already used by `do_admit()`, rather than
  silently truncated at the UI boundary

At the data-model boundary, `patient_init()` should still defensively bound and
null-terminate the field in case a non-GUI caller passes a longer string.

### 4. Read-only display surfaces

Use the existing patient bar rendered by `paint_patient_bar()` as the
dashboard-header context surface for this feature. Do not overload the top navy
navigation chrome in `paint_header()` with patient bed/location text.

Required display surfaces:

- dashboard patient bar in `paint_patient_bar()`
- patient summary output in `patient_print_summary()`

Out-of-scope display surfaces:

- `paint_header()`
- status banner
- alert list
- reading-history list
- vital-sign tiles
- logs or exports

Recommended blank-state rendering on read-only surfaces:

- show `Bed/Location: Not entered` when the stored value is blank
- keep the placeholder visually neutral and informational only

Recommended populated rendering:

- patient bar appends `Bed/Location: <value>` to the existing patient-context
  string
- patient summary prints a dedicated `Bed/Location` line alongside the other
  demographics

No new GUI summary panel is needed. The existing summary surface is
`patient_print_summary()`.

### 5. Lifecycle and stale-data controls

Carry the risk note's stale-data controls into concrete lifecycle behavior:

- `do_admit()` shall overwrite the stored `bed_location` with the current edit
  control value every time a patient is admitted/refreshed
- `do_clear()` shall zero the stored `bed_location` and reset the edit control
  to blank
- logout shall zero the stored `bed_location`
- dashboard startup auto-patient creation in `WM_CREATE` shall initialize
  `bed_location` as blank
- `apply_sim_mode()` auto-patient creation shall initialize `bed_location` as
  blank
- demo scenarios in `do_scenario()` shall initialize `bed_location` as blank

The timer-driven history-wrap path is different: when `WM_TIMER` reinitializes
the same patient after `MAX_READINGS`, it should preserve the current
`bed_location`, because that path continues the same patient context rather than
creating a new one.

### 6. Layout and localization scope

Because the existing dashboard uses fixed-position controls and the patient bar
already serves as the patient-context header surface, this issue should avoid a
layout refactor. The implementation should add the new field within the
existing admit controls and keep all other dashboard surfaces unchanged.

If the GUI uses localization for labels, add a new localized label string for
`Bed/Location` and a neutral placeholder string for `Not entered` rather than
hard-coding new display text into multiple branches.

### 7. Explicit exclusions

Implementation shall not:

- persist bed/location in `monitor.cfg` or any other file
- add the field to alert messages, status banners, or reading-history rows
- couple the field to NEWS2, alarms, or reading acceptance logic
- infer a location automatically from simulation state or hardware mode
- claim that the software confirms patient identity or physical device location

## Files expected to change

Expected implementation files:

- `include/patient.h`
- `src/patient.c`
- `src/gui_main.c`

Expected implementation files if localization is kept consistent with the rest
of the GUI:

- `include/localization.h`
- `src/localization.c`

Expected automated test files:

- `tests/unit/test_patient.cpp`

Expected documentation and traceability files before merge:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Files to inspect but not modify unless implementation reveals a direct need:

- `docs/history/risk/58-show-patient-bed-location-context-in-the-dashboard-header.md`
- `tests/unit/test_localization.cpp`

Files that should not change for this issue:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/trend.c`
- `src/gui_auth.c`
- `src/gui_users.c`
- `.github/workflows/**`

## Requirements and traceability impact

This issue adds traceable patient-context behavior and should not ship as an
undocumented UI tweak.

Recommended requirement strategy:

- extend `UNS-008` so the patient-association story can include optional
  bedside context as non-authoritative metadata
- extend `SYS-008` to store optional bounded `Bed/Location` context with the
  patient record
- extend `SYS-011` so patient summary output includes bedside context
- extend `SYS-014` so the GUI supports entry and display of the optional
  bedside-context field in the patient-context header surface

Recommended software-requirement impact:

- extend `SWR-PAT-001` for initialization/copy/null-termination of
  `bed_location`
- extend `SWR-PAT-006` so `patient_print_summary()` includes the new field
- extend `SWR-GUI-004` for optional `Bed/Location` entry, validation, and
  admit behavior
- extend `SWR-GUI-003` or add one narrow GUI requirement covering display of
  bedside context in the patient bar if the existing status-display requirement
  becomes too overloaded

Traceability matrix updates must be included before merge so the new data-model
and UI behavior map cleanly from UNS through SYS/SWR to implementation and test
evidence.

## Medical-safety, security, and privacy impact

Medical-safety impact is low but real. The feature does not alter physiologic
calculations or alerting, but it changes safety-relevant patient context shown
near identity information. The main hazard is stale or ambiguous bedside
context creating false confidence during patient review.

Required safety controls from this design:

- keep the field informational only
- make blank state explicit and neutral
- clear the field on fresh patient-context creation paths
- prevent the field from entering computed clinical outputs
- reject overlong GUI input rather than silently accepting ambiguous display

Security impact is low. No new privilege boundary, credential path, or network
surface is introduced.

Privacy impact is moderate but bounded because bed/location is patient-linked
operational data. Scope remains acceptable for this MVP only because the field
stays in-memory and on the intended patient-context displays, and is excluded
from logs, exports, alerts, and transport/integration features.

## AI/ML impact assessment

This change does not add, remove, modify, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond ordinary manually entered patient context text
- Output: none beyond static UI text rendering
- Human-in-the-loop limits: not applicable
- Transparency needs: standard UI labeling only; no AI disclosure required
- Dataset and bias considerations: none
- Monitoring expectations: none beyond normal regression verification
- PCCP impact: none

## Validation plan

Use the issue's recommended build/test commands plus focused automated and
manual verification for the new bounded field.

Primary regression commands:

```powershell
build.bat
run_tests.bat
ctest --test-dir build --output-on-failure
```

Required automated-test updates:

- extend `PatientInit.*` coverage in `tests/unit/test_patient.cpp` to verify
  blank initialization and bounded copy behavior for `bed_location`
- strengthen `PatientPrintSummary.*` coverage so it verifies rendered
  bed/location output, not only no-crash execution
- add or extend validation-focused tests if the implementation introduces a
  helper for allowed-character or max-length checks

Required manual GUI verification:

- admit a patient with blank bed/location and verify the patient bar shows
  `Not entered`
- admit a patient with a normal short value such as `Bed 12`
- admit a patient with a near-limit value and verify stable rendering
- try an over-limit value and verify admit is rejected with a clear error
- admit a second patient after a first one and verify no stale carry-over
- use `Clear Session` and verify the value is cleared from both data model and
  edit control
- logout and log back in; verify no stale value remains
- enable simulation when no patient exists and verify the auto-created patient
  starts with blank bed/location
- run both demo scenarios and verify they do not inherit a prior patient's
  location value
- allow the reading buffer to wrap and verify the same patient's current
  bed/location persists through the history reset
- confirm alerts, NEWS2 score, alarm limits, status banner, and reading-history
  behavior remain unchanged

Traceability verification before merge:

- update UNS/SYS/SWR/RTM documents on the implementation branch
- confirm the new requirement text maps to automated or approved manual
  verification evidence

## Rollback or failure handling

If implementation discovers that the requested value must be persisted, used in
exports, synchronized externally, or split into multiple workflow fields, stop
and raise a follow-up issue rather than silently broadening this MVP.

If the existing admit-form layout cannot absorb the new optional field without
breaking usability, prefer a narrow layout adjustment within the existing admit
surface. Do not turn this issue into a dashboard redesign.

If product-owner direction later changes the canonical semantics from
`Bed/Location` to a different single field such as `Room/Bed`, the data-model
and traceability wording may be renamed, but the safety boundaries in this spec
should remain unchanged.
