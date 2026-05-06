# Risk Note: Issue #81 - Focus newest reading in the dashboard history list

Date: 2026-05-06
Issue: #81 "Feature: focus newest reading in the dashboard history list"
Branch: `feature/81-focus-newest-reading-in-the-dashboard-history-list`

## Proposed change

After each dashboard refresh, automatically select and scroll to the newest row
in the `IDC_LIST_HISTORY` list so the operator lands on the current session
state first.

Repo observations supporting the change:

- `src/gui_main.c` `update_dashboard()` clears `IDC_LIST_HISTORY` and repopulates
  it in chronological order from `g_app.patient.readings[0..reading_count-1]`.
- The same function separately derives current active alerts from
  `patient_latest_reading()` plus `generate_alerts()`, so the proposed change is
  a presentation refinement to the raw history list rather than a clinical-logic
  change.
- `update_dashboard()` is called from timer refresh, manual add-reading,
  patient admit/refresh, clear-session, scenario load, and language-refresh
  paths, so the selection behavior must be safe across both live updates and
  session resets.

## Product hypothesis and intended user benefit

Hypothesis: defaulting the history list to the newest row reduces scanning and
scrolling after refresh, helping operators review the current session state
faster without changing what data is stored or displayed elsewhere.

Expected user benefit:

- the most recent reading is immediately visible in the raw history list
- the operator does not need to manually scroll after timer or manual refreshes
- the current-state review path aligns better with the existing latest-reading
  tiles and active-alert list

## Source evidence quality

Evidence quality is adequate for a narrow product-discovery UI improvement and
insufficient for any clinical-effectiveness claim.

- Philips Patient Information Center iX Quick Guide shows review workflows with
  tabular trend views and explicit time focus behavior tied to recent review
  context. This is useful evidence that time-focused review is a standard
  monitoring workflow, but it is a vendor quick guide rather than independent
  clinical evidence.
- GE HealthCare B1x5M product material describes single-screen workflow,
  multiple layout options, customized alarm management, and trend review with
  72 hours of data. This supports general workflow relevance only and is also
  vendor marketing material.

Conclusion: the sources are sufficient to justify a narrow, non-copying MVP
that improves list focus behavior. They do not justify claims of improved
clinical outcomes, alarm performance, or decision support.

## Medical-safety impact

This issue does not change acquisition, thresholds, NEWS2 logic, alarm limits,
patient identity handling, or data persistence. The safety impact is limited to
how historical information is presented.

Primary safety benefit:

- reduces the chance that the newest reading is visually buried in the raw
  history list during routine review

Primary safety risks:

- automatic reselection could disrupt an operator who is intentionally reviewing
  an older row while live refresh continues
- a selected historical row could be mistaken for the authoritative current
  state if the UI does not keep latest-reading tiles and active alerts clearly
  primary
- stale selection after clear/reset could imply that a prior session row still
  belongs to the current patient

Overall medical-safety impact: low, provided the change remains read-only and
does not alter list contents, ordering, or the current-alert source of truth.

## Security and privacy impact

No new network, storage, export, authentication, authorization, or patient-data
sharing path is introduced.

- Security impact is negligible if the change stays inside the existing local
  authenticated GUI session.
- Privacy impact is negligible because the feature only changes which existing
  local row is selected and scrolled into view.

## Affected requirements or "none"

None in the currently approved baseline. This is a presentation-detail change
that is adjacent to existing GUI behavior but is not yet stated as a normative
requirement.

If the team wants the behavior to be mandatory and testable, add a narrowly
scoped GUI requirement adjacent to:

- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-004` Patient Data Entry via GUI
- `SWR-GUI-013` Session Alarm Event Review List

The requirement should state that refresh repopulates the history list without
changing data order and selects the newest available row when one exists.

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinical users and testers review the current session's latest raw
  reading more quickly after refresh

User population:

- bedside clinicians, reviewers, and internal testers using the Windows desktop
  pilot

Operating environment:

- local Win32 GUI session with bounded in-memory reading history and either
  simulated or future device-fed readings

Foreseeable misuse:

- assuming the selected row is a new alarm or a required acknowledgment target
- assuming the selected history row replaces the latest-reading tiles or active
  alerts as the primary current-state signal
- losing place while intentionally reviewing older rows if refresh always
  reselects the newest row

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Minor to moderate: a misleading or disruptive selection state could slow review or briefly confuse which reading is current, but it does not modify clinical calculations or suppress the existing current-state indicators. |
| Probability | Possible without controls because refresh occurs from several paths, including timer-driven updates and session reset flows. |
| Initial risk | Low |
| Key risk controls | Keep the change read-only; preserve chronological list order; select only the newest existing row; do not leave stale selection after reset; do not move keyboard focus away from other active controls; keep latest tiles and active alerts visually primary. |
| Verification method | Manual GUI verification across timer refresh, manual add-reading, admit/refresh, scenario load, and clear-session flows; targeted tests or harness checks if feasible for list population/order; review against requirements scope to ensure no new clinical claims are introduced. |
| Residual risk | Low |
| Residual-risk acceptability rationale | Acceptable for this pilot because the feature is a bounded ergonomics improvement layered on top of unchanged current-state alerts and latest-reading tiles, with no new clinical interpretation path. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Current reading remains hard to find | Newest row is not selected or not scrolled into view after refresh | Slower recognition of the latest session state |
| User loses review context | Timer refresh forcibly reselects newest row while the operator is reviewing an older row | Review interruption, possible confusion during handoff or test review |
| Historical row is mistaken for the current decision surface | Visual emphasis on the selected history row is stronger than the latest tiles or active alerts | User over-weights historical detail instead of the current-state indicators |
| Stale session data appears current | Selection survives clear-session, patient change, or empty-list states incorrectly | Patient/session confusion |
| Data-integrity expectations are violated | Implementation changes list ordering, row text, or stored readings instead of selection only | Misleading history review and avoidable verification churn |

## Existing controls

- Latest patient status, alert banner, and active-alert list already derive from
  `patient_latest_reading()` and `generate_alerts()`, so current-state safety
  signals do not depend on history-row selection.
- `update_dashboard()` already rebuilds the history list from bounded in-memory
  readings, which limits the change to deterministic presentation logic.
- Session clear and patient reinitialization paths already define when the
  history list should become empty or restart from a new session boundary.

## Required design controls

- Limit implementation to selection and scroll behavior for
  `IDC_LIST_HISTORY`; do not change stored readings, row text, or row order.
- Apply newest-row selection only when at least one history row exists.
- Clear selection cleanly when the history list becomes empty after
  clear-session or patient reset paths.
- Keep keyboard navigation stable by avoiding forced control focus changes when
  another control is active.
- Preserve the latest-reading tiles and active-alert list as the primary
  current-state surfaces.
- If the team wants refresh to stop overriding an intentional user review of an
  older row, resolve that interaction rule explicitly during design rather than
  letting it emerge implicitly from implementation details.

## Validation expectations

- Manual GUI check that timer-driven refresh selects and scrolls to the newest
  row after new readings arrive.
- Manual GUI check that `Add Reading`, `Admit / Refresh`, and scenario-load
  paths land on the newest row without reordering history.
- Manual GUI check that `Clear Session` leaves no stale selection or prior-row
  residue.
- Manual GUI check that keyboard focus remains usable and that auto-selection
  does not move focus away from active input controls.
- Code review or targeted harness validation that list population remains
  chronological and that the change does not alter active-alert behavior.

## MVP boundary that avoids copying proprietary UX

Keep the MVP narrow and repo-native:

- select and scroll to the newest row in the existing listbox only
- reuse the existing row text and chronological ordering
- no new trend view, no filter/search, no split panes, no waveform review, no
  alarm-review workflow, no persistence of list position, and no competitor-like
  time-navigation UI

## Clinical-safety boundary and claims that must not be made

The feature may improve review ergonomics. It must not claim to:

- prioritize clinical action automatically
- replace active alerts or latest-reading tiles
- acknowledge alarms, confirm review completion, or prove clinician awareness
- improve diagnostic accuracy or treatment outcomes

## Whether the candidate is safe to send to Architect

Yes. The candidate is safe to send to Architect because it is read-only,
presentation-only, and sufficiently bounded. The design should explicitly
resolve whether timer refresh always reselects the newest row or yields to an
intentional user review of older history, but that is a manageable product/UI
decision rather than a blocker to architecture work.

## Residual risk for this pilot

Residual risk is limited to user-interface confusion or annoyance if auto-focus
is too aggressive during live refresh. For this pilot that is acceptable if the
implementation preserves current-state indicators as primary and handles reset,
empty-list, and keyboard-interaction states cleanly.

## Human owner decision needed, if any

Product owner / architect should confirm one interaction rule before
implementation finalizes:

- whether live refresh must always reselect the newest row, or whether an
  explicit user review of an older row should temporarily suppress auto-reset
  until the next manual refresh or session change
