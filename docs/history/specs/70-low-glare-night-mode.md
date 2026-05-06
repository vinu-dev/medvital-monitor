# Design Spec: Add a low-glare night mode

Issue: `#70`  
Branch: `feature/70-low-glare-night-mode`  
Spec path: `docs/history/specs/70-low-glare-night-mode.md`

## Problem

Issue `#70` asks for a low-glare night mode that makes bedside dashboard review
less visually harsh in low-light rooms without changing monitored values,
thresholds, NEWS2 scoring, alert generation, or patient data handling.

The current Win32 dashboard has a single bright palette hard-coded into
`src/gui_main.c`. `paint_header()`, `paint_patient_bar()`, `paint_tile()`,
`paint_tiles()`, `paint_status_banner()`, and the dashboard background all use
one presentation mode only. The repository already persists simple UI
preferences in `src/app_config.c`, but that file currently stores only the
global `sim_enabled` and `language` settings.

The main design risk is not algorithmic; it is human factors and workstation
handoff. Alert semantics currently depend on clear color differentiation for
`NORMAL`, `WARNING`, and `CRITICAL`. A low-glare palette that reduces contrast
too far could make abnormal states harder to recognize. A second risk is
preference scope: if night mode is persisted as one global workstation setting,
one clinician can silently hand off an unexpected display mode to the next
clinician on a shared machine.

## Goal

Add one manual, presentation-only night mode toggle for the authenticated user
that:

- reduces neutral-surface brightness on the dashboard for low-light review
- preserves the same clinical values, alert semantics, and update behavior
- remains visibly identifiable when active
- persists locally in a user-scoped way that avoids cross-user carryover on a
  shared workstation

## Non-goals

- No change to vital classification, alert thresholds, alarm generation, NEWS2,
  alarm limits, patient storage, authentication decisions, or account roles.
- No automatic time-of-day switching, ambient-light sensing, or audio-volume
  control.
- No competitor-specific naming, iconography, scheduling workflow, or layout.
- No change to `users.dat` format or to `gui_users.c` account persistence.
- No requirement to apply night mode to the pre-authentication login window.
- No broad theme-engine rewrite or unrelated visual redesign.

## Intended use impact, user population, operating environment, and foreseeable misuse

- Intended use impact: improve comfort and readability for trained clinicians
  reviewing the monitor in dim rooms while keeping clinical meaning unchanged.
- User population: bedside clinicians, ward staff, and other authenticated
  operators using the Windows dashboard.
- Operating environment: shared Windows workstations in low-light or overnight
  care areas where a bright dashboard can be distracting.
- Foreseeable misuse: a user may assume night mode lowers clinical urgency when
  it only changes presentation.
- Foreseeable misuse: a dimmed palette may reduce recognition of warning or
  critical states if contrast is not preserved.
- Foreseeable misuse: a global persisted preference may surprise the next user
  on a shared workstation.
- Foreseeable misuse: a user may interpret night mode as a privacy feature or
  alarm-silence feature when it is neither.

## Current behavior

- `src/gui_main.c` defines one fixed palette through `CLR_NAVY`, `CLR_SLATE`,
  `CLR_NEAR_WHITE`, `CLR_OK_*`, `CLR_WN_*`, and `CLR_CR_*`.
- `AppState` tracks simulation and localization state, but it has no
  `night_mode_enabled` flag.
- `attempt_login()` stores `g_app.logged_username`, which provides enough user
  context to load a user-scoped display preference after authentication.
- `src/app_config.c` persists only `sim_enabled` and `language`, and its save
  helpers rewrite the full `monitor.cfg` file each time.
- The Settings surface already contains simulation and language controls, but
  there is no night-mode control or indicator.
- `include/localization.h` and `src/localization.c` contain no strings for
  night-mode labels, status text, or explanatory copy.
- `tests/unit/test_config.cpp` verifies config persistence behavior, and
  `tests/unit/test_localization.cpp` verifies language persistence and string
  lookup. No automated test currently covers a night-mode preference.

## Proposed change

### 1. Add authenticated user-scoped night-mode state

Add a dedicated `night_mode_enabled` flag to `AppState` and treat it as a
presentation preference only.

Behavior:

- Default to standard mode when no saved preference exists or when a saved
  preference is invalid.
- Load the preference only after authentication, using the already captured
  `g_app.logged_username`.
- Keep the login window in standard mode because no authenticated user context
  exists before sign-in.
- Do not couple night mode to `sim_enabled`, `sim_paused`, patient presence, or
  alert severity.

### 2. Use one manual dashboard toggle as the primary control

The issue asks for a single UI toggle. The least ambiguous place for that
toggle is the dashboard header, not the Simulation settings tab.

Recommended behavior:

- Add one header control that switches between standard mode and night mode.
- The toggle must apply immediately without requiring a restart.
- The toggle label must make the current action clear, for example
  `Enable Night Mode` when standard mode is active and `Disable Night Mode`
  when night mode is active.
- Night mode must also have a non-color-only active indicator in the header or
  nearby status area so the mode is explicit even if the operator joins mid-use.

This keeps the feature discoverable during bedside review and avoids implying
that it is part of simulation transport control.

### 3. Persist the preference locally, but scope it to the authenticated user

To satisfy the issue's local-persistence intent without creating cross-user
handoff risk, persist night mode in `monitor.cfg` as a user-scoped preference,
not as one workstation-global `night_mode=0|1` flag.

Implementation requirements:

- Extend `app_config` with dedicated load/save helpers for night mode that
  accept the authenticated username.
- Preserve existing `sim_enabled` and `language` values on every write.
- Use a plain-text encoding that round-trips usernames safely and does not
  silently collapse multiple users into one shared preference.
- Treat missing, malformed, or out-of-range persisted values as standard mode.

The exact on-disk encoding can be chosen during implementation, but the
behavioral contract is fixed: night mode persists per authenticated user and
must not silently follow the last operator on the workstation.

### 4. Centralize palette selection instead of scattering conditionals

Do not implement night mode as one-off `if (night_mode_enabled)` branches at
every draw site. Introduce a small theme or palette selection helper so all
paint paths read from one active presentation palette.

The active palette must cover:

- dashboard background
- header background and header text
- patient bar background and text
- vital tiles, NEWS2 tile, borders, badge text, and value text
- status banner backgrounds and text
- sparkline stroke colors where needed for readability
- post-login settings and dialog surfaces to the extent needed to avoid a
  bright full-screen contrast jump during the same authenticated session

The login screen may remain standard mode because the user preference is not
known until after authentication.

### 5. Preserve alert semantics and visibility

Night mode is presentation-only, but it still changes how alert states are
perceived. The palette must preserve the existing semantic ordering and
one-glance distinguishability of `NORMAL`, `WARNING`, and `CRITICAL`.

Required visual rules:

- Do not change how alert levels are computed or labeled.
- Keep `WARNING` and `CRITICAL` visually distinct from `NORMAL` even on a dark
  background.
- Keep numeric values, status labels, and banner text at least as legible as in
  the current mode.
- Keep device-mode and simulation-mode text readable in both standard and night
  mode.
- Do not use night mode to soften or mute severe-state wording.

If a low-glare color choice makes warning or critical states harder to read,
that palette choice is a defect, not an acceptable tradeoff.

### 6. Keep post-login windows aligned enough to avoid jarring mode breaks

Night mode is mainly for the bedside dashboard, but authenticated operators may
open Settings, password, or account dialogs while the mode is active.

Implementation target:

- The dashboard window is mandatory scope.
- The Settings surface and other post-login dialogs should inherit the same
  neutral background direction where practical.
- Native Win32 edit and button chrome may remain platform-default if a full
  owner-draw restyle would expand scope too far, but no control may become
  unreadable and no dialog should revert to a glaring white full-window surface
  while night mode is active.

### 7. Localize all new visible night-mode text

Any new button label, indicator text, or explanatory copy must route through
the existing localization layer. Do not hard-code English-only night-mode text
in `gui_main.c`.

At minimum, implementation should add localized strings for:

- night mode on/off action labels
- a compact active indicator such as `NIGHT MODE`
- any brief explanatory copy attached to the control

### 8. Add new requirements instead of overloading existing GUI wording

The current approved requirements describe a color-coded dashboard and existing
GUI controls, but they do not explicitly capture low-light ergonomics or
user-scoped night-mode persistence. The cleanest traceability path is to add a
new requirement chain instead of stretching `UNS-014` and `SWR-GUI-003` beyond
their present wording.

Recommended additions:

- `UNS-017` - Low-light dashboard review ergonomics
- `SYS-020` - Manual low-glare night presentation mode
- `SWR-GUI-013` - Manual night-mode toggle and user-scoped persistence
- `SWR-GUI-014` - Night-mode palette and alert-visibility preservation

If parallel branch activity changes the next available IDs, preserve the same
behavioral split and assign the next free identifiers at implementation time.

## Files expected to change

Expected implementation files:

- `src/gui_main.c`
- `src/app_config.c`
- `include/app_config.h`
- `src/localization.c`
- `include/localization.h`

Expected requirements and traceability files:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected verification-support files:

- `tests/unit/test_config.cpp`
- optionally `tests/unit/test_localization.cpp` if the team adds string-presence
  assertions for the new localized labels

Files that should not change for this issue:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/patient.c`
- `src/gui_users.c`
- `src/gui_auth.c`
- `include/gui_users.h`
- `.github/workflows/**`

## Requirements and traceability impact

This feature adds a new human-factors presentation capability. It should not be
recorded as a change to clinical algorithms or alert-generation logic.

Recommended traceability shape:

- Add `UNS-017` for low-light dashboard review ergonomics on shared clinical
  workstations.
- Add `SYS-020` to define one manual low-glare mode that preserves clinical
  meaning and remains user-scoped.
- Add `SWR-GUI-013` for:
  - the manual toggle
  - immediate mode switching
  - per-user persistence
  - default-off fallback on missing or invalid saved state
- Add `SWR-GUI-014` for:
  - palette application across the defined dashboard surfaces
  - preserved `NORMAL` / `WARNING` / `CRITICAL` distinction
  - visible active indication
  - readability requirements for device-mode and simulation-mode text

Recommended verification mappings:

- `SWR-GUI-013` -> `src/gui_main.c`, `src/app_config.c`, `include/app_config.h`
  with `ConfigTest.*` extensions plus manual GUI review
- `SWR-GUI-014` -> `src/gui_main.c`, `src/localization.c`,
  `include/localization.h` with manual GUI review in normal, warning, and
  critical states

Existing requirements that must remain unchanged in intent:

- `SYS-014` color-coded dashboard behavior
- `SYS-015` HAL behavior
- `SWR-GUI-010` simulation toggle
- `SWR-GUI-011` simulation banner behavior
- `SWR-GUI-012` localization selection and persistence
- all alert, NEWS2, alarm-limit, authentication, and patient-record
  requirements

## Medical-safety, security, and privacy impact

Medical safety impact is low for direct function and medium for presentation
integrity. The feature does not alter clinical computation, but it does affect
how quickly a clinician perceives urgency on screen.

Safety constraints:

- preserve one-glance readability of warning and critical states
- keep all values, thresholds, and status wording unchanged
- keep the mode visibly identifiable when active
- default to standard mode on invalid or missing saved preference
- avoid cross-user persistence bleed on shared workstations

Security impact is low. The feature stores only a non-clinical display
preference and does not change authentication decisions, roles, or credential
storage. Do not move the preference into `users.dat` or tie it to privileged
operations.

Privacy impact is low to none. No patient data, alarm history, or new export
path is introduced. The only added persisted information should be a local
display preference associated with an existing username.

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none
- Output: none
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged beyond ordinary UI clarity
- Dataset and bias considerations: none
- Monitoring expectations: none beyond normal regression and usability review
- PCCP impact: none

## Validation plan

Primary implementation-time checks:

- verify night mode can be toggled on and off from the dashboard header without
  restart
- verify the active mode is visibly indicated and is not represented by color
  alone
- verify standard-mode and night-mode dashboards show identical values, alert
  levels, NEWS2 scores, and active-alert content
- verify normal, warning, and critical states remain clearly distinguishable in
  both tile and banner surfaces
- verify device mode and simulation mode messaging remain readable in both
  themes
- verify user A can enable night mode, user B still starts in standard mode,
  and user A's preference is restored when that user logs in again
- verify missing or malformed saved night-mode data falls back to standard mode
- verify existing `sim_enabled` and `language` settings survive night-mode
  writes unchanged

Recommended automated checks during implementation:

```powershell
run_tests.bat
ctest --test-dir build --output-on-failure -R Config|Localization
rg -n "night|Night Mode|night_mode|SWR-GUI-013|SWR-GUI-014|SYS-020|UNS-017" src include requirements tests
git diff --stat
```

Required manual GUI scenarios:

- standard mode and night mode with all-normal data
- standard mode and night mode with warning-state data
- standard mode and night mode with critical-state data
- device mode with night mode enabled
- simulation paused and simulation active with night mode enabled
- logout/login cross-user handoff on a shared workstation
- language change while night mode is active

## Rollback or failure handling

If implementation cannot add safe user-scoped persistence to `monitor.cfg`
without destabilizing `sim_enabled` and `language` behavior, do not silently
replace the design with a workstation-global night-mode flag. Reopen the scope
decision or split persistence into a follow-on issue rather than shipping a
cross-user handoff hazard.

If a proposed low-glare palette weakens warning or critical visibility, keep the
existing standard palette for those surfaces until the contrast issue is fixed.
Do not ship a partially dimmed theme that makes severe states less legible.

If themed post-login dialogs would require a large owner-draw refactor, keep the
issue focused on the dashboard-first path, but do not regress readability and do
not claim the feature covers more surfaces than were actually updated.
