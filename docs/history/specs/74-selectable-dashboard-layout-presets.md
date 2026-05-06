# Design Spec: Issue 74

## Problem

Issue `#74` asks for operator-selectable dashboard layout presets so the same
monitoring screen can support both a broader bedside overview workflow and a
denser review or handover workflow.

The current Win32 dashboard is effectively single-layout:

- `src/gui_main.c` paints the vital-sign dashboard as a fixed 3-column by
  2-row tile grid in `paint_tiles()`.
- The dashboard window is resizable, but `reposition_dash_controls()` only
  reanchors header buttons and stretches the two listboxes horizontally.
- Most patient-entry, reading-entry, and scenario controls are created at fixed
  coordinates in `create_dash_controls()`.
- `app_config.c` persists only global GUI preferences for simulation mode and
  language in `monitor.cfg`; there is no layout-preset state today.

This means the product cannot offer a compact review presentation without
editing the only existing layout path, and any implementation that simply
"moves things around" risks clipping or de-emphasising required monitoring
surfaces.

## Goal

Add a narrow, traceable design for exactly two dashboard layout presets:
`Overview` and `Review`.

The design must:

- preserve the current layout as the safe baseline
- keep the feature presentation-only
- keep the active preset continuously visible
- keep all mandatory monitoring information visible in both presets
- define safe persistence behavior for a workstation-local default

## Non-goals

- Drag-and-drop layout editing.
- User-authored custom presets.
- Per-patient layout state.
- Shared, synced, or cloud-backed profiles.
- Changes to vital-sign acquisition, thresholds, alarm logic, NEWS2 scoring,
  authentication, or patient-record behavior.
- Adding or removing monitored parameters.
- Hiding the alert list, history list, patient identity, status banner, RESP
  RATE tile, or NEWS2 tile in any preset.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use does not change. The monitor remains a Windows workstation
application used by trained clinical staff to view patient identity, vital
signs, alert status, and reading history.

Relevant user groups remain:

- bedside clinicians monitoring the current patient state
- ward nurses reviewing deterioration or recovery
- staff performing review or handover using recent readings and active alerts

The relevant operating environment remains the current resizable Win32 desktop
window, including:

- the default dashboard size (`920x850`)
- the current enforced minimum tracked size (`760x640`)
- simulation mode, paused simulation mode, and device mode
- localized UI strings loaded through the existing localization layer

Foreseeable misuse and operator risks remain the ones called out in the risk
note:

- the operator may assume non-visible information is absent or normal
- the operator may not notice which preset is active
- a remembered compact preset may surprise the next login session on the same
  workstation
- resize or localization changes may create overlap, clipping, or inaccessible
  controls

The design therefore treats visibility invariants and active-preset visibility
as first-class safety controls rather than cosmetic details.

## Current Behavior

- `paint_header()` renders the application title, logged-in user block, role
  badge, and simulation status text.
- `paint_patient_bar()` renders patient identity and BMI context immediately
  below the header.
- `paint_tiles()` renders six dashboard surfaces in a fixed 3x2 arrangement:
  HEART RATE, BLOOD PRESSURE, TEMPERATURE, SpO2, RESP RATE, and NEWS2 SCORE.
- `paint_status_banner()` renders the aggregate status/device-mode banner below
  the tile block.
- `create_dash_controls()` creates patient-entry fields, manual-reading fields,
  demo buttons, an active-alerts listbox, and a reading-history listbox at
  mostly fixed pixel coordinates.
- `reposition_dash_controls()` currently adjusts only:
  - header buttons (`Logout`, `Pause Sim`, `Settings`)
  - the widths of the active-alerts and reading-history listboxes
- `WM_GETMINMAXINFO` enforces a minimum dashboard size of `760x640`.
- `app_config_load()` / `app_config_save()` persist `sim_enabled`.
- `app_config_load_language()` / `app_config_save_language()` persist
  `language`.
- `refresh_dash_language()` destroys and recreates dashboard child controls when
  the language changes, which is relevant because any new layout controls must
  survive the same rebuild flow.

## Proposed Change

### 1. Preset model and state

Add one explicit dashboard-layout state with exactly two values:

- `Overview`
- `Review`

Implementation should keep this state presentation-only. It must not affect:

- patient data
- alert computation
- NEWS2 computation
- simulation/device mode behavior
- account or role logic

The state should live alongside the other dashboard UI state in `AppState` and
be loaded before the dashboard is first painted.

### 2. Always-visible preset affordance

Add one dashboard-level preset control in the header area plus one
always-visible active-preset indicator.

Recommended implementation:

- a header toggle button that switches between the two presets
- a persistent header label or pill that displays the active preset as
  `Overview` or `Review`

The active preset indicator shall remain visible regardless of:

- current alert severity
- simulation mode
- resize events
- language refreshes

The control may move during resize, but the active preset may not become hidden
behind the user-info block or header buttons.

### 3. Centralized geometry instead of ad hoc coordinates

Do not implement the second preset by scattering one-off coordinate changes
through `paint_tiles()`, `create_dash_controls()`, and `reposition_dash_controls()`.

Implementation should introduce one centralized dashboard-layout helper that
computes preset-specific rectangles or metrics for:

- the tile region
- the status banner
- the patient-entry control block
- the manual-reading control block
- the scenario-button row
- the active-alerts list
- the reading-history list
- the preset indicator/toggle placement

Both painting and child-control reflow should consume the same computed layout
metrics so that resize behavior is deterministic and testable.

### 4. Preserve Overview as the baseline

`Overview` should be functionally equivalent to the current layout except for
the added preset control and active-preset indicator.

That means:

- the current six dashboard surfaces remain present
- the existing visual hierarchy remains familiar
- the change does not force clinicians off the established overview-oriented
  presentation

If implementation cannot preserve the current overview geometry closely, the
change is too broad and should be split before code lands.

### 5. Define Review as a denser review-oriented presentation

`Review` should not be a free-form redesign. It should be a bounded, denser
presentation of the same monitoring information.

The Review preset may:

- reduce decorative whitespace inside or between tiles
- compact the patient-entry and manual-reading sections
- allocate more visible space to active alerts and reading history
- reorder lower-screen blocks to favour side-by-side review of alerts and
  reading history

The Review preset shall not:

- remove any of the six tiles
- hide the patient bar
- hide the aggregate status banner
- hide the active-alerts list
- hide the reading-history list
- hide the Admit / Refresh, Add Reading, or Clear Session actions

Recommended behavior:

- keep the six vital/NEWS2 surfaces simultaneously visible
- keep the most review-relevant lower panels more prominent than in Overview
- make the layout difference obvious enough that the operator perceives a real
  mode change, but not so different that the screen becomes unfamiliar

### 6. Visibility invariants and fail-safe layout behavior

Both presets must satisfy these invariants at the supported window sizes:

- patient identity remains visible
- HEART RATE, BLOOD PRESSURE, TEMPERATURE, SpO2, RESP RATE, and NEWS2 remain
  visible at the same time
- the aggregate status banner remains visible
- the active-alerts list remains visible
- the reading-history list remains visible
- the active preset indicator remains visible

If the Review geometry cannot satisfy those invariants at the current minimum
window size, the implementation should prefer one of these fail-safe options:

- increase the minimum dashboard size
- clamp the layout to a larger safe minimum
- fall back to the Overview geometry

Silent clipping is not acceptable.

### 7. Persistence semantics

Persist the selected preset as workstation-local, non-PHI UI state in the
existing `monitor.cfg` file.

Recommended format:

```text
sim_enabled=1
language=0
layout_preset=overview
```

Required behavior:

- missing `layout_preset` defaults to `overview`
- invalid `layout_preset` defaults to `overview`
- persistence remains global to the workstation, not per-patient and not
  cloud-synced
- the preset is restored on the next dashboard launch

Recommended product decision for implementation:

- keep persistence global rather than per-user because the repository already
  stores GUI preferences globally in `monitor.cfg` and has no account-scoped
  preference store
- mitigate cross-login surprise by keeping the active preset continuously
  visible instead of introducing a second settings path or hidden remembered
  state

If saving the preset fails, the current session may continue using the in-memory
preset, but the next launch must safely fall back to `Overview` or the last
validly saved value. The failure must not affect monitoring behavior.

### 8. Localization scope

New user-facing control text should be routed through the existing localization
layer.

Expected new localized strings:

- layout/preset label text
- toggle button text
- `Overview`
- `Review`

If the team decides to treat preset names as fixed product terms rather than
translated strings, that choice must be documented explicitly in the final
requirements and verification notes so localization behavior is not ambiguous.

## Files Expected To Change

Implementation is expected to touch these production/UI files:

- `src/gui_main.c`
- `src/app_config.c`
- `include/app_config.h`
- `src/localization.c`
- `include/localization.h`

Implementation is expected to touch these requirements and architecture files:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `docs/ARCHITECTURE.md`

Implementation is expected to touch these verification artifacts:

- `tests/unit/test_config.cpp`
- `dvt/DVT_Protocol.md` if a formal DVT case is added or updated
- any GUI verification checklist or automation that captures the preset
  switching, persistence, and resize evidence

No domain-logic source files should change for this issue:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- `src/sim_vitals.c`
- authentication/account-management files

## Requirements And Traceability Impact

This feature should not be implemented under the existing GUI requirements
alone. It needs explicit requirement coverage for preset selection, visibility
invariants, and persisted non-clinical UI state.

Recommended new requirements:

- `UNS-017` - dashboard layout personalization for different clinical review
  workflows without loss of mandatory monitoring information
- `SYS-020` - selectable dashboard layout presets that preserve simultaneous
  visibility of mandatory monitoring surfaces
- `SWR-GUI-013` - dashboard preset selection, active-preset visibility, and
  invariant visibility of all six tiles plus alerts/history
- `SWR-GUI-014` - persisted workstation-local layout preference with safe
  default/fallback behavior

Existing requirements that constrain this change and should be referenced in the
new traces:

- `UNS-010`
- `UNS-014`
- `SYS-011`
- `SYS-014`
- `SWR-GUI-003`
- `SWR-GUI-004`
- `SWR-GUI-010`
- `SWR-GUI-012`
- `SWR-VIT-008`
- `SWR-NEW-001`

The traceability update should make it explicit that:

- layout presets are a UI organization aid, not a new clinical mode
- persistence is non-clinical UI state only
- verification includes resize and visibility evidence, not just config-file
  parsing

## Medical-Safety, Security, And Privacy Impact

Medical safety remains indirect but meaningful.

This change does not alter:

- vital-sign classification
- alert-generation logic
- NEWS2 scoring
- alarm limits
- acquisition/HAL behavior

The safety risk is display-related: delayed recognition caused by hidden,
clipped, or visually de-emphasised information. The design controls for this
issue are therefore:

- simultaneous visibility of all required monitoring surfaces
- always-visible active-preset state
- safe default to `Overview`
- no silent fallback to a clipped layout

Security impact should remain low because the change introduces no new network
surface, credential path, or privileged action.

Privacy impact should remain low because the persisted state is limited to a
non-clinical workstation preference. The implementation must not write patient
identity or patient-specific layout choices into `monitor.cfg`.

## AI/ML Impact Assessment

This change does not add, remove, modify, or depend on an AI-enabled device
software function.

- No model is introduced or changed.
- No model input or output changes.
- No human-in-the-loop AI workflow changes.
- No dataset, bias, monitoring, or transparency concerns are introduced.
- No Predetermined Change Control Plan (PCCP) impact.

AI/ML impact: none.

## Validation Plan

Implementation and review should cover both automated persistence checks and
manual GUI evidence.

Automated/configuration checks:

- extend unit coverage for `monitor.cfg` parsing and persistence to include
  `layout_preset`
- verify missing and invalid preset values fall back to `Overview`
- verify saving a preset preserves existing `sim_enabled` and `language`
  values, just as those settings preserve one another today

Build and regression commands:

- `cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure -R GUI|DVT`

Manual GUI verification:

- verify `Overview` matches the existing baseline layout apart from the new
  preset affordance
- verify `Review` is visibly denser and still shows all mandatory surfaces
- verify the active preset indicator always matches the actual layout
- verify preset switching under:
  - no patient admitted
  - normal-status patient
  - warning-status patient
  - critical-status patient
- verify behavior in both simulation mode and device mode
- verify logout/login and full application restart restore the intended preset
  behavior
- verify resize behavior at:
  - default size (`920x850`)
  - minimum allowed size
  - a wider-than-default window
- verify no control overlap or clipping with longer localized strings after a
  language change

Review checks:

- inspect the final diff to confirm no domain logic or clinical thresholds were
  changed
- inspect the final diff to confirm no new persistence path stores PHI

## Rollback Or Failure Handling

- If the team cannot produce a Review layout that preserves all mandatory
  surfaces at supported window sizes, do not ship the Review preset. Keep
  `Overview` only and split the redesign work into a follow-up issue.
- If `layout_preset` loading fails or the persisted value is invalid, load
  `Overview`.
- If `layout_preset` saving fails, keep the current session running and avoid
  any change to patient or monitoring state.
- If localization or resize support for the new preset affordance becomes
  materially more complex than the bounded issue allows, prefer a simpler header
  indicator/toggle design rather than expanding scope into a general dashboard
  redesign.
- Do not compensate for layout difficulty by hiding required information or by
  weakening the visibility invariants established in this spec.
