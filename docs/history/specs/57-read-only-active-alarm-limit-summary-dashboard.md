# Design Spec: Read-only active alarm-limit summary on the dashboard

Issue: #57
Branch: `feature/57-read-only-active-alarm-limit-summary-dashboard`
Spec path: `docs/history/specs/57-read-only-active-alarm-limit-summary-dashboard.md`

## Problem

Issue #57 asks for a compact, read-only dashboard summary of the active alarm
limits so operators can confirm threshold context without opening Settings.
That user need is reasonable under `UNS-010` and `UNS-014`, but the current
repository state does not yet provide a single authoritative "active alarm
limit" source that the dashboard can safely surface.

Today the dashboard and alert-generation path still classify readings with
fixed threshold helpers in `src/vitals.c` and `src/alerts.c`, while the
editable alarm-limit values shown in the Settings tab live separately in
`g_app.alarm_limits` and `alarm_limits.cfg`. A dashboard summary of
`g_app.alarm_limits` would therefore risk showing values that are not the
thresholds currently driving tile severity, active-alert text, or patient
status. That is safety-relevant because the feature is intended to help humans
interpret alarm context during monitoring and handoff.

The issue is therefore not ready for a pure presentation-only implementation.
It needs an explicit source-of-truth decision and a narrowed follow-up plan
before implementation can safely proceed.

## Goal

Define a safe design path for issue #57 that:

- preserves the operator need for on-dashboard threshold context,
- avoids displaying threshold values that do not match actual alert behavior,
- keeps the MVP read-only and visually subordinate to active alerts, and
- identifies the prerequisite decisions required before any implementation work
  is moved to `ready-for-implementation`.

The intended design outcome for this issue is a bounded split proposal and a
traceable MVP specification, not immediate authorization to implement the
summary on the current codebase as-is.

## Non-goals

- No direct implementation of a dashboard summary that merely mirrors
  `g_app.alarm_limits` while `src/vitals.c` and `src/alerts.c` remain the
  alerting authority.
- No silent clinical-behavior change that retargets alarm evaluation from
  fixed thresholds to configurable thresholds without human-approved acceptance
  criteria.
- No new inline editing, preset management, export, print, or persistence
  workflow changes on the dashboard.
- No vendor-like monitor mimicry, proprietary icon copying, or branded badge
  design.
- No default/custom comparator badge in the MVP until the authoritative
  default baseline is resolved.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact:
- Give authenticated operators a quicker way to inspect the threshold context
  that applies to the current monitoring session without opening the Settings
  dialog.
- Support faster review and handoff, but not change alarm generation,
  diagnosis, escalation, or clinical recommendations.

User population:
- Trained clinical staff and ward operators already authorized to use the
  dashboard.

Operating environment:
- Authenticated Windows workstation sessions running the current Win32
  dashboard in simulation mode today and equivalent live-monitoring sessions in
  the same application architecture later.

Foreseeable misuse:
- An operator assumes the summary reflects the thresholds currently driving
  alert severity when it does not.
- An operator assumes the summary is editable or that it confirms a save
  action.
- An operator assumes the displayed limits are per-patient when the current
  implementation is session-scoped and persisted globally.
- The summary crowds or visually weakens the patient bar, active-alert list, or
  status banner.
- A future default/custom badge creates false confidence because requirements,
  code, and tests disagree on the factory baseline.

## Current behavior

Relevant current repository behavior:

- `alarm_limits_load(&g_app.alarm_limits)` runs during dashboard startup in
  `src/gui_main.c`, restoring one persisted limit set into application state.
- The Settings "Alarm Limits" tab in `src/gui_main.c` displays and edits
  `g_app.alarm_limits`, with `Apply & Save` writing the edited values back to
  `alarm_limits.cfg`.
- `Reset Defaults` also overwrites `g_app.alarm_limits` in memory immediately,
  before any save operation, and repopulates the Settings fields from that new
  in-memory state.
- The dashboard tiles in `paint_tiles()` still call fixed-threshold helpers
  such as `check_heart_rate()`, `check_blood_pressure()`,
  `check_temperature()`, `check_spo2()`, and `check_respiration_rate()`.
- `update_dashboard()` still uses `generate_alerts()` for the alert list, and
  `src/alerts.c` formats alert messages with hard-coded "normal" ranges such as
  `60-100`, `90-140 / 60-90`, `36.1-37.2`, `95-100`, and `12-20`.
- `src/vitals.c`, `src/alerts.c`, `tests/unit/test_vitals.cpp`, and
  `tests/unit/test_alerts.cpp` therefore encode a fixed-threshold clinical
  model that is separate from `src/alarm_limits.c` and
  `tests/unit/test_alarm_limits.cpp`.

Current design consequence:

- The repository does not yet have a safe, single runtime definition of
  "active alarm limits" for dashboard interpretation.
- A read-only summary cannot honestly claim to show the active threshold
  context until the authoritative threshold model is resolved.

## Proposed change

### Decision

Do not advance issue #57 directly to implementation as a presentation-only
dashboard enhancement on the current codebase.

Instead, split the work into a prerequisite authority-alignment step and a
follow-on dashboard-summary step.

### Recommended split

1. Authority alignment issue

Define and approve the single authoritative threshold model that the product
means by "active alarm limits."

Recommended direction if Product still wants the dashboard summary:
- Make one approved alarm-limit model authoritative across dashboard
  classification, alert generation, and the eventual summary display.
- Resolve whether configurable limits are meant to replace the fixed threshold
  literals currently embedded in `src/vitals.c` and `src/alerts.c`, or whether
  the configurable settings UI should be re-scoped because it does not
  represent actual alert behavior today.

Required human decisions before this authority-alignment work can be treated as
ready:
- Approve the clinical acceptance criteria for any change that alters which
  thresholds drive alert severity.
- Approve the factory-default baseline because `requirements/SWR.md` and the
  implemented/tested defaults in `src/alarm_limits.c` do not match.
- Approve the intended `Reset Defaults` semantics if configurable limits become
  authoritative, because the current code changes in-memory state immediately
  before save.

2. Dashboard summary issue

After authority alignment is complete, implement a new read-only dashboard
summary with these MVP constraints:

- Bind the summary to the same authoritative runtime threshold object used by
  dashboard severity and alert generation.
- Render a compact textual strip, not an editable form and not a branded
  workstation mimic.
- Preserve parameter order and units already used in Settings:
  HR, SBP, DBP, Temp, SpO2, RR.
- Show a clear read-only cue in text or layout.
- Omit any default/custom badge from the MVP.
- Keep the summary visually subordinate to active alerts and the status banner.
- Localize any new user-visible static text through the existing localization
  layer.

Recommended placement:
- Add a dedicated summary strip between the patient bar and the tile grid, not
  inside the status banner and not inside the active-alert list.
- Adjust the dashboard layout constants (`TILE_Y`, `STAT_Y`, `CTRL_Y`, and the
  dependent control placement) to create real space for the summary rather than
  overloading an existing safety-critical area.

Recommended refresh triggers:
- Dashboard initialization after limits are loaded.
- Any approved Settings action that changes the authoritative active limits.
- Dashboard language refresh if the summary includes localized heading or
  read-only text.

Explicitly rejected implementation path:
- Do not add a summary that mirrors `g_app.alarm_limits` while tiles and alert
  records continue to use hard-coded thresholds elsewhere.

## Files expected to change

Because the issue needs a split, the expected implementation files are broader
than the original issue description implied.

Authority-alignment step is expected to inspect and likely change:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `src/gui_main.c`
- `src/alerts.c`
- `include/alerts.h`
- `src/vitals.c` and/or a replacement domain service that defines the approved
  authoritative threshold evaluation path
- `include/vitals.h` if public evaluation interfaces change
- `src/patient.c`
- `src/main.c`
- `tests/unit/test_alerts.cpp`
- `tests/unit/test_vitals.cpp`
- `tests/unit/test_alarm_limits.cpp`
- affected integration or DVT evidence if alert behavior changes

Dashboard-summary step is expected to inspect and likely change:

- `src/gui_main.c`
- `src/localization.c`
- `include/localization.h`
- `tests/unit/test_localization.cpp` if localization coverage needs extension
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Files that should not change in the dashboard-summary step:

- networking or cloud code
- authentication or user-management behavior beyond existing role checks
- patient export, print, or persistence formats
- AI/ML components, because none are involved here

## Requirements and traceability impact

Existing requirement foundations are relevant:

- `UNS-010` consolidated status summary
- `UNS-014` graphical dashboard
- `SYS-014` graphical dashboard
- `SWR-GUI-003` dashboard repaint and tile presentation
- `SWR-GUI-004` dashboard control layout
- `SWR-GUI-012` localization
- `SWR-ALM-001` configurable alarm limits

However, the approved requirements do not currently define a dashboard surface
for alarm-limit context, and current implementation evidence does not support a
single authoritative threshold model.

Required requirement work before implementation closeout:

- Add a new system-level requirement for exposing active alarm-limit context on
  the dashboard, for example `SYS-020`, traced to `UNS-010` and `UNS-014`.
- Add a new software requirement for the read-only summary itself, for example
  `SWR-GUI-013`, traced to the new system requirement and implemented in the
  dashboard layer.
- Update traceability rows for the new requirement(s), their implementation
  points, and their verification assets.
- Resolve the mismatch between documented factory defaults in `SWR-ALM-001`
  and the defaults implemented and tested in `src/alarm_limits.c` and
  `tests/unit/test_alarm_limits.cpp`.

Clinical-logic warning:
- If configurable limits become the authoritative threshold source, that is not
  a cosmetic traceability update. It changes the behavior that determines alert
  severity and therefore needs explicit human-approved acceptance criteria
  before implementation.

## Medical-safety, security, and privacy impact

Medical-safety impact is moderate until the source-of-truth conflict is
resolved.

Primary safety risk:
- a summary claims to show active alarm context while the real alerting path is
  still driven by different thresholds.

Required controls:

- Do not implement the summary until the authoritative threshold model is
  explicit and approved.
- Keep the summary read-only and visually distinct from editable Settings
  controls.
- Preserve the visual priority of active alerts, tile severity, and the status
  banner.
- Use the same approved parameter names, order, and units as the authoritative
  threshold model.
- Do not ship a default/custom badge until the default baseline is reconciled.

Security impact:
- Low. No new network path, credential flow, or privilege boundary is needed.
- Integrity still matters because an incorrect summary would misrepresent
  monitoring context.

Privacy impact:
- Low. No new patient-data category or external disclosure path is introduced.

## AI/ML impact assessment

This change does not add, remove, modify, or depend on any AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing non-AI vital-sign and configuration data
- Output: none from any AI/ML model
- Human-in-the-loop limits: not applicable
- Transparency needs: not applicable beyond normal UI clarity
- Dataset and bias considerations: none
- Monitoring expectations: none specific to AI/ML
- PCCP impact: none

## Validation plan

Design-gate validation before implementation:

- Confirm Product and the human owner choose the authoritative active
  threshold model.
- Confirm the approved factory-default baseline for any future comparator or
  reset behavior.
- Confirm whether `Reset Defaults` is intended to change active in-session
  thresholds immediately or only stage values until Apply.

If authority alignment is approved for implementation:

- Add or update automated tests proving that dashboard severity and
  alert-generation paths use the same threshold source.
- Update alert-message tests so any displayed normal ranges match the approved
  authoritative model.
- Re-run affected unit and integration suites for alert evaluation and patient
  status.

If the dashboard summary step is approved after authority alignment:

- Add focused tests for any formatter or helper that builds the summary text.
- Add or update localization coverage for any new heading/read-only strings.
- Perform GUI smoke checks showing that the summary:
  - appears in the intended layout strip,
  - matches the authoritative limits on dashboard startup,
  - updates after any approved Settings action that changes active limits, and
  - does not obscure the status banner, tiles, or active-alert list.

Regression expectations:

- Verify no heap allocation is introduced.
- Verify role-based access remains unchanged.
- Verify requirement and traceability documents are updated together with the
  implementation.

## Rollback or failure handling

If the authoritative threshold model cannot be approved, do not implement the
dashboard summary. Keep the work blocked or split until the product owner makes
that decision.

If implementation begins and expands into unapproved clinical-threshold
behavior changes, stop and revert that expansion before handoff.

If the only implementable path is a summary that does not match the thresholds
actually driving alerts, reject that path as unsafe and incomplete rather than
shipping a misleading read-only display.
