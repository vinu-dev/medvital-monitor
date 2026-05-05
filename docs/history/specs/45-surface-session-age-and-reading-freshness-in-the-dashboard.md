# Design Spec: Surface session age and reading freshness in the dashboard

Issue: `#45`  
Branch: `feature/45-surface-session-age-and-reading-freshness-in-the-dashboard`  
Spec path: `docs/history/specs/45-surface-session-age-and-reading-freshness-in-the-dashboard.md`

## Problem

The live dashboard shows the latest physiologic values and session reading
count, but it does not show how long the active session has been running or
how old the displayed reading is.

Current repo behavior leaves that gap visible:

- `src/gui_main.c:292` `paint_header()` shows the authenticated user, role, and
  simulation live/paused text only.
- `src/gui_main.c:333` `paint_patient_bar()` shows demographics, BMI, and
  `Readings: N / 10`, but no age or freshness context.
- `src/gui_main.c:680` `update_dashboard()` repopulates ordinal history rows
  and active alerts, but the dashboard has no time-based cue for whether the
  current screen is fresh, paused, or stale.
- `include/patient.h` and `src/patient.c` define the patient record as
  demographics plus bounded vital-sign history only; there is no timestamp or
  session-age concept in the domain record today.

That creates an interpretation risk identified in the risk note for `#45`:
operators may over-trust a static but normal-looking dashboard because the UI
does not make recency explicit.

## Goal

Add a narrow, read-only dashboard cue that lets a trained operator tell at a
glance:

- how long the current patient session has been active; and
- how recent the currently displayed reading is.

The MVP shall stay session-scoped and presentation-only. It shall not change
acquisition cadence, thresholds, NEWS2 scoring, alert generation, persistence,
export, or patient-record storage semantics.

## Non-goals

- No change to vital-sign thresholds, alarm limits, aggregate-alert logic, or
  NEWS2 scoring.
- No change to `PatientRecord`, stored `VitalSigns` payloads, or the meaning of
  `patient_latest_reading()`.
- No persistence of timestamps, freshness metadata, or session chronology to
  disk or external systems.
- No wall-clock synchronization, timezone handling, audit-log semantics, or
  claim of authoritative clinical chronology.
- No export, print, historical drill-down, trend-review, or remote-monitoring
  workflow.
- No change to authentication, authorization, CI, release, or localization
  architecture beyond any strings needed for the new dashboard labels.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The feature improves contextual understanding of the current dashboard state.
- It does not change physiologic interpretation or alarming.

User population:

- trained clinical users and internal demo/DVT operators already within the
  current intended user group.

Operating environment:

- one local Windows workstation running the existing single-process Win32 GUI;
- one active in-memory patient session;
- simulation mode today, with future HAL-backed live acquisition still routed
  through the same GUI session layer.

Foreseeable misuse:

- mistaking paused or stale data for a fresh live state;
- confusing session age with last-reading recency;
- assuming a displayed time cue proves sensor connectivity or clinical validity;
- interpreting a workstation clock display as audit-grade or synchronized time.

The design must reduce those misuse paths explicitly instead of relying on
training alone.

## Current Behavior

Relevant implementation details:

- `AppState` in `src/gui_main.c` stores the active `PatientRecord`, login
  state, simulation mode, and UI fonts, but no session-timing metadata.
- `do_admit()` and simulation startup paths call `patient_init()` and set
  `g_app.has_patient = 1`, but they do not record when the session began.
- `do_add_reading()`, scenario loading, and the `WM_TIMER` simulation path call
  `patient_add_reading()`, but they do not record when the last reading was
  added.
- `TIMER_SIM` fires every 2 seconds only when simulation mode is enabled and
  not paused. There is no always-on dashboard timer for age/freshness repaint.
- when simulation history reaches `MAX_READINGS`, the timer path resets the
  patient session by calling `patient_init()` again before appending the next
  reading. That means session-age state must reset cleanly with rollover.

Relevant current requirements:

- `UNS-010`, `SYS-011`, and `SWR-PAT-006` cover consolidated/latest summary
  behavior.
- `UNS-014`, `SYS-014`, and `SWR-GUI-003`/`SWR-GUI-004` cover dashboard
  rendering and data-entry workflow.
- `UNS-015`, `SYS-015`, and `SWR-GUI-010`/`SWR-GUI-011` cover live-feed and
  simulation-state behavior.

None of those approved requirements defines:

- a session-age display;
- a last-reading freshness display;
- paused/no-reading/stale wording semantics; or
- the time source/provenance for those cues.

## Proposed Change

### 1. Keep session-age and freshness state in the GUI layer

Implement the MVP entirely inside `src/gui_main.c` by extending `AppState` with
session-scoped, monotonic timing fields such as:

- session start tick;
- last-reading tick; and
- any small helper state needed to distinguish "no reading yet" from a recorded
  reading.

Do not add timestamps to `PatientRecord` or `VitalSigns` in this issue. The
risk note explicitly recommends keeping freshness metadata out of the clinical
core unless scope is intentionally widened under separate requirements.

### 2. Use monotonic relative time, not wall-clock time

The MVP should use a monotonic Windows tick source such as `GetTickCount64()`
to derive relative elapsed time for:

- `Session age`
- `Last reading`

This is preferred over `SYSTEMTIME` or any persisted wall-clock timestamp
because it avoids unsupported claims about:

- external time synchronization;
- timezone or locale authority;
- clock drift and workstation time changes; or
- audit/event chronology.

The displayed values should be relative and compact, for example:

- `Session age: 03m 12s`
- `Last reading: 4s ago`

Exact wording may vary, but the UI must make the two concepts visually distinct
so users do not confuse total session duration with data recency.

### 3. Add a lightweight dashboard age-refresh timer

Because current timer behavior only repaints automatically while simulation is
actively generating data, the MVP needs a small dashboard heartbeat separate
from acquisition. Add a lightweight UI timer that:

- runs while the dashboard window exists;
- invalidates the painted dashboard surfaces used for session age/freshness;
- does not append readings or alter acquisition cadence.

This is necessary so relative age text remains truthful during:

- manual/admit-before-reading workflows;
- paused simulation;
- stale/no-update states; and
- any dashboard session where no new reading arrives for several seconds.

The timer should repaint only what is needed for visual freshness context, not
re-run unnecessary data mutation.

### 4. Surface the new context in the existing header/patient-bar area

Keep the feature inside the already-established summary surfaces rather than
competing with the alert banner or vital tiles.

Recommended placement:

- add a compact `Session age` label in the header information block or the
  patient bar;
- add `Last reading` next to the existing `Readings: N / 10` summary in the
  patient bar;
- if a compact badge is used, it should communicate state such as
  `LIVE`, `PAUSED`, `STALE`, or `NO DATA` without overpowering alert colors.

The change should preserve current alert banner prominence and tile readability.

### 5. Define explicit freshness-state semantics

The dashboard should not show one generic freshness string for every state.
Instead, the MVP should use explicit state semantics derived from the current
session mode:

- `NO DATA`: patient/session exists but no reading has been captured yet.
- `LIVE`: automatic acquisition is active and the latest reading is within the
  expected refresh window.
- `PAUSED`: simulation is paused; the UI must say so explicitly and continue to
  make clear that no new readings are arriving.
- `STALE` or `NO UPDATE`: automatic live mode is expected, but the last reading
  age has exceeded the acceptable live-update window.
- `MANUAL` or equivalent neutral wording when automatic live acquisition is not
  active and the operator is viewing manually entered data.

For the MVP, a stale/no-update cue should be derived only from modes with a
known expected cadence, such as the current 2-second simulation timer path.
Do not apply a misleading fixed clinical freshness threshold to manual/device
mode where no automatic cadence is defined.

### 6. Reset timing state on every session boundary

Session-age and last-reading metadata must reset on every workflow that starts
or invalidates a patient session, including at least:

- dashboard startup when simulation auto-creates a patient session;
- `Admit / Refresh`;
- `Clear Session`;
- demo-scenario loading;
- simulation enable/disable transitions;
- logout;
- timer-driven session rollover when the bounded reading history is full.

If a session exists before the first reading arrives, session age may advance,
but `Last reading` must remain an explicit `No reading yet` state rather than a
green/normal freshness cue.

### 7. Stamp last-reading time on every successful reading append

Every path that successfully appends a new reading should update the
last-reading tick state, including:

- the initial simulation reading at dashboard creation;
- timer-driven simulation updates;
- manual `Add Reading`;
- demo-scenario insertion.

Scenario loading is a special case because the code currently appends several
historical-looking readings immediately in a loop. For this issue, treat those
as a synthetic session loaded "now"; do not widen scope by backfilling per-row
historical timestamps.

### 8. Leave the patient-domain and persistence contracts unchanged

The design should deliberately avoid changes to:

- `include/patient.h`
- `src/patient.c`
- `patient_print_summary()`
- any file format or config persistence

The issue is specifically about the live dashboard. Extending CLI/domain record
output with timing metadata would widen validation, traceability, and storage
scope beyond what the risk note recommends for this MVP.

### 9. Requirements updates

Add a new traceable requirement path rather than stretching existing dashboard
requirements beyond their approved meaning.

Recommended additions:

- `SYS-020 - Session age and reading freshness context`
- `SWR-GUI-013 - Dashboard session age and reading freshness display`

Recommended scope for `SYS-020`:

- the system shall display the elapsed age of the active patient session and
  the elapsed age of the most recent displayed reading in the graphical
  dashboard;
- the system shall distinguish no-reading, paused, and stale/no-update states
  explicitly;
- the display shall use session-scoped relative elapsed time suitable for the
  current local dashboard session.

Recommended scope for `SWR-GUI-013`:

- the dashboard shall capture session-start and latest-reading timing in the
  GUI session state using monotonic relative time;
- the dashboard shall render distinct `Session age` and `Last reading`
  indicators;
- the dashboard shall show explicit wording for `NO DATA`, `PAUSED`, and
  `STALE`/`NO UPDATE` states as applicable;
- timing state shall reset on admit, clear, logout, simulation-mode transition,
  and session rollover;
- the feature shall not alter vital classification, alert generation, NEWS2, or
  stored patient-record semantics.

No new UNS entry is required. Existing `UNS-010`, `UNS-014`, and `UNS-015`
provide adequate user-need anchors once the missing SYS/SWR-level semantics are
made explicit.

## Files Expected To Change

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- DVT or GUI-verification evidence files for the new freshness behavior

Possible validation/support files if implementation extracts testable helpers:

- a dedicated DVT checklist/protocol entry for session-age and freshness states
- optional targeted automated tests for pure formatting/state helpers, if the
  implementer factors them into a testable unit without widening scope

Files that should not change in this issue:

- `include/patient.h`
- `src/patient.c`
- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- persistence, CI, release, or export files

## Requirements And Traceability Impact

Existing traceability that remains valid:

- `UNS-010 -> SYS-011 -> SWR-PAT-006` for consolidated latest summary
- `UNS-014 -> SYS-014 -> SWR-GUI-003/SWR-GUI-004` for dashboard rendering
- `UNS-015 -> SYS-015 -> SWR-GUI-010/SWR-GUI-011` for live/simulation context

New traceability needed:

- `UNS-010`, `UNS-014`, `UNS-015 -> SYS-020`
- `SYS-020`, `SYS-014 -> SWR-GUI-013`
- `SWR-GUI-013 -> src/gui_main.c`
- `SWR-GUI-013 -> dedicated GUI/DVT verification evidence`

Do not broaden `SWR-PAT-003` or `SWR-PAT-006` to include time semantics. Those
requirements describe latest-reading access and summary output as they exist
today. Session freshness should be added as a new, explicit dashboard behavior.

## Medical-Safety, Security, And Privacy Impact

Medical-safety impact:

- indirect but real, because the feature changes how an operator judges whether
  displayed data are current;
- the main hazard is false reassurance from ambiguous or overconfident
  freshness wording.

Required safety controls for implementation:

- use relative elapsed time, not authoritative wall-clock claims;
- distinguish paused/no-data/stale states explicitly;
- keep freshness/session-age cues visually subordinate to alert colors and
  clinically meaningful status surfaces;
- reset timing state cleanly at all session boundaries.

This issue must not change:

- vital classification thresholds;
- alarm-limit behavior;
- NEWS2 scoring or response bands;
- stored reading payload semantics;
- authentication, security roles, or privacy/storage scope.

Security impact:

- no direct security-control change is expected.

Privacy impact:

- low for this MVP because it remains local and session-scoped;
- rises materially if future work adds persisted timestamps, exports, or remote
  synchronization, which are out of scope here.

## AI/ML Impact Assessment

This change does not add, modify, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing deterministic session state and vital values
- Output: deterministic UI text/rendering only
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged except for clearer display-state labeling
- Dataset and bias considerations: none
- Monitoring expectations: standard GUI regression and requirement verification
- PCCP impact: none

## Validation Plan

Validation should focus on freshness semantics and session-boundary behavior.

Required GUI/DVT verification scenarios:

- dashboard startup with simulation enabled and first reading present;
- patient admitted before first reading, confirming `No reading yet` handling;
- normal live updates, confirming `Session age` advances and `Last reading`
  resets on each new sample;
- paused simulation, confirming the UI says `PAUSED` explicitly and does not
  imply live updates;
- resumed simulation, confirming freshness returns to normal live behavior;
- manual/add-reading workflow with no automatic cadence, confirming neutral
  manual wording rather than false live freshness claims;
- clear-session and logout reset behavior;
- simulation disable/enable transitions;
- timer-driven session rollover when the bounded history resets.

Review expectations:

- confirm no change to alert generation, NEWS2, threshold logic, or patient
  record storage semantics;
- confirm the final requirement text explicitly defines freshness semantics and
  time source boundaries;
- confirm the new cue does not visually compete with the alert banner.

Recommended regression commands after implementation:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R "Patient|GUI|Trend"
python dvt/run_dvt.py
```

If implementation factors freshness-state or duration-formatting logic into a
small pure helper, add targeted automated tests for that helper. Otherwise,
dedicated DVT/manual evidence is mandatory; adjacent generic GUI-demo evidence
is not sufficient.

## Rollback Or Failure Handling

If implementation cannot make freshness context explicit without introducing
ambiguous time claims or widening the data model, revert to the current
dashboard behavior and return the issue for narrower redesign.

Stop and reopen design instead of expanding the MVP if implementation discovers
it needs any of the following:

- persisted or exported timestamps;
- wall-clock synchronization semantics;
- changes to `PatientRecord` or stored `VitalSigns`;
- audit/event chronology claims;
- broader historical review or trend-timeline workflows.
