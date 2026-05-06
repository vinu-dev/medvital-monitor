# Design Spec: Large-font readability mode

Issue: #71
Branch: `feature/71-large-font-readability-mode`
Spec path: `docs/history/specs/71-large-font-readability-mode.md`

## Problem

The dashboard currently uses one fixed font profile for vital-value tiles:

- `src/gui_main.c` creates the tile value font at 18 pt bold.
- `src/gui_main.c` creates the tile label font at 9 pt and reuses it for tile
  labels, tile badges, and header pills.
- `paint_tile()` renders labels, combined value+unit text, sparkline, and badge
  inside a fixed tile geometry.
- `app_config.c` persists only `sim_enabled` and `language`, so adding a third
  dashboard preference through the current ad hoc save helpers would risk one
  preference overwriting another.

Issue #71 asks for a large-font readability mode to improve glance readability
from short bedside distance without changing any clinical meaning. The current
baseline does not explicitly specify a user-selectable large-font mode, its
persistence behavior, or its supported layout boundary.

This leaves two gaps:

- no approved requirement chain currently authorizes the feature
- no bounded implementation plan currently states how to enlarge the dashboard
  text without clipping values, hiding units, or weakening existing alert cues

## Goal

Add a narrow, dashboard-only readability mode that enlarges the most important
vital tile text while preserving identical values, units, tile order, alert
colors, alert words, NEWS2 output, and data flow.

The intended implementation outcome is:

- one user-selectable toggle
- immediate effect on the active dashboard
- persisted preference across restart
- no production changes outside the GUI/config/localization surface

## Non-goals

- No change to vital-sign thresholds, NEWS2 scoring, alarm generation, patient
  record handling, authentication, localization behavior, CI, release flow, or
  hardware acquisition.
- No claim of accessibility compliance, low-vision certification, or improved
  clinical outcomes.
- No redesign of the dashboard information architecture, tile order, color
  palette, trend sparkline feature, or role-based workflow.
- No change to login, settings-account management, password dialogs, or
  non-dashboard windows beyond the single preference control required to toggle
  the mode.
- No broad responsive-layout rewrite for arbitrarily small window sizes.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact:
trained clinical staff and demo operators may enable a readability mode to view
the same live dashboard information more clearly from a short distance on the
existing Windows workstation UI.

User population:

- bedside clinicians
- ward nurses
- intensivists
- demo and validation operators

Operating environment:

- the existing Win32 desktop dashboard
- default dashboard geometry and larger desktop window sizes
- the same simulated or hardware-backed monitoring flow already supported by
  the application

Foreseeable misuse:

- a user assumes the mode changes alarm severity or value meaning because the
  presentation changed
- enlarged text clips a digit, unit, label, sparkline, or badge on a smaller
  window
- a future implementer enlarges shared fonts globally and unintentionally
  changes header pills or other UI elements outside the intended dashboard scope
- a new config key is written in a way that silently drops existing
  `sim_enabled` or `language` preferences

## Current behavior

Current repository behavior is compatible with a narrow presentation-layer
implementation, but not with a copy-paste font increase:

- `src/gui_main.c` centralizes tile drawing in `paint_tile()` and
  `paint_tiles()`, which means the readability change can stay in one rendering
  path.
- `src/gui_main.c` currently uses `font_tile_lbl` for both tile labels and
  header/tile badge text, so simply enlarging that shared font would broaden the
  visible change outside the intended tile content.
- `src/gui_main.c` restores persisted settings on dashboard startup through
  `app_config_load()` and `app_config_load_language()`, then repaints the
  dashboard.
- `src/gui_main.c` already exposes shared settings surfaces to both admin and
  clinical users.
- `src/app_config.c` writes `monitor.cfg` by rewriting the whole file from a
  small set of helpers. Each helper currently preserves only one sibling value,
  which is acceptable for two keys but fragile for a third.
- `tests/unit/test_config.cpp` already verifies persistence semantics for
  `sim_enabled`.
- `src/localization.c` and `include/localization.h` already hold the localized
  string catalog that any new UI label or toggle text must join.

The pre-existing risk note on this branch already constrains the safe boundary
to a dashboard-only presentation feature with explicit clipping/visibility
verification.

## Proposed change

### 1. Add one persistent readability preference

Add a boolean dashboard preference named `readability_mode` persisted in
`monitor.cfg` as `0` or `1`.

Behavior:

- missing or invalid key defaults to `0` (off)
- enabling the mode applies immediately to the active dashboard
- restarting the application restores the last saved preference
- the setting affects the dashboard only

### 2. Surface the toggle in the existing shared settings flow

Add one readability toggle to the existing Settings window, available to both
admin and clinical users.

Design choice:

- place the control in the existing Language tab as a bounded "display
  preference" addition rather than adding another crowded top-level tab or a new
  dashboard-header button

Rationale:

- both user roles already have access to this settings surface
- the dashboard header is already tightly packed with role and action controls
- adding a seventh settings tab increases layout risk without adding real MVP
  value

Expected behavior:

- the toggle text is localized through the existing localization table
- pressing the toggle updates `g_app` state, saves the preference, and
  invalidates the dashboard for repaint
- no logout or restart is required

### 3. Use a dedicated dashboard font/layout profile instead of enlarging shared fonts

Implement the feature through a small readability profile in `src/gui_main.c`
rather than a blanket font-size change.

The implementation should:

- keep the existing standard profile unchanged
- add a readability profile for tile label/value rendering only
- split shared font responsibilities so tile content can enlarge without also
  enlarging header pills or unrelated settings/list controls
- keep `font_ui` and non-dashboard control fonts unchanged

Concrete scope:

- enlarge tile labels and tile numeric/value text for:
  - Heart Rate
  - Blood Pressure
  - Temperature
  - SpO2
  - Resp Rate
  - NEWS2 Score
- preserve the current combined value+unit presentation unless implementation
  proves that a separate unit placement is necessary to avoid clipping at the
  supported size
- preserve tile badges, alert colors, sparkline presence, and tile order

The implementation may adjust tile-internal padding and y-offsets, but should
do so through a profile/metric struct so both standard and readability modes
share the same drawing path.

### 4. Define the supported layout boundary explicitly

The readability mode is supported for the default dashboard window size and
larger.

Baseline geometry:

- default dashboard size from `WIN_CW` / `WIN_CH` in `src/gui_main.c`
- maximized desktop dashboard size

Implementation rule:

- if the current dashboard window is smaller than the supported baseline when
  readability mode is enabled, the implementation should restore the dashboard
  to at least the default size before repainting, or otherwise refuse the mode
  change and keep standard mode active

This keeps the feature bounded and avoids silently allowing a clipped
"large-font" state on unsupported window geometry.

### 5. Refactor config persistence before adding the third key

Do not add `readability_mode` as an isolated one-off read/write function that
manually preserves the other two keys.

Instead, refactor `app_config.c` to use one internal parse/write path for all
supported preferences, for example through an internal config struct containing:

- `sim_enabled`
- `language`
- `readability_mode`

Public behavior should remain compatible for existing callers:

- existing sim helpers continue to work
- existing language helpers continue to work
- new readability helpers are added on the same preservation model

This is necessary because the current helper pattern would otherwise create a
high-probability regression where saving one preference discards another.

### 6. Add explicit requirements and RTM coverage

Do not stretch existing dashboard requirements by implication. Add explicit
requirement text for the new behavior.

Recommended requirement delta:

- Add `SYS-020` to `requirements/SYS.md` for an optional dashboard readability
  mode that enlarges vital-tile text while preserving the same values, units,
  color semantics, and status cues.
- Add `SWR-GUI-013` to `requirements/SWR.md` for the concrete software behavior:
  toggle location, affected tiles, persistence default, immediate repaint, and
  supported-layout/clipping expectations.
- Add matching rows to `requirements/TRACEABILITY.md`.

Recommended trace anchors:

- `UNS-014` graphical dashboard
- `UNS-010` consolidated patient status presentation

Verification model:

- config persistence is covered by automated unit tests
- visible dashboard integrity remains a manual GUI/DVT verification item

### 7. Keep implementation bounded to the presentation layer

Expected runtime code changes should stay inside:

- dashboard rendering
- UI preference persistence
- localization text
- tests and requirement documents

No clinical-core module should change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- HAL modules

## Files expected to change

Expected implementation files:

- `src/gui_main.c`
- `src/app_config.c`
- `include/app_config.h`
- `src/localization.c`
- `include/localization.h`
- `tests/unit/test_config.cpp`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Files to inspect and update only if implementation needs them for supporting
evidence:

- `dvt/DVT_Protocol.md`
- `README.md`

Files to inspect but not modify unless a bounded defect is found:

- `docs/history/risk/71-large-font-readability-mode.md`
- `tests/unit/test_localization.cpp`

Files that should not change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- `src/sim_vitals.c`
- `include/vitals.h`
- `include/alerts.h`
- `include/patient.h`
- `.github/workflows/**`

## Requirements and traceability impact

This issue should add, not reinterpret, requirements coverage.

Recommended requirement content:

- `SYS-020`: the system shall provide an optional dashboard readability mode
  that enlarges vital-value and label presentation while preserving identical
  values, units, alert colors, and status cues.
- `SWR-GUI-013`: the software shall provide one user-selectable readability
  toggle, default it to off when absent/invalid, apply it immediately to the
  dashboard, persist it in `monitor.cfg`, and keep the supported dashboard tiles
  fully visible without clipped digits or hidden units at the supported window
  size.

Expected RTM evidence:

- new SYS/SWR rows and coverage summaries
- unit-test references for config persistence support
- manual GUI verification reference for visual integrity

No `@req` retagging or traceability edits should touch clinical-classification,
NEWS2, alarm-limit, authentication, or persistence requirements unrelated to
this new preference.

## Medical-safety, security, and privacy impact

Medical-safety impact:

- direct clinical logic impact is low because the change does not alter values,
  thresholds, scoring, alerts, or storage
- human-factors risk is non-zero because clipped or obscured text could cause a
  clinician to misread the current state

Required safety controls:

- preserve identical displayed values and units in both modes
- preserve alert colors, words, and NEWS2 output
- verify that enlarged text does not hide the sparkline, badge, patient bar, or
  alert list cues needed for the pilot workflow
- keep the feature optional and user-invoked

Security impact:

- low
- the new persisted value is a local UI preference only
- no credential, RBAC, or network behavior changes

Privacy impact:

- none expected
- no new PHI fields or new export/storage surface

## AI/ML impact assessment

This change does not add, remove, change, or depend on an AI-enabled device
software function.

Assessment:

- model purpose: not applicable
- input data: not applicable beyond existing non-AI vital-sign display data
- output: not applicable
- human-in-the-loop limits: not applicable
- transparency needs: no new AI transparency requirement
- dataset and bias considerations: not applicable
- monitoring expectations: standard GUI regression monitoring only
- PCCP impact: none

## Validation plan

Repository validation should combine bounded automated checks with explicit
manual GUI review.

Suggested commands:

```powershell
build.bat
run_tests.bat
ctest --test-dir build --output-on-failure -R "Config|Localization"
```

Targeted config/requirements consistency checks:

```powershell
rg -n "readability_mode|SWR-GUI-013|SYS-020|Large Font|Readability" src include tests requirements
git diff --stat
git diff -- src/gui_main.c src/app_config.c include/app_config.h src/localization.c include/localization.h tests/unit/test_config.cpp requirements/SYS.md requirements/SWR.md requirements/TRACEABILITY.md
```

Required manual GUI verification:

- start in standard mode and capture the default dashboard layout
- enable readability mode and verify the six dashboard tiles still show complete
  values, units, labels, badges, and sparklines
- verify identical patient data yields identical alert levels, tile colors, and
  NEWS2 score before and after toggling the mode
- verify the patient bar, alerts list, history list, and role/settings actions
  remain visible and usable
- verify restart persistence
- verify disabling the mode restores the standard layout cleanly
- verify supported default-size and maximized-size behavior

If implementation cannot satisfy the no-clipping requirement at the supported
size without changing dashboard information architecture, stop and return the
issue to design rather than broadening scope silently.

## Rollback or failure handling

If the implementation introduces clipping, hidden units, or weakened alert
visibility, disable the new preference path and revert to standard mode rather
than shipping a partially working readability state.

Operational recovery should stay simple:

- missing or invalid `readability_mode` falls back to standard mode
- setting `readability_mode=0` in `monitor.cfg` restores the standard layout on
  next launch
- a rejected implementation can be reverted as a bounded GUI/config change

If implementation discovers that the feature really requires broader dashboard
recomposition, new accessibility claims, or clinical-behavior changes, stop,
comment on the issue with the scope split, and move the item back to
`ready-for-design` instead of guessing.
