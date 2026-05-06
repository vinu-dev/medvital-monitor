# Design Spec: Add an alarm-audio status badge

Issue: #64
Branch: `feature/64-alarm-audio-status-badge`
Spec path: `docs/history/specs/64-alarm-audio-status-badge.md`

## Problem

Issue #64 asks for a small dashboard badge that tells an operator whether local
alarm audio is currently audible or silenced. The current Win32 dashboard does
not provide that signal anywhere in the UI.

The current codebase also does not contain a local alarm-audio subsystem,
silence workflow, or authoritative "audio state" model. The existing dashboard
shows:

- the logged-in user and role badge in the header
- a simulation live/paused indicator in the header
- patient-severity tiles, alert lists, and the status banner for clinical state

That creates a design risk: adding a new badge without a precise source-of-truth
model could mislead clinicians into reading simulation state, alert severity, or
general device mode as if it were local alarm-audio state. The issue is narrow
enough to design, but it needs explicit requirements for state semantics,
wording, fail-safe behavior, and layout.

## Goal

Produce a narrow implementation plan for a read-only dashboard indicator that
reports the current local alarm-audio presentation state without changing alarm
thresholds, alert generation, NEWS2 behavior, routing, or operator controls.

The intended outcome is:

- the dashboard header contains a dedicated alarm-audio badge
- the badge is clearly secondary to clinical alert indicators
- the badge reports only states backed by an authoritative local source
- the badge fails safe to `Unknown` when the application cannot determine the
  true local audio state

## Non-goals

- No alarm-threshold, alert-generation, NEWS2, authentication, persistence, or
  hardware-acquisition behavior change.
- No clickable mute, silence, acknowledge, route, or remote-alarm control.
- No claim that alarms are heard elsewhere or routed remotely.
- No inference of alarm-audio state from simulation enabled/disabled, simulation
  pause/resume, active-alert presence, or patient severity.
- No attempt to retrofit or invent an audio-output subsystem in this issue.
- No reuse of red/amber/green patient-risk color semantics for the audio badge.

## Intended use impact, user population, operating environment, and foreseeable misuse

- Intended use impact: improve local operator awareness of whether this
  workstation is expected to emit local alarm audio.
- User population: trained bedside clinicians and administrators using the
  Windows dashboard in the pilot environment.
- Operating environment: the existing single-process Win32 dashboard at the
  current fixed window width (`WIN_CW = 920`), with simulation today and future
  HAL-backed live acquisition.
- Foreseeable misuse:
  - a user may read `Silenced` as "no active alarm"
  - a user may assume `Audible` means alarm delivery is functioning elsewhere
  - a user may trust a stale state after login/logout or mode transitions
  - a user may read the badge as a clinical severity indicator if the styling is
    too close to the current alert colors

## Current behavior

Current header behavior in `src/gui_main.c` is tightly hand-positioned:

- username text is drawn in the right-side info block
- the role pill is drawn in the same block
- the simulation indicator is drawn as text below that block
- header buttons occupy fixed positions at the right edge

Current repo evidence does not show any authoritative local alarm-audio state:

- no alarm-audio badge is rendered today
- no local audio-state enum or service exists
- no sound, speaker, mute, silence, or alarm-audio control path exists in
  `src/**` or `include/**`
- `monitor.cfg` persists simulation mode and language only
- localization contains no strings for alarm-audio state labels

Because the source-of-truth is absent, a two-state `Audible` / `Silenced` badge
cannot be made trustworthy by presentation logic alone.

## Proposed change

Implement the feature as a presentation-only, tri-state badge with explicit
fail-safe behavior, even though the nominal operator-facing states remain
`Audible` and `Silenced`.

### 1. Add an explicit local alarm-audio state model

Define a small application-layer state model for the dashboard:

- `ALARM_AUDIO_AUDIBLE`
- `ALARM_AUDIO_SILENCED`
- `ALARM_AUDIO_UNKNOWN`

This model must represent only the local workstation's audio presentation
expectation. It must not collapse remote routing, unsupported hardware, stale
state, or device mode into `Silenced`.

If implementation cannot identify an authoritative local source of truth for the
current pilot, the badge shall remain `Unknown` rather than assert `Audible`.

### 2. Render a dedicated read-only badge in the dashboard header

Add a small neutral-styled badge to the header cluster near the existing role
and simulation indicators. The layout must be updated deliberately for the
fixed-width `920px` window so the new badge does not overlap:

- the app title
- username text
- role badge
- simulation live/paused indicator
- Settings, Pause/Resume, Logout, or Account buttons

The safest layout is to treat the right-side header content as a compact
two-row status cluster left of the button group, with username truncation if
needed. The implementation should not assume dynamic responsive layout that does
not exist in the current Win32 UI.

### 3. Keep the badge semantically separate from clinical severity

Use neutral wording and neutral styling. The badge should read as a workstation
status indicator, not as a patient-risk badge.

Recommended visible text:

- `Audio: Audible`
- `Audio: Silenced`
- `Audio: Unknown`

Do not use icon-only semantics. Do not reuse the current green/amber/red tile
palette for the badge state itself.

### 4. Do not persist the audio-state badge in `monitor.cfg`

This issue should not add `alarm_audio_state` persistence to `monitor.cfg`.
Persisting a stale audio state across restarts or login transitions would create
false reassurance and would also broaden the configuration surface without a
real audio subsystem.

Instead:

- initialize the badge state to `Unknown` on dashboard creation or login
- reset it to `Unknown` on logout
- reset or recompute it on simulation/device-mode transitions only if a real
  source-of-truth exists

### 5. Add explicit requirements and traceability

Add new requirements for this behavior instead of overloading existing alarm or
simulation requirements.

Add `SYS-020` to `requirements/SYS.md` with language equivalent to:

`The system shall display the current local alarm-audio presentation state in
the dashboard header as Audible, Silenced, or Unknown when the local state is
unavailable or indeterminate. The indication shall be informational only and
shall not alter alarm thresholds, alert generation, or alarm routing.`

Trace `SYS-020` to:

- `UNS-014` graphical dashboard

Add `SWR-GUI-013` to `requirements/SWR.md` with language equivalent to:

`The dashboard shall render a dedicated non-clinical alarm-audio badge sourced
from an authoritative local alarm-audio state. The badge shall support
Audible, Silenced, and Unknown states; shall show Unknown whenever the local
audio state is unavailable, stale, unsupported, or not yet established; shall
be display-only; and shall remain visually distinct from patient severity
status colors.`

Trace `SWR-GUI-013` to:

- `SYS-020`

Update `requirements/TRACEABILITY.md` accordingly. This should increase the
approved counts by one SYS and one SWR, with no UNS-count change.

### 6. Extend localization for all supported UI languages

The current localization layer supports English, Spanish, French, and German.
Implementation should add localized strings for:

- the `Audio` label
- `Audible`
- `Silenced`
- `Unknown`

The issue must not hard-code English-only badge text in `gui_main.c`.

### 7. Keep implementation scope bounded to UI/status-state plumbing

If the implementer discovers that the feature cannot be delivered without adding
an actual local audio-control workflow, stop and escalate rather than silently
expanding the issue into alarm-control behavior.

## Files expected to change

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `README.md`

Expected test or verification-support files:

- `tests/unit/test_localization.cpp`

Optional implementation file only if the team chooses to isolate state mapping
outside paint code for unit testing:

- `include/alarm_audio_state.h`
- `src/alarm_audio_state.c`
- `tests/unit/test_alarm_audio_state.cpp`

Files that should not change:

- `src/vitals.c`
- `src/alerts.c`
- `src/patient.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/gui_users.c`
- `src/gui_auth.c`
- `include/hw_vitals.h`
- `src/app_config.c`
- `include/app_config.h`
- `.github/workflows/**`

## Requirements and traceability impact

This issue introduces new approved requirements for a non-clinical but
safety-adjacent UI signal.

Impacted UNS layer:

- `UNS-014`

Impacted SYS layer:

- new `SYS-020` for local alarm-audio state indication

Impacted SWR layer:

- new `SWR-GUI-013` for header badge behavior, wording, and fail-safe state

Traceability impact:

- add a new `UNS-014 -> SYS-020 -> SWR-GUI-013` chain
- add implementation and verification evidence rows for `SWR-GUI-013`
- update totals in `requirements/TRACEABILITY.md`

This issue should not change any existing `@req` tags for alert classification,
NEWS2, alarm limits, simulation mode, authentication, or persistence.

## Medical-safety, security, and privacy impact

Medical-safety impact is low for direct function and medium for misleading
presentation risk if the badge is implemented carelessly.

Safety constraints:

- the badge must remain secondary to the existing alert tiles, alert list, and
  status banner
- the badge must never imply "no alarm condition"
- the badge must never claim remote coverage or routed delivery
- unknown or unsupported state must degrade to `Unknown`, not to `Audible`

Security impact is low if the feature stays display-only. No new remote control,
network path, or permission boundary is needed.

Privacy impact is none expected. No patient-data field, export, or storage path
changes.

This issue must explicitly call out that it does not change vital
classification, NEWS2 scoring, alarm limits, authentication, persistence, or
clinical workflow.

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none
- Output: none
- Human-in-the-loop limits: not applicable
- Transparency needs: not applicable beyond ordinary UI wording clarity
- Dataset and bias considerations: none
- Monitoring expectations: none beyond ordinary UI regression verification
- PCCP impact: none

## Validation plan

Use bounded implementation and UI verification focused on state semantics and
layout.

Requirements and localization checks:

```powershell
rg -n "SYS-020|SWR-GUI-013|Audio|Audible|Silenced|Unknown" requirements src include README.md
```

Diff-scope checks:

```powershell
git diff --stat
git diff -- src/gui_main.c include/localization.h src/localization.c requirements/SYS.md requirements/SWR.md requirements/TRACEABILITY.md README.md
```

Manual GUI verification:

- verify the badge renders without header overlap at the default `920px` width
- verify the badge is visible after login and is cleared on logout
- verify `Unknown` is shown whenever no authoritative state has been established
- verify `Silenced` does not suppress or restyle active clinical alerts
- verify simulation enabled/disabled and pause/resume do not by themselves
  change the badge unless explicitly wired to a real audio-state source
- verify the badge remains visually distinct from patient-severity colors

Automated verification expectations:

- extend localization tests for any new string IDs
- add targeted unit tests only if state resolution is moved into a helper module

No alert-classification, NEWS2, authentication, or config-persistence regression
tests are required unless implementation broadens beyond this design.

## Rollback or failure handling

If implementation cannot identify a trustworthy local source of alarm-audio
state, keep the badge in `Unknown` and document the follow-up dependency rather
than fabricating `Audible` / `Silenced` semantics.

If the only way to make the badge meaningful is to add a local mute/silence
workflow, stop and return the item to design instead of broadening the issue
silently.

If the header layout cannot accommodate the new badge cleanly at the current
fixed width, revert any unstable placement changes and revise the layout plan
before shipping.
