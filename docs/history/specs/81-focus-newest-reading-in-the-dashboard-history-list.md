# Design Spec: Issue 81

Issue: `#81`  
Branch: `feature/81-focus-newest-reading-in-the-dashboard-history-list`  
Spec path: `docs/history/specs/81-focus-newest-reading-in-the-dashboard-history-list.md`

## Problem

The dashboard already stores and renders bounded session history, but the raw
`Reading History` list does not automatically land on the newest row after a
refresh.

- `src/gui_main.c` `update_dashboard()` clears `IDC_LIST_HISTORY` and rebuilds
  it in chronological order from `g_app.patient.readings[0..reading_count-1]`.
- The newest row is appended last, so it can fall below the visible viewport as
  the session fills.
- Current-state surfaces already exist elsewhere:
  `patient_latest_reading()` drives the live tiles and `generate_alerts()`
  drives `IDC_LIST_ALERTS`.

That means the operator can still recover the latest raw row, but only after
manual scrolling or scanning through prior entries.

## Goal

Add a narrow, read-only GUI behavior so each dashboard refresh:

- preserves the existing chronological history rows
- selects the newest available history row
- scrolls that row into view
- does not steal keyboard focus from another active control

## Non-goals

- Changing stored readings, row text, row order, or session-retention limits.
- Changing alert thresholds, NEWS2 logic, alarm limits, patient identity, or
  persistence behavior.
- Adding alarm acknowledgment, historical filtering, exports, pinned review
  state, or a new trend-review workflow.
- Replacing the latest tiles or active-alert list as the primary current-state
  surfaces.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The change improves review ergonomics by making the latest raw history row
  the default selected row after refresh.
- It does not alter the core monitoring workflow or any clinical calculation.

User population:

- Trained clinical users, testers, and reviewers using the local Win32 desktop
  application.

Operating environment:

- The existing single-patient Windows GUI using in-memory session history and
  either simulator-fed or manually entered readings.

Foreseeable misuse:

- Assuming the selected history row is a new alarm or acknowledgment target.
- Treating the selected history row as more authoritative than the latest tiles
  or current active-alert list.
- Expecting the UI to preserve an intentional older-row review during live
  refresh. That behavior is intentionally not part of this MVP.

## Current Behavior

- `create_dash_controls()` creates `IDC_LIST_HISTORY` as a standard single-
  selection Win32 listbox.
- `update_dashboard()` resets `IDC_LIST_HISTORY`, then appends one formatted row
  per stored reading in oldest-to-newest order.
- No explicit row-selection or scroll-to-latest behavior is applied after
  repopulation.
- `update_dashboard()` is reached from timer refresh, manual reading entry,
  admit/refresh, clear-session, scenario-load, and language-refresh paths.
- Empty or no-patient states leave the history list cleared.

## Proposed Change

1. Keep `IDC_LIST_HISTORY` as the existing chronological, read-only listbox.
   Do not reorder rows newest-first.
2. After `update_dashboard()` finishes repopulating the history list and only
   when `g_app.patient.reading_count > 0`, select the newest row
   (`reading_count - 1`) using listbox-selection APIs.
3. Ensure the selected newest row is visible after refresh by using the
   listbox's native scroll behavior or an explicit top-index adjustment when
   needed.
4. Do not call `SetFocus()` or otherwise move keyboard focus to
   `IDC_LIST_HISTORY`. The change is visual/default-selection behavior, not
   input-focus transfer.
5. When the history list is empty because no patient is admitted, the session
   was cleared, or a new patient session has not yet recorded a reading, leave
   the list with no selected row.
6. Apply the same newest-row rule consistently across all existing
   `update_dashboard()` entry paths, including:
   - timer-driven refresh
   - `Add Reading`
   - `Admit / Refresh`
   - scenario playback
   - language refresh
7. Accept the product tradeoff that live refresh will reselect the newest row
   each time. If the team later wants a "preserve manual older-row review"
   interaction, treat that as a separate issue because it requires additional
   UI state and acceptance criteria.
8. Keep the latest tiles and `IDC_LIST_ALERTS` as the primary current-state
   indicators. History-row selection must remain a convenience for raw review,
   not a new source of truth.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `dvt/DVT_Protocol.md`

Expected implementation files:

- `src/gui_main.c`

Optional supplemental verification files if the team automates selection-state
checks:

- `dvt/automation/run_dvt.py`

Files expected not to change:

- `src/patient.c`
- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- persistence, authentication, or patient-record data structures

## Requirements And Traceability Impact

- Existing requirements directly impacted:
  - `SYS-009` Vital Sign Reading History
  - `SYS-014` Graphical Vital Signs Dashboard
  - `SWR-PAT-002` because the stored history remains the source list content
  - `SWR-GUI-003` and `SWR-GUI-004` because `update_dashboard()` and dashboard
    refresh behavior are the affected GUI mechanisms
- No new UNS or SYS requirement is required for this MVP if the team treats the
  behavior as a software-level dashboard refinement.
- Add one new software requirement, adjacent to the existing GUI requirements,
  for example `SWR-GUI-014`:
  - after each dashboard refresh, `IDC_LIST_HISTORY` shall preserve
    chronological row order, select the newest available row, scroll that row
    into view, and leave the list unselected when no rows exist
  - the behavior shall not transfer keyboard focus away from the operator's
    active control
- Update `requirements/TRACEABILITY.md` so the new GUI requirement traces back
  to `SYS-009` and `SYS-014` and cites its GUI verification evidence.
- Prefer dedicated manual verification evidence such as a new `GUI-MAN-07`
  entry in `dvt/DVT_Protocol.md`. If the DVT harness can observe listbox
  selection safely, supplemental automation may extend `DVT-GUI-09`, but that
  is not required for the MVP.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- No change to acquisition, alert generation, NEWS2, alarm limits, patient
  identity, or data retention.
- Safety benefit: the newest raw reading becomes easier to find during routine
  review.
- Primary residual risk: a user intentionally reviewing an older row can be
  visually pulled back to the newest row on the next refresh. This is accepted
  in this MVP because the issue explicitly prioritizes landing on the current
  state first.
- Risk control: do not move keyboard focus, do not alter row content/order, and
  keep the latest tiles plus active-alert list visually primary.

Security:

- No new network, storage, authorization, or privilege behavior is introduced.

Privacy:

- No new disclosure or export path is introduced; the feature only changes
  selection state for already displayed local patient data.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: existing deterministic vital-sign rows only
- Output: deterministic listbox selection and scroll state only
- Human-in-the-loop limits: unchanged
- Transparency needs: ordinary GUI clarity only
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard GUI verification only
- PCCP impact: none

## Validation Plan

Minimum required validation is GUI-focused because the change is a Win32
presentation behavior.

Manual GUI validation:

- add a dedicated DVT/manual step that confirms the newest row is selected and
  visible after:
  - timer-driven refresh with accumulating readings
  - `Add Reading`
  - `Admit / Refresh` after a new patient admission
  - scenario load
- confirm `Clear Session` and no-patient states leave `IDC_LIST_HISTORY`
  without a stale selected row
- confirm the history list remains oldest-to-newest and that row text is
  unchanged apart from selection state
- confirm keyboard focus stays on the previously active input/control rather
  than being forced into the history list
- confirm active alerts and latest tiles still reflect only the latest reading

Optional supplemental automation:

- if the GUI DVT harness can safely inspect `LB_GETCURSEL`, extend the existing
  `DVT-GUI-09` check to assert that the last row is selected after refresh when
  at least one row exists

Recommended validation commands:

```powershell
run_tests.bat
python dvt/run_dvt.py
```

Expected validation outcome:

- Requirements and traceability updates stay limited to the new GUI behavior.
- Runtime changes stay confined to `src/gui_main.c`.
- Verification shows default newest-row selection without changes to alert
  semantics, stored history, or keyboard-focus ownership.

## Rollback Or Failure Handling

- If implementation needs to preserve manual review state across timer refresh,
  split that into a follow-on issue rather than broadening this MVP during
  implementation.
- If the listbox cannot reliably select and reveal the newest row without
  stealing focus or producing stale empty-list selection, stop and revisit the
  design before changing broader dashboard behavior.
- Rollback is straightforward because the feature is a local GUI refinement:
  remove the explicit newest-row selection/scroll logic and retain the existing
  history-list population flow.
