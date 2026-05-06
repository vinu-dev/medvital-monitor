# Risk Note: Issue #86 - Show Session Elapsed Time in the Patient Bar

Date: 2026-05-06
Issue: #86 "Feature: show session elapsed time in the patient bar"
Branch: `feature/86-show-session-elapsed-time-in-the-patient-bar`

## Proposed change

Add a read-only elapsed-session indicator to the patient bar so the operator can
see how long the current local monitoring session has been open without leaving
the dashboard.

The safe MVP is a clearly session-scoped label such as `SESSION 00:12:34`,
derived from the local app's session lifecycle. It should not imply a hospital
admission timestamp, chart-derived length of stay, or any persisted clinical
timeline that the current pilot does not have.

Repo observations supporting the change:

- `src/gui_main.c` `paint_patient_bar()` currently shows patient name, ID, age,
  BMI, and reading count only; no elapsed-time field exists today.
- Session-reset paths already exist and are safety-relevant:
  `do_admit()` calls `patient_init()`, `do_clear()` zeroes patient state,
  logout zeroes patient state, simulation-disable clears state, and the
  simulation timer reinitializes the patient when `MAX_READINGS` is reached.
- `PatientRecord` stores demographics, readings, and alert-event review data,
  but no session-start timestamp or elapsed-time field exists in the current
  domain model.
- `Pause Sim` stops new readings, but it does not end the patient session.
  That means reading count or simulator cadence is not a safe proxy for true
  elapsed time.

## Product hypothesis and intended user benefit

Hypothesis: operators benefit from a quick answer to "how long has this local
session been open?" during review and handoff, especially when the patient bar
is already the main contextual summary surface.

Expected user benefit:

- faster orientation without opening history views
- clearer distinction between a newly admitted/refreshed patient and a session
  that has been running for a while
- lower risk of inferring freshness indirectly from reading count or latest
  sample timing

## Source evidence quality

Evidence quality is adequate for a narrow product-discovery decision and weak
for any broader clinical or workflow claim.

- Philips Horizon Trends product page states that its trend display provides
  visual information about how patient measurements change over time at a
  glance. This is good evidence that time-oriented patient context on monitor
  surfaces is a standard workflow, but it is product marketing and does not
  justify a specific elapsed-session widget or proprietary layout.
  Source: https://www.usa.philips.com/healthcare/product/HCNOCTN172/horizontrendstrendmeasurementdisplay
- Philips Telemetry Monitor 5500 IFU lists local measurement trend/alarm
  history and 24-hour numeric trend capabilities. This supports the general
  relevance of time-based monitor context, but not the exact semantics or UX of
  a local session-age label.
  Source: https://www.documents.philips.com/assets/Instruction%20for%20Use/20251028/723ae1fdf34a4b219d36b384012166a8.pdf?feed=ifu_docs_feed

Conclusion: the sources are sufficient to justify a narrow, non-copying
session-context aid. They are not sufficient to justify copying vendor trend
visuals or claiming improved clinical outcomes from an elapsed-session label.

## Medical-safety impact

This is a UI-context feature only. It does not change thresholds, NEWS2 logic,
alarm limits, alert generation, or treatment guidance.

Primary safety benefit:

- gives the operator explicit session context instead of forcing them to infer
  it from reading cadence or memory

Primary safety risks:

- stale elapsed time could remain visible after patient clear, logout, patient
  change, or automatic session rollover
- inaccurate elapsed time could be shown if implementation derives duration
  from reading count rather than an actual session time source
- wording such as "admission age" could be mistaken for true hospital-admission
  duration rather than local monitoring-session age
- users could over-trust the timer as evidence of review freshness or clinical
  continuity across session resets

Overall medical-safety impact: low direct clinical impact, but low-to-moderate
presentation risk if the time label is semantically ambiguous or stale.

## Security and privacy impact

Security/privacy impact is limited.

- No new network, cloud, or export path is needed.
- The label remains inside the existing authenticated local session boundary.
- The timer should not persist or display outside the active patient session.
- The feature exposes only session duration metadata associated with the already
  visible patient context; it does not introduce new categories of PHI.

## Affected requirements or "none"

No approved requirement explicitly covers elapsed session age today. If the
feature proceeds, traceability updates are needed.

Likely affected existing requirements:

- `UNS-010` Consolidated Status Summary
- `UNS-014` Graphical User Interface
- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-002` logout/session-clear behavior
- `SWR-GUI-003` dashboard presentation behavior
- `SWR-GUI-004` admit/refresh behavior
- `SWR-GUI-010` simulation/device-mode patient-bar behavior

New derived requirements are likely needed for:

- elapsed-session semantics and label wording
- exact start/reset boundaries
- paused-simulation behavior
- device-mode/no-patient display behavior
- verification of stale-value prevention

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained operators use the patient bar to see how long the current local
  monitoring session has been open

User population:

- bedside clinical staff, ward reviewers, and internal testers using the local
  Windows workstation application

Operating environment:

- current single-patient Win32 desktop workflow with in-process state, local
  simulation/manual entry, and no authoritative hospital admission feed

Foreseeable misuse:

- treating the timer as true hospital admission duration
- assuming the timer proves a patient has been continuously monitored without
  reset or rollover
- assuming the timer indicates time since last clinician review or time since
  last physiologic change
- using the timer for charting, audit, or legal-record purposes

## MVP boundary that avoids copying proprietary UX

The MVP should remain narrow:

- simple text in the existing patient bar only
- local session age only
- no vendor-style trend graphics, baselines, deviation bars, or historical
  review widgets
- no persistence, no export, no multi-patient timeline, no enterprise
  admission tracking, and no remote synchronization

## Clinical-safety boundary and claims that must not be made

The feature may support operator orientation. It must not claim to:

- show true hospital admission duration unless a future authoritative admission
  timestamp source exists
- prove continuous monitoring across clear, logout, simulation disable, or
  bounded-session rollover
- indicate time since last clinical assessment, alarm acknowledgment, or
  treatment action
- provide diagnostic, triage, or treatment recommendations

## Whether the candidate is safe to send to Architect

Yes, with constraints.

The candidate is safe to send to Architect because it is a read-only context
aid and does not alter clinical algorithms. The design must define one explicit
time source and one explicit session boundary, and it must avoid implying that
the app knows true admission age when it only knows local session age.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a misleading or stale timer could contribute to wrong-patient or wrong-context assumptions during handoff or review, even though it does not directly control alarms or treatment logic. |
| Probability | Possible without controls because the current app has several reset paths and no existing session-time source. |
| Initial risk | Medium |
| Key risk controls | Use an explicit monotonic time source; label the field as `Session`; reset it on every session boundary; prevent stale display in no-patient/device-mode states; avoid deriving time from reading count. |
| Verification method | GUI smoke checks across admit, refresh, clear, logout, sim-disable, pause/resume, and automatic rollover; targeted automated tests if a time-source abstraction is introduced; traceability updates for the new behavior. |
| Residual risk | Low if controls are implemented and verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the label remains a read-only orientation aid, existing live vitals/alerts remain primary, and the major failure modes are controllable through explicit semantics and reset behavior. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Wrong-patient time context | Timer survives patient clear, admit refresh, logout, or simulation rollover | User misattributes elapsed time to the wrong patient session |
| False sense of elapsed duration accuracy | Timer is derived from sample count or paused simulation state instead of a real session clock | User underestimates or overestimates how long the session has been open |
| Admission/session confusion | UI wording implies chart-level admission age | User assumes continuity or legal significance that the app cannot support |
| Hidden reset boundary | Timer silently jumps or restarts without disclosure where the app already resets bounded session data | User assumes continuity across a local retention boundary |

## Existing controls

- The app already has explicit reset paths for admit/refresh, clear, logout,
  simulation disable, and automatic bounded-session rollover.
- The patient bar is already the established contextual summary surface.
- The current session is authenticated and local-only.
- The app already treats patient/session state as bounded and non-persistent.
- Existing alert-review work introduced explicit reset-boundary disclosure for
  bounded local session data, which is a useful precedent for timer semantics.

These controls reduce scope, but they do not by themselves define safe
elapsed-time behavior.

## Required design controls

- Define the field explicitly as local session age, not hospital admission age.
- Use one deterministic time source anchored to the chosen session-start event;
  do not infer elapsed time from `reading_count` or simulator tick count.
- Apply the same reset semantics across all patient/session reset paths:
  admit/refresh, clear, logout, simulation disable, no-patient state, and
  automatic `MAX_READINGS` rollover.
- If the label means elapsed wall-clock session time, it should continue across
  `Pause Sim`; if the team wants it to freeze, the label semantics must change.
- Display `N/A`, hide the field, or otherwise suppress stale values whenever no
  active patient session exists.
- Use neutral presentation. Do not style the timer as an alarm, trend verdict,
  or treatment cue.
- Keep the feature local-only with no persistence, export, or charting claim.

## Validation expectations

- GUI smoke check:
  admit patient and confirm the timer starts from the defined boundary
- GUI smoke check:
  pause and resume simulation and confirm behavior matches documented semantics
- GUI smoke check:
  clear session, logout, admit a new patient, disable simulation, and trigger
  automatic rollover to confirm the old timer value never survives
- Verification that device mode and no-patient states never display stale time
- Traceability updates for any new GUI/session-timing requirement
- If implementation adds a reusable time-source abstraction, automated tests
  should cover start, reset, and formatting behavior deterministically

## Residual risk for this pilot

Residual risk remains that a user may over-interpret the timer as broader
admission history. For the pilot, that is acceptable only if the wording stays
session-scoped and all reset boundaries are explicit and consistent.

## Human owner decision needed, if any

Product owner / architect should explicitly decide:

- whether the field means "time since session/admit started" or "time since
  first successful reading"
- whether the UI label must say `Session` rather than `Admission`
- how automatic bounded-session rollover should be disclosed if the timer
  resets to zero during long simulation runs
