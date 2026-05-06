# Risk Note: Issue #93 - Show Trigger Time In The Session Alarm Review Log

Date: 2026-05-06
Issue: #93 "Feature: show trigger time in the session alarm review log"
Branch: `feature/93-show-trigger-time-in-the-session-alarm-review-log`

## Proposed change

Add a read-only trigger-time field to each stored session alarm event and render
that field as a dedicated column in the session alarm review log. The change is
intended to improve chronological review of alarm-state transitions that are
already retained for the current local session.

Repo observations supporting the change:

- `include/patient.h` currently defines `AlertEvent` with `reading_index`,
  `level`, `abnormal_mask`, and `summary`, but no explicit time field.
- `src/patient.c` currently appends event records from
  `patient_add_reading()` and formats them as `#<reading> [<severity>] <summary>`.
- `src/gui_main.c` currently renders the session alarm event list from the
  stored `AlertEvent` records and shows no chronology metadata beyond reading
  order.

## Product hypothesis and intended user benefit

Hypothesis: reviewers can assess transient deterioration more accurately when
each stored session alarm event shows when it occurred, instead of forcing the
reviewer to infer chronology only from row order or reading index.

Expected user benefit:

- faster reconstruction of warning-to-critical-to-recovery sequences
- clearer handoff and design-verification review of transient events
- better alignment between the event log and common alarm-history workflows seen
  in external monitoring products

## Source evidence quality

Source evidence quality is adequate for product-discovery scope and weak for any
clinical-effectiveness claim.

- The issue cites vendor product pages and one operator manual describing alarm
  history review workflows that include time-oriented alarm metadata.
- The cited sources are product/manual evidence that this workflow is common;
  they are not comparative usability studies, clinical outcome evidence, or
  validation for this implementation.
- The sources are sufficient to justify a narrow, non-copying chronology aid.
  They are not sufficient to justify claims about response-time measurement,
  alarm-management performance, or legal-record completeness.

## Medical-safety impact

This feature does not change thresholds, NEWS2 scoring, alarm limits, or active
alert behavior. Risk is introduced through chronology presentation, not through
new clinical logic.

Primary safety benefit:

- reviewers can see when a stored alarm transition occurred, which reduces the
  chance that a recent transient event is overlooked during review

Primary safety risks:

- an incorrect or non-monotonic time value could misrepresent event order
- a wall-clock-style timestamp could be mistaken for an authoritative charting
  record or an acknowledgement/response-time measurement
- stale time values that survive session reset could create a wrong-patient or
  wrong-session narrative
- a crowded or ambiguous row layout could blur the distinction between
  historical review data and current active alarms

Overall medical-safety impact: low to moderate if the feature remains read-only,
session-scoped, and explicitly secondary to the existing active-alert surface.

## Security and privacy impact

Security and privacy impact remains limited but non-zero because patient-linked
event rows gain extra chronology metadata.

- No new network, cloud, or background export path should be introduced.
- The time field must remain inside the current authenticated local session.
- Session-reset, logout, patient-clear, and patient-change paths must clear the
  stored time metadata together with the corresponding event rows.
- If a local wall-clock timestamp is used, the product must not imply that it is
  a synchronized enterprise audit timestamp.

## Affected requirements or "none"

Existing requirements likely affected:

- `UNS-017` Session Alarm Event Review
- `SYS-020` Session Alarm Event Review Storage
- `SYS-021` Session Alarm Event Review Presentation
- `SWR-PAT-007` Session Alarm Event Capture
- `SWR-PAT-008` Session Alarm Event Access and Reset
- `SWR-GUI-013` Session Alarm Event Review List

Derived requirement work is still needed to define:

- the authoritative trigger-time representation for this pilot
- monotonic ordering expectations when time and reading index both appear
- reset and rollover behavior for stored time metadata
- exact GUI label wording so users do not confuse historical event time with
  current alert state or clinician response timing

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinicians, testers, and reviewers inspect when session alarm events
  occurred during the current local monitoring session

User population:

- bedside clinical staff and internal reviewers using the Windows workstation
  pilot application

Operating environment:

- single-patient local desktop session using static in-process storage and
  either simulator-fed or manually entered readings

Foreseeable misuse:

- treating the displayed time as a legal charting timestamp
- inferring clinician acknowledgement or response time from the event log
- assuming the event log spans prior sessions or survives patient resets
- confusing trigger time with current live-monitoring status

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: misleading chronology can distort handoff or review of recent deterioration, but the feature does not change live detection or treatment logic. |
| Probability | Possible without controls because a new time field can drift from the reading that created it, become non-monotonic, or survive reset boundaries incorrectly. |
| Initial risk | Medium |
| Key risk controls | Capture the time value at the same point the event is appended; keep reading index as the stable ordering anchor; choose one clearly labeled time representation; clear time metadata on all session resets; preserve the active-alert list as the primary safety surface. |
| Verification method | Unit tests for time capture/reset behavior, integration checks proving event chronology remains aligned with stored reading order, and GUI/manual review showing the time column is clearly historical and does not imply acknowledgement or current alarm state. |
| Residual risk | Low |
| Residual-risk acceptability rationale | Acceptable for this pilot if the feature remains a bounded review aid, keeps deterministic semantics, and does not over-claim timestamp authority or response-tracking meaning. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Misleading chronology | Stored trigger time does not match the event-producing reading or appears out of order | Reviewer underestimates or misreads recent deterioration |
| False timestamp authority | UI presents local trigger time as if it were an audited charting record | Users over-trust the timestamp for handoff or retrospective claims |
| Wrong-session narrative | Event times remain after session reset, patient clear, or logout | Patient mix-up or false continuity of care story |
| Active-vs-historical confusion | Time column makes event rows look like live active alarms | User acts on stale information or misses the active-alert surface |
| Layout-driven omission | Column width truncates severity/summary or makes review rows unreadable | Important alarm context is missed during review |

## Existing controls

- The session alarm review feature is already bounded by `MAX_ALERT_EVENTS` and
  current-session storage only.
- Event capture already runs through `patient_add_reading()`, which gives a
  single append point for deterministic metadata capture.
- The GUI already presents active alerts separately from session alarm events.
- Existing session-reset notice behavior already discloses one retention
  boundary when earlier event history is cleared.

These controls reduce risk, but they do not yet define safe time semantics.

## Required design controls

- Decide explicitly whether the MVP uses session-elapsed time or local
  workstation trigger time. Do not leave the representation ambiguous.
- Prefer a deterministic, monotonic representation for the pilot. If absolute
  wall-clock time is used, add a controlled time source and label it clearly as
  local trigger time.
- Capture the time value exactly once when the event record is created, not when
  the GUI repaints.
- Retain `reading_index` in the row even after adding time so ordering remains
  auditable when times collide or are formatted coarsely.
- Keep the event list read-only and separate from acknowledgement, escalation,
  or response-time workflows.
- Clear stored time metadata whenever the corresponding session event history is
  cleared.
- Ensure the GUI column layout preserves severity and summary readability at the
  supported window sizes.

## Validation expectations

- unit tests proving time metadata is captured once per appended event and is
  cleared on `patient_init()` and other session-reset paths
- tests proving stable readings do not mutate prior event times or create new
  timed rows
- integration or deterministic seam tests proving displayed chronology matches
  reading order for warning, escalation, and recovery sequences
- manual GUI review in simulation mode confirming the label and column layout
  communicate "historical review" rather than current alarm state
- traceability updates for whichever time representation is chosen

## Residual risk for this pilot

Residual risk remains that users may over-interpret the displayed time as a
formal charting or response-time artifact. For the pilot, this is acceptable
only if the label, requirements, and UI wording keep the field bounded to
session alarm review.

## Product hypothesis validation

Validated. The issue describes a concrete review problem, the repo already has a
bounded session event surface, and external product-discovery evidence supports
chronology as a common alarm-review need.

## MVP boundary that avoids copying proprietary UX

The MVP should remain narrow:

- add one time field to the existing local session event record
- display that field in the current review list without redesigning the overall
  dashboard or copying central-station layouts
- do not add exports, acknowledgement states, operator notes, sorting systems,
  or analytics dashboards as part of this issue

## Clinical-safety boundary and claims that must not be made

This feature may support retrospective review. It must not claim to:

- provide an enterprise-grade audit trail
- prove clinician response time or alarm acknowledgement behavior
- replace live alerting or current active-alert review
- serve as a complete longitudinal medical record

## Whether the candidate is safe to send to Architect

Yes. This candidate is safe to send to Architect if the design keeps the change
read-only, session-scoped, deterministic, and explicit about what the displayed
time does and does not mean.

## Human owner decision needed, if any

Product owner and architect should explicitly decide:

- whether the MVP uses session-elapsed time or local wall-clock trigger time
- the exact UI label text for the new column
- whether coarse display formatting such as `HH:MM:SS` is acceptable, or whether
  elapsed-session formatting is safer for this pilot
