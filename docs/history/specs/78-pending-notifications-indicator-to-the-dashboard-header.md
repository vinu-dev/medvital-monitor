# Design Spec: Issue 78

Issue: `#78`  
Branch: `feature/78-pending-notifications-indicator-to-the-dashboard-header`  
Spec path: `docs/history/specs/78-pending-notifications-indicator-to-the-dashboard-header.md`

## Problem

The dashboard header currently exposes only the app title, logged-in user,
role badge, and simulation state. The banner area below the tiles does not
represent a true pending-notice queue: in device mode it shows a static
device-mode message, and in simulation mode it scrolls a repeated localized
simulation string whose background is derived from patient alert severity.
`AppState` in `src/gui_main.c` holds patient/session state plus
`sim_msg_scroll_offset`, but no session-scoped notification state.
`update_dashboard()` refreshes the alerts/history list boxes only, so there is
no existing path that tells the header whether a non-alarm operational message
is still pending review.

Issue `#78` therefore cannot be implemented as "just add a badge." The design
has to define:

- what message class the indicator covers
- where the pending state lives
- how the banner and header stay in sync
- when the state is cleared
- how the cue stays visually distinct from clinical alarm semantics

PR `#79` (`feature/52-active-patient-identity-dashboard-header`) is also open
against the same `paint_header()` seam, so implementation needs an explicit
merge-order/layout plan rather than another independent header widget.

## Goal

Add a compact, read-only header indicator that tells an authenticated operator
when a non-alarm operational notice is pending in the current dashboard
session.

The MVP shall:

- cover non-alarm dashboard/session notices only
- treat the banner as the primary notice surface and the header badge as a
  secondary cue
- use a binary pending/not-pending state, not a queue browser or alarm count
- remain session-scoped, non-persistent, and privacy-minimal
- stay outside alert generation, NEWS2, thresholds, authentication, and
  patient-record logic

## Non-goals

- Any alarm acknowledgement, silencing, escalation, or triage workflow
- Reusing `generate_alerts()` output or active-alert list contents as
  notification state
- A notification history, inbox, export, persistence, or cross-session
  carry-over
- Header display of PHI, detailed message text, or patient-specific clinical
  content
- Copying competitor iconography, colors, wording, or interaction patterns
- Full header redesign unrelated to the indicator, or changes to domain /
  persistence modules

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- Improves visibility of non-alarm operational notices that the operator should
  review in the current session.
- Does not change clinical interpretation, alarm generation, or treatment
  guidance.

User population:

- Authenticated clinical users using the Win32 dashboard
- Authenticated admin users who also use the same dashboard view

Operating environment:

- Current Win32 desktop dashboard implemented in `src/gui_main.c`
- Existing minimum dashboard width of `760 x 640`
- Session states driven by Admit, Add Reading, Clear Session, simulation mode
  changes, pause/resume, demo scenarios, timer updates, and logout

Foreseeable misuse:

- User mistakes the indicator for an alarm or alert-severity badge
- User expects the indicator to acknowledge, dismiss, or open a notifications
  tray
- Indicator remains set after the underlying notice is cleared or superseded
- Header displays detailed notice text or PHI in an always-visible area
- Independent work from issue `#52` creates overlapping header widgets or
  conflicting layout rules

## Current Behavior

- `src/gui_main.c:192-217` defines `AppState` with patient/session fields,
  `sim_enabled`, `sim_paused`, and `sim_msg_scroll_offset`, but no
  pending-notice state.
- `src/gui_main.c:292-327` paints the header with:
  - app title
  - logged-in display name
  - role badge
  - simulation status text when simulation is enabled
- `src/gui_main.c:534-576` paints the banner with:
  - a centered device-mode message when simulation is off
  - a repeated rolling simulation message when simulation is on
  - background colors derived from aggregate alert severity even though the
    banner text is non-alarm
- `src/gui_main.c:680-723` repopulates the active-alert and reading-history
  list boxes, but does not create or resolve a pending-notice concept.
- `src/gui_main.c:764-823`, `1580-1606`, and `1749-1818` already define the
  transition points that any pending-notice state must honor:
  - admit / refresh
  - add reading
  - clear session
  - demo scenario load
  - simulation enable / disable
  - timer-driven updates
  - pause / resume
  - logout
- The repo already has localized string infrastructure in
  `include/localization.h` and `src/localization.c`, so any new visible notice
  labels or banner text should route through that layer.
- Open PR `#79` (issue `#52`) already changes `src/gui_main.c`,
  `include/localization.h`, `src/localization.c`, and requirements docs for a
  new header patient-identity card. That is a direct header-layout dependency
  for this issue.

## Proposed Change

1. Define one approved notice class boundary for this feature: non-alarm
   operational notices generated inside the dashboard UI flow. Covered examples
   may include session/mode notices such as simulation paused, session cleared,
   or other human-approved operational states. Excluded examples include
   physiological alerts, technical alarms, NEWS2 status, alarm limits, patient
   identity, or anything derived from `generate_alerts()`.
2. Add a bounded session-scoped notice model in `src/gui_main.c`, owned by
   `AppState`. Recommended shape:
   - a fixed enum or localized string ID describing the active notice type
   - a binary `pending` flag
   - no heap allocation
   - no persistence to config or disk
   - no detailed free-form text buffer unless a fixed-size, localized enum
     approach proves insufficient
3. Keep the status banner as the primary notice surface. Recommended behavior:
   - if a pending operational notice exists, `paint_status_banner()` renders
     that notice first
   - if no pending notice exists, the banner falls back to the normal
     device/simulation baseline behavior
   - pending operational notices use a neutral visual treatment that is
     distinct from red/amber clinical-alert semantics
4. Add a neutral, read-only header indicator in `paint_header()` that reflects
   only whether a pending operational notice exists. MVP recommendation:
   - binary on/off cue only
   - neutral label or iconography such as `Notice` or `1 pending notice`
   - no click target, no dropdown, no message text, no patient identifiers
   - no alarm colors, pulsing, or alarm-count semantics
5. Centralize notice state transitions in helper functions instead of open-coded
   flag edits across the window procedure. The helper path must define set,
   supersede, and clear behavior for:
   - logout
   - clear session
   - patient replacement / re-admit
   - simulation enable / disable
   - pause / resume
   - any explicit notice dismissal workflow approved for the MVP
   - any event that replaces one pending notice with another
6. Keep implementation localized to GUI presentation/state seams. Expected
   runtime scope:
   - `src/gui_main.c`
   - `include/localization.h`
   - `src/localization.c`
   - requirements and traceability docs
   No changes should be routed through `alerts.c`, `patient.c`, `news2.c`,
   alarm-limit logic, authentication, or persistence formats.
7. Coordinate with issue `#52` header work before implementation lands.
   Recommended merge-order rule:
   - if PR `#79` merges first, reuse its header layout helper(s) and place the
     notice indicator inside the same consolidated header information cluster
   - if issue `#78` is implemented first, the resulting header layout must
     still leave room for the patient-identity card expected by issue `#52`
   - in either case, logout/settings/pause controls, logged-in user, and role
     badge keep visibility priority over the new indicator
8. Prefer a binary notice model for the MVP. A count-based indicator is not
   recommended in this issue because the current repo has no notice queue, no
   review workflow, and no persistence model. Adding a count without those
   boundaries raises stale-state risk and expands scope.

## Files Expected To Change

Expected implementation files:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `tests/unit/test_localization.cpp` if new localized string IDs are added
- manual verification notes / DVT evidence only if the repo requires an
  explicit GUI evidence update for the new requirement text

Files expected not to change:

- `src/alerts.c`
- `src/patient.c`
- `src/vitals.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/gui_users.c`
- `src/gui_auth.c`
- persistence formats such as `monitor.cfg`, `users.dat`, or alarm-limit
  storage

## Requirements And Traceability Impact

The current requirements set is not sufficient as-is for this feature.

- `SYS-014` and `SWR-GUI-003` cover the graphical dashboard and alert-color
  display, but they do not define a persistent non-alarm notice cue in the
  header.
- `SWR-GUI-011` already describes banner behavior that does not match the
  current implementation baseline, so issue `#78` should not layer new
  behavior on top of that mismatch.

Recommended requirement plan:

1. Add or revise one SYS-level requirement for non-alarm operational notice
   visibility in the authenticated dashboard. Preferred approach: add a new SYS
   entry rather than overloading `SYS-014`, because the existing SYS text is
   focused on vital tiles and aggregate alert display.
2. Revise `SWR-GUI-011` so it defines the primary status-banner behavior for
   non-alarm operational notices, including:
   - notice priority over default banner text
   - neutral visual treatment
   - reset / supersede semantics
   - exclusion from alarm semantics
3. Add one new GUI requirement for the header indicator itself (next available
   `SWR-GUI-xxx`; re-check numbering if issue `#52` lands first), covering:
   - read-only header cue
   - binary pending state
   - no PHI / no detailed text
   - session-scoped reset rules
   - non-interference with higher-priority header controls and badges
4. Update `requirements/TRACEABILITY.md` so the new/revised requirements trace
   to:
   - `src/gui_main.c`
   - localization files if new strings are introduced
   - manual GUI verification evidence for set/clear/reset behavior
   - localization regression coverage if new string IDs are added to
     `test_localization.cpp`

No new AI, alarm, authentication, or persistence trace chains should be
introduced for this issue.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- Direct clinical impact remains low because the change is display-only and
  must not alter vitals, alert generation, NEWS2 scoring, alarm limits, or
  patient-record logic.
- The primary safety hazard is visual ambiguity or stale state. The indicator
  and any pending-notice banner state must stay clearly non-alarm and must
  clear deterministically on session transitions.

Security:

- No new auth, authorization, network, or privilege boundary is expected.
- The feature must remain inside the authenticated dashboard session and must
  not create a new action surface.

Privacy:

- Header content must be privacy-minimal.
- Do not show patient identifiers, detailed notice text, or clinically
  sensitive content in the indicator itself.
- Do not persist notice state or write it to logs / exports.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: not applicable beyond existing non-AI session state
- Output: not applicable
- Human-in-the-loop limits: not applicable
- Transparency needs: standard UI clarity only
- Dataset and bias considerations: not applicable
- Monitoring expectations: not applicable
- PCCP impact: none

## Validation Plan

Repository checks:

```powershell
rg -n "SWR-GUI-011|SWR-GUI-|SYS-0|Notice|notification|pending" requirements src include tests
git diff --name-only
```

Automated baseline:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Targeted regression if new localized string IDs are added:

```powershell
ctest --test-dir build --output-on-failure -R Localization
```

Manual GUI verification:

1. Trigger each approved non-alarm operational notice source and confirm the
   banner becomes the primary message surface while the header indicator becomes
   visible.
2. Confirm the indicator uses neutral, non-alarm styling and does not reuse
   red/amber alarm semantics.
3. Confirm dismiss / supersede behavior clears or updates the indicator and
   banner together.
4. Confirm logout, Clear Session, patient re-admit, simulation enable / disable,
   and pause / resume reset the pending state deterministically.
5. Confirm no detailed notice text or PHI appears in the header.
6. Confirm active-alert list behavior, `generate_alerts()`, tile colors, and
   NEWS2 behavior are unchanged.
7. Confirm the new indicator does not overlap logout/settings/pause controls,
   the user name, the role badge, or the patient-identity card from issue
   `#52` at the minimum width `760 x 640`.
8. If requirement numbering changed because issue `#52` merged first, confirm
   the final SWR/SYS IDs and RTM rows are internally consistent.

## Rollback Or Failure Handling

- If the team cannot define a narrow non-alarm notice class without expanding
  into a queue, inbox, or acknowledgement workflow, stop and split the issue
  rather than implementing a vague "notification" concept.
- If the header cannot accommodate both issue `#52` and issue `#78` cues
  without obscuring higher-priority controls at minimum width, rebase onto the
  resolved header layout or split a dedicated header-layout issue.
- If requirement owners do not approve the SYS/SWR changes needed to define
  banner priority and reset semantics, do not implement the runtime change
  under implicit requirements.
- Rollback is straightforward: revert the notice-state, header-indicator,
  localization, and requirements changes together so the dashboard returns to
  its prior no-pending-notice behavior.
