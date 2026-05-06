# Design Spec: Add a non-clinical session note banner

Issue: #61
Branch: `feature/61-add-a-non-clinical-session-note-banner`
Spec path: `docs/history/specs/61-add-a-non-clinical-session-note-banner.md`

## Problem

The current product has no bounded way to attach short, operator-entered,
non-clinical context to the active monitoring session. The dashboard already
shows patient identity, live vitals, alerts, and reading history, and the CLI
summary path already prints a structured patient summary, but none of those
surfaces can carry a brief explanation such as transfer context, demo scenario
label, or a handoff cue.

That leaves two gaps:

- later reviewers can lose important session context that is not encoded in the
  vitals themselves
- operators may try to reuse clinical-looking fields or ad hoc verbal workarounds
  for information that should remain explicitly non-clinical

Because this is a medical-monitoring UI, the bigger risk is not missing storage
capacity. The bigger risk is that an unlabeled free-text field could be mistaken
for charting, a validated observation, or a system-generated clinical signal.

## Goal

Add one narrow, session-scoped note feature that lets an authenticated operator
attach a short non-clinical context string to the active monitoring session
without changing any vital-sign, alerting, NEWS2, alarm-limit, or authentication
behavior.

The intended outcome is:

- exactly one note per active session
- explicit "non-clinical" labeling everywhere the note is shown
- one controlled entry/edit path only
- display in a neutral session banner area and in the existing summary output
- clear reset rules so the note does not leak across patients or sessions

## Non-goals

- No clinical charting, diagnosis, treatment advice, or legal medical-record
  workflow.
- No multi-note history, per-reading annotations, comments, tagging, templates,
  or collaboration.
- No use of the note in alerts, NEWS2, triage, trends, alarm limits, summary
  status, or any recommendation logic.
- No persistence to `monitor.cfg`, `users.dat`, or any new disk file. This is
  active-session memory only.
- No changes to authentication, RBAC, vital thresholds, simulator sequencing,
  or requirements unrelated to the new note slice.
- No reuse of the existing alert banner or severity colors for note display.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact:

Allow an authenticated operator to attach one brief non-clinical context note to
the current monitoring session so later review or handoff is less ambiguous.

User population:

- trained clinical staff using the workstation GUI
- demo operators using simulation mode
- internal reviewers inspecting a session summary

Operating environment:

- the current Win32 desktop application
- local authenticated workstation sessions
- both simulation-backed and manually entered patient sessions

Foreseeable misuse:

- entering treatment advice, diagnosis, or quasi-charting text into the note
- assuming the note is system-generated or clinically validated
- allowing the note to remain attached after Admit / Refresh, Clear Session,
  logout, simulation-mode reset, or a new patient admission
- placing the note too close to the alert banner and causing false clinical
  significance
- including PHI or unnecessary identifying detail in a field intended only for
  short operational context

## Current behavior

Current repository behavior is well bounded but has no note path:

- `PatientRecord` in `include/patient.h` stores demographics and a fixed-size
  reading history only.
- `patient_init()` in `src/patient.c` zero-fills the entire record and therefore
  clears all session-scoped state whenever a new patient record is initialized.
- `paint_patient_bar()` in `src/gui_main.c` renders a single-line patient banner
  containing demographics, BMI, and reading count, but no operator context.
- `patient_print_summary()` in `src/patient.c` prints demographics, latest
  vitals, overall status, and active alerts only.
- `do_admit()`, `do_clear()`, logout, simulation startup, and simulation-mode
  disable paths all reset or replace the active `PatientRecord`.

There is one subtle existing behavior that matters to design:

- when the simulation timer sees a full reading buffer, `dash_proc()` calls
  `patient_init()` with the same demographics to clear readings and continue the
  active session

That rollover is not a true new session. If the note is session-scoped, the
design must preserve it across reading-buffer rollover while still clearing it
on real session-reset paths.

## Proposed change

Implement the feature as a narrow patient-record and GUI enhancement with the
following decisions.

### 1. Store the note in `PatientRecord`, not in alerting or config state

Add a bounded note field to the patient module, for example:

- `#define SESSION_NOTE_MAX_LEN 120`
- `char session_note[SESSION_NOTE_MAX_LEN + 1];`

This keeps the note session-scoped, static-memory-only, and clearly separate
from `VitalSigns`, alert records, and application configuration.

Recommended patient-layer API additions:

- `int patient_set_session_note(PatientRecord *rec, const char *text);`
- `const char *patient_get_session_note(const PatientRecord *rec);`

Behavior of the setter should be:

- enforce the 120-character cap
- guarantee null termination
- reject line breaks and control characters
- treat empty or whitespace-only input as "no note"
- avoid heap allocation

Putting the length and content checks in `patient.c` keeps validation central
and unit-testable instead of scattering note rules through Win32 handlers.

### 2. Use one controlled GUI entry path only

Add one dedicated note-entry affordance in the dashboard, not an always-editable
free-text field mixed into the vital-sign controls.

Recommended interaction:

- add a `Session Note...` button near the patient-admission controls
- disable the button until an active patient/session exists
- open a small modal dialog with one single-line edit control and Save / Cancel
- use that same dialog as the only path to create, update, or clear the note

This keeps the UI narrow, avoids a second inline edit surface, and reduces the
chance that operators confuse the note with a clinical input field.

Recommended owner choice:

- allow edits only through this same dialog rather than hard write-once after
  first save

That still satisfies the "one simple path" constraint while allowing harmless
correction of typos without introducing multiple edit affordances. If the human
owner later insists on write-once behavior, the same dialog path can simply be
disabled after the first save without changing the underlying storage model.

### 3. Display the saved note in a neutral session banner, not in the alert banner

Display the saved value as read-only text in the patient banner area rendered by
`paint_patient_bar()`, with an explicit label such as:

`Session note (non-clinical): <text>`

Presentation rules:

- use the neutral patient-bar visual language, not the green/amber/red alert
  palette
- keep the note visually separate from the rolling simulation status banner
- increase the patient-bar height as needed to support a second line
- truncate only at the storage boundary, not unpredictably at paint time

This design matches the issue intent of a visible session note/banner without
reusing the clinical status banner for non-clinical content.

### 4. Include the note in the existing summary output with the same label

When `patient_print_summary()` is called and the note is non-empty, print one
additional labeled line in the demographics/header section, for example:

`Session note (non-clinical): Transfer to bay 2`

Rules:

- omit the line entirely when no note exists
- never prefix it as an alert
- never fold it into "Overall Status" or active-alert text

This gives the issue a bounded summary-output path using the repo's existing
review summary function without expanding into a broader export feature.

### 5. Define exact reset and retention semantics

The note must clear on true new-session paths:

- manual `Admit / Refresh`
- `Clear Session`
- logout
- simulation-mode disable
- initial simulation autoadmit on dashboard startup
- demo-scenario patient replacement
- any future patient switch that calls `patient_init()` for a new session

The note must persist during the same session across:

- adding new readings
- pause / resume simulation
- window resize and repaint
- dashboard language refresh
- list refreshes and other non-session UI updates
- automatic reading-buffer rollover when the session continues with the same
  patient context

Implementation consequence:

- do not silently lose the note when the timer clears a full reading buffer
- either preserve and restore the note around the existing rollover
  `patient_init()` call, or introduce a bounded helper that clears readings
  while retaining session-scoped non-clinical context

### 6. Keep the note completely outside clinical logic

The note must not be consumed by or influence:

- `overall_alert_level()`
- `generate_alerts()`
- `news2_calculate()`
- alarm-limit configuration or checking
- trend extraction or trend direction logic
- authentication, RBAC, or account management
- simulation-mode configuration persistence

Implementation review should treat any diff in `src/vitals.c`, `src/alerts.c`,
`src/news2.c`, `src/alarm_limits.c`, or auth modules as out of scope unless it
is purely comment-level and justified.

## Files expected to change

Expected implementation files:

- `include/patient.h`
- `src/patient.c`
- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `tests/unit/test_patient.cpp`
- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected files to inspect and update only if they carry counts or maintenance
guidance that must stay accurate after the new requirement slice:

- `requirements/README.md`

Expected files to inspect but normally not modify:

- `docs/history/risk/61-add-a-non-clinical-session-note-banner.md`
- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`

Files that should not change:

- `src/gui_users.c`
- `src/gui_auth.c`
- `src/app_config.c`
- `.github/workflows/**`
- release scripts and installer files

## Requirements and traceability impact

This issue should add a new bounded requirements slice rather than piggybacking
on existing clinical display requirements.

Recommended requirement additions:

- add `UNS-017` for brief non-clinical session context
- add `SYS-020` for one short session-scoped non-clinical note, explicit
  labeling, reset semantics, and exclusion from clinical logic
- add `SWR-PAT-007` for note storage bounds, validation, retrieval, and
  summary-label behavior in the patient module
- add `SWR-GUI-013` for the single note-entry path and neutral patient-banner
  display in the Win32 GUI

Expected traceability impact:

- `UNS-017 -> SYS-020 -> SWR-PAT-007`
- `UNS-017 -> SYS-020 -> SWR-GUI-013`

Do not trace this feature through existing alert or summary requirements such as
`SYS-005`, `SYS-006`, or `SYS-011`. The point of this issue is to keep the note
explicitly non-clinical and independently governed.

Revision histories and total requirement counts in the touched requirements
documents should be updated factually to reflect the new UNS / SYS / SWR rows.

## Medical-safety, security, and privacy impact

Medical-safety impact is low in direct algorithmic terms and meaningful in
human-factors terms.

What does not change:

- no classification thresholds
- no alert generation logic
- no NEWS2 score calculation
- no alarm-limit behavior
- no recommendation or response-band behavior

What can still go wrong if the design is loose:

- the note could be mistaken for clinical documentation
- stale text could carry over to the wrong patient or session
- unlabeled or color-coded placement could imply alert significance
- summary inclusion without a clear label could look like official clinical
  output instead of operator context

Risk controls required by this design:

- explicit non-clinical label everywhere the note appears
- one short bounded field only
- no multiline or control-character input
- clear on all true session-reset paths
- preserve isolation from clinical logic and alert presentation

Security impact is limited and mostly about data integrity and UI governance:

- the feature introduces one new free-text input surface
- the input must remain bounded and single-line
- no new privilege level, account path, or file permission model is introduced

Privacy impact is small but real:

- operators could enter PHI or unnecessary identifying detail into the note
- the note therefore should remain session-memory only in this issue
- there should be no new disk persistence or background export path

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- No model is introduced.
- No model input or output path changes.
- No human-in-the-loop AI decision changes.
- No dataset, bias, monitoring, or PCCP implications apply.

## Validation plan

After implementation, validate three things: bounded scope, correct note
behavior, and no clinical-logic regression.

Build and automated verification:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Targeted text and diff inspection:

```powershell
rg -n "SESSION_NOTE_MAX_LEN|session_note|Session note|non-clinical" include src tests requirements
git diff --stat
```

Clinical-logic guardrail inspection:

```powershell
git diff -- src/vitals.c src/alerts.c src/news2.c src/alarm_limits.c src/gui_users.c src/gui_auth.c
```

Expected automated coverage additions:

- patient-module tests for valid note save and retrieval
- patient-module tests for overlength, newline, and control-character rejection
- patient-module tests confirming `patient_init()` clears the note
- patient-module tests confirming ordinary reading additions do not clear the note
- summary tests confirming the note line appears only when populated and is
  labeled as non-clinical

Manual GUI verification:

1. Admit a patient and confirm the note path is enabled only when a session is
   active.
2. Save a short valid note and confirm it appears in the patient banner with an
   explicit non-clinical label.
3. Re-open the same dialog and confirm the same single path is used for any
   edit or clear operation.
4. Confirm the note never appears in the alert banner, alert list, or severity
   badge styling.
5. Trigger `Clear Session`, logout, and patient re-admit flows and confirm the
   note is cleared.
6. Let the simulation reading buffer roll over and confirm the note is preserved
   when the patient session itself is continuing.
7. Confirm simulation-mode disable and fresh simulation startup do clear the
   note because those are new-session paths.
8. Confirm `patient_print_summary()` shows the note only when present and always
   labels it as non-clinical.

## Rollback or failure handling

If implementation cannot preserve the note across reading-buffer rollover
without an unexpectedly broad refactor, stop and add a bounded helper for
session-preserving reading reset rather than accepting silent note loss.

If the human owner rejects summary inclusion, remove only the summary-display
piece and keep the storage, labeling, and GUI-banner design intact. Do not
compensate by moving the note into the alert banner or another clinical-looking
surface.

If stakeholder expectations expand toward charting, durable persistence,
cross-session history, or clinical instructions, split that work into a new
issue with fresh risk review rather than broadening issue #61 in place.

If the requirements slice cannot be approved in time, do not implement code
under existing alert or summary requirements as a shortcut. Return the item to
design review until the non-clinical traceability path is explicit.
