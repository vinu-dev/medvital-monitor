# Design Spec: Add a compact dashboard density preset

Issue: #49
Branch: `feature/49-compact-dashboard-density-preset`
Spec path: `docs/history/specs/49-compact-dashboard-density-preset.md`

## Problem

The current Win32 dashboard has one fixed presentation density. `src/gui_main.c`
hard-codes the dashboard geometry through constants such as `WIN_CW`, `WIN_CH`,
`TILE_H`, and the tile padding math inside `paint_tiles()`. Operators cannot
choose a tighter presentation when working on smaller displays or when they
want less white space around the same six monitored surfaces.

The repository already has the pieces needed for a narrow implementation:

- `settings_proc()` provides an existing preferences surface.
- `src/app_config.c` persists workstation-local UI state in `monitor.cfg`.
- `paint_tile()` and `paint_tiles()` already centralize most tile rendering.

What is missing is a traceable design that adds density selection without
changing any monitored data, alert thresholds, NEWS2 behavior, or alert
salience.

## Goal

Add exactly two operator-selectable dashboard density presets, `standard` and
`compact`, so the dashboard can present the same monitored context in a tighter
layout while preserving identical clinical content, alert behavior, and tile
ordering.

## Non-goals

- No change to alarm thresholds, NEWS2 scoring, alert timing, or patient-record
  behavior.
- No addition, removal, or reordering of dashboard tiles, alert surfaces, input
  fields, history, or trend data.
- No free-form layout editor, drag/drop customization, or hidden-parameter
  behavior.
- No per-user preference storage in `users.dat`.
- No vendor-specific UI mimicry, waveform-density cloning, or new clinical
  claims.
- No production-code changes outside dashboard presentation, localization, and
  config persistence support for this feature.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use does not change. The feature remains a display preference for the
existing Win32 monitoring dashboard used by trained clinical staff or supervised
demo operators.

User population:

- bedside clinicians
- ward or ICU staff
- training and demonstration operators

Operating environment:

- shared Windows workstations
- desktop displays where the current dashboard can feel overly spacious or
  crowded depending on monitor size and scaling

Foreseeable misuse:

- enabling compact mode on a low-resolution or high-scaling display where value
  readability degrades
- assuming compact mode changes monitoring logic rather than layout only
- leaving a shared workstation in compact mode and surprising the next operator

Design decision for this issue:

- persist density as a workstation-local preference in `monitor.cfg`, because
  the current application has workstation-scoped configuration rather than
  user-scoped display preferences
- do not expand scope into per-user preference storage; if the product owner
  later wants user-specific density, that should be a separate design issue

## Current behavior

The current dashboard has one layout profile:

- `src/gui_main.c` defines fixed geometry constants at lines around the
  dashboard layout block and uses a fixed `pad/tw/th` calculation inside
  `paint_tiles()`.
- `paint_tile()` assumes one label row, one value row, one sparkline strip, and
  one badge row with fixed offsets.
- `paint_tiles()` always renders the same six slots: Heart Rate, Blood
  Pressure, Temperature, SpO2, Resp Rate, and NEWS2.
- `paint_status_banner()` and the patient/action controls assume the current
  spacing model.

The current persistence model is also fixed:

- `src/app_config.c` stores `sim_enabled` and `language` in `monitor.cfg`.
- `app_config_save()` rewrites the file while preserving language.
- `app_config_save_language()` rewrites the file while preserving sim mode.
- there is no density key, loader, saver, or invalid-value fallback for a
  display preset

The current tests cover only the existing config keys:

- `tests/unit/test_config.cpp` verifies `sim_enabled` persistence and malformed
  file fallback behavior
- `tests/unit/test_localization.cpp` verifies language persistence and that
  saving language preserves `sim_enabled`

## Proposed change

### 1. Add explicit density state and layout profiles

Introduce a two-value dashboard density state:

- `standard`: behavior-equivalent to the current geometry
- `compact`: a tighter profile that reduces presentation whitespace only

Implementation should stop relying on scattered hard-coded geometry and instead
derive dashboard metrics from a shared profile or helper structure. The active
profile should govern at minimum:

- tile container height
- inter-tile padding and gaps
- label/value/badge vertical offsets inside a tile
- status-banner spacing
- any dashboard-control offsets that must move to stay aligned with the denser
  tile block

Compact mode should reduce whitespace before it reduces text size. Numeric value
legibility and alert salience are higher priority than making the layout
maximally tight.

### 2. Preserve identical monitored content and alert surfaces

Both presets must render the same dashboard information:

- the same six tile slots in the same order
- the same measured values and units
- the same alert colors, badges, and aggregate banner behavior
- the same sparkline content and active-alert list behavior
- the same patient-entry and action controls

Compact mode may change spacing, padding, and controlled label wrapping, but it
must not:

- clip or ellipsize current numeric values
- clip or ellipsize severity badges
- hide any tile, sparkline, or alert surface
- reduce warning/critical color prominence
- alter the semantics of `N/A` or `--` display states

If wrapping is needed, only descriptive labels may wrap, and only in a bounded,
deterministic way. Numeric values, units, and alert badges should remain
single-line.

### 3. Expose density through the existing preferences surface

Add a dashboard-display preference control to the existing settings window flow
implemented by `settings_proc()`. The preferred UI is a dedicated `Display` tab
with:

- a selector for `Standard` and `Compact`
- short explanatory copy that the feature changes layout density only
- an apply action that updates the in-memory density state immediately

The dashboard must repaint without requiring logout or application restart.

Role boundary:

- user-management controls remain admin-only
- the density control belongs to the non-clinical-logic preferences surface and
  should be reachable wherever the existing role-filtered settings/preferences
  window is already used

If implementation discovers that current RBAC wording in `SWR-GUI-008` /
`SWR-GUI-009` conflicts with this preferences placement, update the
requirements wording narrowly so display preferences remain accessible without
granting clinical users access to account-management functions.

### 4. Localize the new strings

Because tab labels and UI text already route through the localization layer, the
new density controls should add localized strings for:

- the display tab label
- dashboard density field label
- `Standard`
- `Compact`
- short help text explaining that only spacing changes

This keeps the feature consistent with the existing `SWR-GUI-012` localization
behavior instead of introducing hard-coded English text into one new tab.

### 5. Persist density in monitor.cfg without cross-setting data loss

Persist the selected density in `monitor.cfg` as an explicit key:

```text
sim_enabled=1
language=0
dashboard_density=standard
```

Behavior requirements:

- missing density key defaults to `standard`
- invalid density value defaults to `standard`
- saving density preserves the current `sim_enabled` and `language` values
- saving sim mode or language preserves the current density value

Because the current config module rewrites the whole file from separate feature
save paths, this issue should add a small shared config-state helper inside
`src/app_config.c` rather than copying the current two-key pattern a third time.

Recommended implementation shape:

- add a private `AppConfigState`-style struct or equivalent shared parse/write
  helper
- keep the public API narrow, for example with
  `app_config_load_density()` / `app_config_save_density()`
- migrate the existing sim/language save paths to the shared helper so all
  three keys round-trip together

This is still a narrow change and avoids silent density loss the next time some
other setting is saved.

### 6. Keep the feature presentation-only at startup and runtime

Density must load before the first full dashboard paint so the initial view
matches persisted state. On invalid or missing config, startup should fall back
cleanly to `standard` and continue with the existing sim/language defaults.

No part of this feature should change:

- vital-sign acquisition
- timer cadence
- alert generation
- NEWS2 calculation
- patient storage
- alarm-limit logic

## Files expected to change

Implementation files expected to change:

- `src/gui_main.c`
- `src/app_config.c`
- `include/app_config.h`
- `src/localization.c`
- `include/localization.h`
- `tests/unit/test_config.cpp`
- `tests/unit/test_localization.cpp`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Files that should not change:

- `src/vitals.c`
- `src/news2.c`
- `src/alerts.c`
- `src/patient.c`
- `include/vitals.h`
- `include/patient.h`
- hardware/HAL interfaces unrelated to config persistence

## Requirements and traceability impact

No approved requirement currently describes dashboard density as its own
behavior. The implementation should therefore update requirements narrowly
rather than overloading an unrelated existing requirement.

Recommended requirement strategy:

- extend `SYS-014` so the graphical dashboard explicitly supports an operator
  density preset while preserving identical monitored content and alert
  semantics
- add a new GUI software requirement, `SWR-GUI-013` or the next available GUI
  identifier, to cover:
  - exactly two approved presets
  - explicit selector labels
  - immediate repaint on apply
  - `monitor.cfg` persistence and invalid-value fallback to `standard`
  - preservation of value, badge, and alert-surface readability

Traceability impact should point to:

- `src/gui_main.c` for density-state handling and repaint behavior
- `src/app_config.c` / `include/app_config.h` for persistence
- `src/localization.c` / `include/localization.h` for selector strings
- `tests/unit/test_config.cpp` and `tests/unit/test_localization.cpp` for
  supporting automated evidence
- manual GUI review for the visual legibility aspects not suited to unit tests

Requirements that must remain unchanged in behavior:

- `SWR-GUI-003`
- `SWR-GUI-010`
- `SWR-GUI-012`
- `SWR-VIT-008`
- `SWR-NEW-001`
- `SWR-ALM-001`

Those requirements may gain neighboring traceability updates, but this issue
must not change their clinical meaning.

## Medical-safety, security, and privacy impact

Medical-safety impact is low for direct clinical logic and moderate for UI
legibility risk.

Primary safety constraints:

- preserve identical data equivalence between `standard` and `compact`
- preserve warning/critical salience
- prohibit clipping or ambiguity in numeric values, units, badges, and banner
  content
- make the persisted density state explicit and deterministic for shared
  workstations

Security impact is low. The feature stores only a workstation-local display
preference and must not introduce credentials, user identifiers, patient data,
or audit-significant workflow state into `monitor.cfg`.

Privacy impact is low for the same reason: no new patient or operator data
should be stored.

## AI/ML impact assessment

This feature does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing non-AI dashboard state
- Output: none beyond non-AI layout selection
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged beyond clear labeling of the selected density
- Dataset and bias considerations: none
- Monitoring expectations: none beyond normal GUI verification
- PCCP impact: none

## Validation plan

Automated validation:

- run `build.bat`
- run `run_tests.bat`
- extend `tests/unit/test_config.cpp` with density coverage for:
  - missing key defaults to `standard`
  - invalid key defaults to `standard`
  - save/load of `compact`
  - saving density preserves sim mode and language
  - saving sim mode preserves density
  - saving language preserves density
- add a light localization smoke test so the new selector strings are available
  across the four approved languages

Manual GUI validation:

- verify switching between `standard` and `compact` repaints immediately with
  no restart
- verify both presets show the same six tiles, the same values, the same NEWS2
  output, and the same alert list/banner for identical readings
- verify no numeric value, unit, badge, or alert text is clipped at the default
  dashboard size and at common Windows scaling levels such as 100% and 125%
- verify `monitor.cfg` restores the chosen preset on restart
- verify malformed `dashboard_density` values fall back to `standard`
- verify role boundaries remain intact and no density access path exposes
  account-management controls to clinical users

## Rollback or failure handling

If density cannot be loaded, parsed, or validated, the application should fall
back to `standard` and continue normal startup.

If implementation discovers either of these conditions, stop and return the
issue to design review instead of widening scope silently:

- achieving compact mode requires hiding data, reducing alert salience, or
  changing clinical semantics
- exposing density selection cleanly requires a broader RBAC or per-user
  preferences redesign than this issue allows

Rollback is straightforward:

- revert the implementation commit set
- remove the `dashboard_density` key from `monitor.cfg` handling
- return all sessions to the current `standard` layout behavior
