# Design Spec: Show latest-reading age in the dashboard header

Issue: #72
Branch: `feature/72-show-latest-reading-age-in-the-dashboard-header`
Spec path: `docs/history/specs/72-show-latest-reading-age-in-the-dashboard-header.md`

## Problem

Issue #72 identifies a live-dashboard ambiguity: operators can see the latest
vital values and whether simulation is live or paused, but they cannot tell at
a glance how old the currently displayed reading is. Today the UI exposes
`* SIM LIVE` / `SIM PAUSED`, the patient summary bar, and the latest tiles, but
it does not show when the current reading was last accepted into the session.

That gap is small but meaningful. A paused or delayed screen can be mistaken
for current data even though the monitor is only repainting stale values. The
feature must reduce that ambiguity without changing thresholds, alert logic,
NEWS2, missing-reading handling, HAL behavior, or any clinical claim.

## Goal

Add one passive, deterministic freshness cue to the dashboard header that tells
the operator how long ago the latest reading was accepted.

The intended outcome is:

- the header shows a small read-only cue such as `Last update: 4 s ago`
- the cue is derived from accepted-reading events, not from repaint timing
- paused sessions remain visibly paused and continue to show growing age
- no domain structs, alert semantics, NEWS2 logic, or persistence rules change

For this MVP, staleness is communicated by the age value itself. The design
does not add a separate stale-data threshold, alarm state, or connectivity
claim.

## Non-goals

- No change to vital-sign classification, aggregate alerting, NEWS2 scoring,
  alarm limits, authentication, localization selection, or patient persistence.
- No absolute wall-clock timestamp display, locale-specific time formatting, or
  time-of-day provenance claim.
- No new stale-data alarm, workflow lockout, device-fault detection, or sensor
  connectivity indicator.
- No change to `PatientRecord`, `VitalSigns`, `patient_latest_reading()`, or
  HAL acquisition APIs for this MVP.
- No persistence, export, audit-log, networking, or requirements-free scope
  expansion beyond the bounded freshness cue.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact is limited to display ergonomics. The cue helps trained
operators judge whether the visible reading is recent enough to treat as the
latest accepted sample, but it does not interpret patient condition.

User population remains the same: trained clinical and admin users of the Win32
dashboard. The operating environment remains the current Windows desktop
application, primarily in simulation mode today, with the same future HAL-backed
device mode already documented elsewhere.

Foreseeable misuse that the implementation must avoid:

- treating the cue as an alarm or proof of data validity
- assuming the cue proves hardware connectivity or trusted clock sync
- confusing a paused feed with a clinically stable patient
- interpreting a large age value as a formal stale-data threshold when no such
  threshold is approved in this issue

## Current behavior

Current dashboard behavior in `src/gui_main.c` is close to what this feature
needs, but it lacks freshness state:

- `paint_header()` renders the title, logged-in user badge, and the simulation
  state text (`* SIM LIVE` or `SIM PAUSED`)
- `paint_patient_bar()` renders patient demographics, BMI, and reading count
- `update_dashboard()` rebuilds reading history and active-alert lists from the
  latest accepted `PatientRecord` sample
- accepted readings enter the session through `patient_add_reading()` call
  sites in `do_add_reading()`, `do_scenario()`, `apply_sim_mode()`, initial
  dashboard startup, and the `WM_TIMER` simulation loop
- paused mode currently prevents new readings, but it also prevents the visible
  age from changing because `WM_TIMER` exits early when `g_app.sim_paused == 1`
- `PatientRecord` and `VitalSigns` contain no timestamp field today

## Proposed change

Implement the feature as a presentation-layer freshness model with no domain
data-model change.

### 1. Record freshness in GUI state only

Add bounded runtime state to `AppState` in `src/gui_main.c`:

- `ULONGLONG last_reading_tick_ms`
- `int has_last_reading_tick`

Capture the timestamp with `GetTickCount64()` only when a reading is actually
accepted into the current session. Do not add timestamp members to
`VitalSigns` or `PatientRecord`, and do not persist freshness across launches.

### 2. Stamp the accepted-reading boundary, not repaint

Create one shared GUI-side helper path, for example
`accept_reading_and_stamp(...)`, that calls `patient_add_reading()` and updates
`last_reading_tick_ms` only on success.

Use that path for every accepted-reading source:

- initial simulation reading during dashboard startup
- `apply_sim_mode()` when simulation is enabled
- `WM_TIMER` when a live simulated reading is acquired
- manual `Add Reading`
- demo scenario injection after the final sample becomes current

Clear the freshness state when the current session is reset or discarded:

- `do_admit()` before any new reading is added
- `do_clear()`
- device-mode transition that zeroes the patient session
- logout

### 3. Keep the age calculation testable outside Win32 paint code

Add a small pure helper dedicated to freshness-state calculation, for example:

- `include/dashboard_freshness.h`
- `src/dashboard_freshness.c`
- `tests/unit/test_dashboard_freshness.cpp`

That helper should stay outside `monitor_lib` because it is presentation logic,
not domain logic. It should accept current monotonic tick, last accepted tick,
and dashboard mode flags, then return a small model such as:

- display state enum: `DEVICE_MODE`, `NO_READING`, `LIVE_AGE`, `PAUSED_AGE`
- whole-second age for the latest reading when applicable

Formatting may remain in `gui_main.c`, but the state and age computation should
be deterministic and unit-testable.

### 4. Render one neutral header cue

Render the freshness cue as a secondary header line under the app title on the
left side of the top banner, using neutral text styling that does not reuse the
green/amber/red clinical alert palette.

Approved MVP semantics:

- simulation disabled: no freshness cue; preserve existing device-mode text
- simulation enabled, no reading yet: `Last update: awaiting first reading`
- simulation enabled, actively updating: `Last update: <N> s ago`
- simulation enabled, paused: `Feed paused - last update: <N> s ago`

For this issue, use age-only wording. Do not display wall-clock time, and do
not add a separate stale threshold or alarm color. If the age grows large, that
age itself is the operator-visible stale signal.

### 5. Allow paused mode to keep aging without acquiring new data

Retain the existing 2-second timer while simulation mode is enabled, but split
timer behavior into:

- acquisition path when `sim_paused == 0`
- repaint/age refresh path when `sim_paused == 1`

That means paused mode should still invalidate or refresh the dashboard so the
freshness cue can change from `4 s ago` to `6 s ago` to `8 s ago`, while no new
sample is acquired and no patient data changes underneath it.

Do not introduce a second scheduler, background worker, or persisted clock
mechanism for this MVP.

### 6. Localize the new header strings

Route the cue text through the existing localization layer rather than adding
hard-coded English-only header text. The simplest acceptable shape is a small
set of new string IDs and format strings for:

- `Last update: awaiting first reading`
- `Last update: %lu s ago`
- `Feed paused - last update: %lu s ago`

Keep the output static-memory safe by formatting into caller-owned stack
buffers, matching existing UI patterns.

## Files expected to change

Expected production/UI files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`

Expected new helper and test files:

- `include/dashboard_freshness.h`
- `src/dashboard_freshness.c`
- `tests/unit/test_dashboard_freshness.cpp`

Expected documentation/traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `docs/history/specs/72-show-latest-reading-age-in-the-dashboard-header.md`

Existing related design-control input already present on the branch:

- `docs/history/risk/72-show-latest-reading-age-in-the-dashboard-header.md`

Files that should not change for this issue:

- `include/patient.h`
- `src/patient.c`
- `include/vitals.h`
- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- alarm-limit logic, authentication logic, or HAL interface contracts

## Requirements and traceability impact

This is a new patient-facing dashboard behavior, so traceability should be made
explicit rather than implied.

Recommended requirement update strategy:

- amend `SYS-014` to mention a passive freshness cue for the latest accepted
  reading in the graphical dashboard
- add a new software requirement, using the next available GUI identifier (`SWR-GUI-014` on the rebased branch), that defines:
  accepted-reading-boundary semantics, age-only display wording, no-reading and
  paused states, monotonic timing basis, and no effect on alerting/NEWS2
- add RTM coverage linking the new SWR to `UNS-014`, `UNS-015`, and `SYS-014`
- trace implementation to `src/gui_main.c` plus the new freshness helper, and
  trace verification to new unit tests plus manual GUI review

This feature should not introduce a new SYS or SWR tied to alarms, device
health, or clinical interpretation.

## Medical-safety, security, and privacy impact

Medical-safety impact is low and bounded. The cue is safety-positive only as a
situational-awareness aid; it must not change patient classification or imply
clinical validity beyond "this is how long ago the latest reading was accepted."

Security impact is low. The design adds no privilege boundary, file format,
network path, or credential change. Using a monotonic in-memory tick avoids
false precision and avoids creating a persisted clock-derived artifact.

Privacy impact is none expected. No new patient-identifying field, export path,
or retained timestamp history is introduced.

## AI/ML impact assessment

This issue does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing non-AI vital-reading acceptance events
- Output: none
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged
- Dataset and bias considerations: not applicable
- Monitoring expectations: unchanged
- PCCP impact: none

## Validation plan

Validation should prove that the feature remains presentation-only and derives
age from accepted readings correctly across live and paused states.

Build and automated checks:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R UnitTests
```

Targeted unit-test expectations for the new helper:

- device mode returns no freshness cue state
- no-reading session returns waiting state
- live mode computes whole-second age from monotonic ticks
- paused mode reuses the same age basis but changes semantic state
- missing timestamp state fails safe to no-reading/waiting semantics

Manual GUI verification:

- startup with simulation enabled: first accepted reading shows `0 s ago` or
  the first timer-aligned age immediately after session start
- live simulation: age resets on each accepted reading and advances between
  acquisitions without affecting tiles or alerts
- paused simulation: no new reading is added, but the displayed age continues
  to increase on the existing timer cadence
- manual `Add Reading`: age resets when the manual reading is accepted
- `Admit / Refresh`: session freshness resets to waiting-for-first-reading
- `Clear Session`: freshness cue disappears or returns to waiting state
- demo scenarios: final injected reading becomes the current reading and age
  starts from that acceptance moment
- full-buffer rollover/reset in `WM_TIMER`: the new post-reset reading becomes
  the current freshness origin
- simulation disabled/device mode: existing device-mode messaging remains and
  no misleading freshness cue is shown

Regression checks:

- `patient_latest_reading()` behavior is unchanged
- reading history, alert list, aggregate status banner, and NEWS2 tile remain
  behaviorally unchanged
- no new heap allocation or persistence path is introduced

## Rollback or failure handling

If implementation cannot keep the feature presentation-only, stop and split the
work rather than silently broadening it into device-health or stale-data alarm
logic.

If paused-mode age refresh becomes unstable, keep the existing timer and reduce
the feature to a live/paused waiting cue rather than introducing a second clock
or background scheduler in the same issue.

If reviewers reject the requirement delta, remove the freshness cue entirely and
restore the previous header behavior; do not partially ship an untraced
patient-facing display feature.
