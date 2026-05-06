# Design Spec: Issue 89

Issue: `#89`
Branch: `feature/89-filter-session-alarm-event-log-by-severity-and-time-range`
Spec path: `docs/history/specs/89-filter-session-alarm-event-log-by-severity-and-time-range.md`

## Problem

The current dashboard exposes the retained session alarm-event history only as a
single unfiltered list:

- `src/gui_main.c` repopulates `IDC_LIST_EVENTS` from every stored
  `AlertEvent`.
- `src/patient.c` and `include/patient.h` store each event with a
  `reading_index`, aggregate `AlertLevel`, abnormal-parameter signature, and
  summary text.

That is enough to preserve historical alarm-state changes, but not enough to
help a reviewer quickly isolate the subset they care about. In a noisy session,
the user must scan the entire retained list to find only critical events, only
recovery events, or only the most recent portion of the session.

The issue title requests filtering by severity and time range, but the current
event model does not store any trustworthy timestamp or elapsed-session field.
A naive "time range" control would therefore imply precision the product does
not actually have.

## Goal

Add a narrow, read-only filter layer for the session alarm-event review list
that:

- defaults to the complete retained session history
- lets the reviewer narrow visible events by stored event severity
- lets the reviewer narrow visible events by a bounded recent-session window
  without claiming wall-clock precision
- keeps active alerts, overall patient status, and event capture behavior
  unchanged
- clears filter state whenever the patient session boundary resets

## Non-goals

- Changing alert thresholds, NEWS2 logic, alarm limits, event-capture rules, or
  treatment guidance.
- Adding wall-clock timestamps, elapsed-time claims, or duration analytics in
  this issue.
- Filtering `IDC_LIST_ALERTS`, the overall status banner, or the reading
  history list.
- Persisting filter preferences across logout, patient change, or application
  restart.
- Adding export, acknowledgement, escalation workflow, multi-patient review, or
  network/cloud behavior.
- Changing `patient_print_summary()` to print a partial filtered view.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The feature remains a review-only adjunct for already retained session alarm
  events.
- It does not change live monitoring behavior or alarm generation.

User population:

- Trained clinical staff, testers, and reviewers using the local Win32 desktop
  application.

Operating environment:

- The existing single-patient workstation workflow, using bounded in-process
  session storage with simulator-fed or manually entered readings.

Foreseeable misuse:

- Assuming a filtered view is the complete retained session history.
- Assuming filtered-out critical events did not occur.
- Assuming a recent-session window represents true elapsed minutes or hours.
- Expecting the filter to change current active alerts or any clinical logic.

## Current Behavior

- `PatientRecord` retains up to `MAX_ALERT_EVENTS` session alarm events and up
  to `MAX_READINGS` readings using static storage only.
- Each `AlertEvent` stores:
  - `reading_index`
  - `level`
  - `abnormal_mask`
  - `summary`
- `update_dashboard()` always renders the full event list in reading order.
- `patient_print_summary()` always renders the full retained session event list
  and any session-reset notice.
- Automatic session reset already discloses that earlier session events are no
  longer retained.
- No existing event field expresses wall-clock time, elapsed seconds, or a
  trustworthy manual-entry timestamp.

## Proposed Change

1. Interpret this MVP's "time range" requirement as a bounded recent-session
   window defined over reading order, not as wall-clock time.
2. Use user-facing control text such as `Recent session window` or
   `Recent readings`, not `Time range`, `Last 5 minutes`, or any wording that
   implies verified chronology.
3. Add a compact filter row associated with `IDC_LIST_EVENTS`, not with
   `IDC_LIST_ALERTS` or the status banner.
4. The severity filter shall operate only on stored `AlertEvent.level` values.
   Recovery-to-normal entries (`ALERT_NORMAL`) must remain representable and
   must not be silently merged into warning or critical filters.
5. The most implementation-friendly severity control is a small set of explicit
   visibility toggles for:
   - recovery / normal events
   - warning events
   - critical events
   All three shall default to enabled.
6. The recent-session window shall be based on `AlertEvent.reading_index`
   relative to the current `PatientRecord.reading_count`. An event matches a
   selected window when its `reading_index` falls within the last `N`
   successfully recorded readings of the current session.
7. Keep the recent-window choices small and fixed. For this repo's bounded
   session size, the recommended initial set is:
   - `All retained` (default)
   - `Last 3 readings`
   - `Last 5 readings`
   Final UI copy may vary, but the semantics must remain reading-window based.
8. The filter shall be a pure view concern. It must not mutate stored
   `AlertEvent` entries, rebuild history, or alter `patient_add_reading()`
   semantics.
9. `IDC_LIST_EVENTS` shall show a distinct placeholder when the current filter
   yields no matches, for example:
   `No session alarm events match current filters.`
   This must remain distinct from the existing no-data placeholder for a
   session that has never recorded any events.
10. The existing session-reset disclosure shall remain visible whenever present,
    even if the current filter matches no events. The reset notice is retention
    context, not a filterable event row.
11. `IDC_LIST_ALERTS`, aggregate status rendering, and `patient_print_summary()`
    shall continue to show the full current-state or full retained-session view
    and shall remain independent from any GUI filter state.
12. Filter state shall live in GUI/session state only. It shall reset to the
    full-view default whenever the patient session is reinitialized, including:
    - logout/dashboard recreation
    - manual clear session
    - admit/refresh for a patient
    - automatic session rollover that triggers `patient_init()`
13. To keep filter semantics testable without Win32 message-loop complexity,
    implement the matching logic as a deterministic helper with no UI
    dependencies. A small helper module such as
    `src/session_event_filter.c` / `include/session_event_filter.h` is
    preferable to burying all predicate logic directly in paint/update code.
14. If product leadership later requires true elapsed-time or wall-clock range
    filtering, split that into a follow-on issue that adds a verified session
    time source, requirement updates, and explicit reset semantics. Do not
    overclaim time precision in this issue.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `src/gui_main.c`
- `src/localization.c`
- `include/localization.h`
- `src/session_event_filter.c`
- `include/session_event_filter.h`
- `CMakeLists.txt`

Expected verification files:

- `tests/unit/test_session_event_filter.cpp`
- `dvt/DVT_Protocol.md`

Files expected not to change:

- `src/patient.c`
- `include/patient.h`
- `src/alerts.c`
- `src/vitals.c`
- `src/news2.c`
- `src/alarm_limits.c`

## Requirements And Traceability Impact

- No new UNS entry is expected. This feature remains within `UNS-017` session
  alarm event review.
- The baseline has no explicit filter requirement yet. Add one new SYS-level
  requirement, or expand `SYS-021`, to cover:
  - default full-session visibility
  - severity filtering over stored event severity
  - recent-session-window filtering over reading order
  - no effect on active alerts or alert capture
  - reset-to-default behavior on session boundaries
- Add one new GUI-focused SWR, or a small pair of SWRs, to cover:
  - filter controls and default state
  - severity and recent-window matching semantics
  - distinct empty-result placeholder behavior
  - independence from `IDC_LIST_ALERTS` and full-session summary evidence
- The RTM should link the new SWR(s) to:
  - new unit tests for pure filter matching logic
  - manual or DVT verification of GUI labeling, reset behavior, and active
    alert independence
- Existing related requirements that constrain this design:
  - `SYS-020`
  - `SYS-021`
  - `SWR-PAT-007`
  - `SWR-PAT-008`
  - `SWR-GUI-013`

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- The feature is presentation-only, but filtering can hide clinically relevant
  historical events if defaults or labels are unsafe.
- The design remains acceptable only if:
  - full retained session visibility is the default
  - active alerts stay separate and unfiltered
  - a partial filter state is obvious and easy to clear
  - recent-window wording avoids false time precision

Security:

- No new authentication, authorization, or network surface is introduced.
- Filter state must not leak across user/session boundaries.

Privacy:

- No new patient-data class or persistence path is introduced.
- Session-local review data already shown in the dashboard remains local only.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: existing deterministic `AlertEvent` records only
- Output: filtered review rows in the GUI only
- Human-in-the-loop limits: unchanged from the current system
- Transparency needs: the UI must disclose when the view is partial, but this
  is a deterministic presentation concern rather than AI explainability
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and DVT only
- PCCP impact: none

## Validation Plan

Automated validation should target the filter predicate and boundary rules,
while manual or DVT review should validate GUI labeling and reset behavior.

Automated validation scope:

- default filter state matches all retained events
- severity toggles include and exclude recovery, warning, and critical rows
  exactly as selected
- recent-session window boundaries are correct for:
  - first included event
  - last included event
  - empty-result case
  - window sizes larger than the current reading count
- applying or clearing filters does not change stored event capture behavior
- existing patient event tests remain green, confirming no regression in event
  generation or ordering

Manual / DVT validation scope:

- event list starts in full-session view after patient admit and after logout
- active filters are visually obvious and can be cleared in one step
- the active-alert list remains unchanged while event filters are adjusted
- automatic session reset keeps its disclosure visible and restores default
  filter state
- the empty-result placeholder differs from the no-events-recorded placeholder

Recommended validation commands:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "Patient|Alert|SessionEventFilter"
python dvt/run_dvt.py
```

## Rollback Or Failure Handling

- If stakeholders require true elapsed-time or wall-clock filtering, stop and
  split that into a follow-on issue rather than shipping misleading labels on
  ordinal data.
- If the GUI cannot make partial filter state obvious, do not ship a hidden
  partial-view control; revert to the full-list behavior and keep the issue
  blocked pending clearer UX.
- If implementation cannot provide automated coverage for the filter predicate,
  move the logic into a smaller testable helper rather than accepting an
  untested boundary-sensitive GUI-only implementation.
- Rollback is straightforward because the intended runtime change is additive:
  remove the GUI filter controls and helper logic and restore the existing
  full-session event list.
