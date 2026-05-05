# Risk Note: Issue #37 - Session Alarm Event Review Log

Date: 2026-05-05
Issue: #37 "Feature: add session alarm event review log"
Branch: `feature/37-session-alarm-event-review-log`

## Proposed change

Add a bounded, read-only session alarm event review log that records clinically
meaningful alert-state transitions that already occurred during the current
session. The proposed MVP is constrained to static storage, existing patient
session scope, and local display/console evidence only.

Repo observations supporting the change:

- `src/gui_main.c` resets `IDC_LIST_HISTORY` and repopulates it with raw reading
  rows for each stored `VitalSigns` sample, but `IDC_LIST_ALERTS` is populated
  only from `patient_latest_reading()` plus `generate_alerts()`.
- `src/patient.c` `patient_print_summary()` prints latest vitals and active
  alerts only; it does not preserve cleared warning or critical episodes.
- The current session is already bounded by static storage. `MAX_READINGS` is 10
  and the simulated timer path reinitializes the patient record when the buffer
  becomes full.

## Product hypothesis and intended user benefit

Hypothesis: a short retrospective alarm-event view improves clinical handoff,
tester review, and DVT evidence because transient warning/critical episodes
remain visible after the latest reading has returned to normal.

Expected user benefit:

- faster recognition that a patient recently deteriorated or recovered
- less manual reconstruction from raw reading rows
- better alignment between GUI review and console/DVT evidence

## Source evidence quality

Evidence quality is adequate for product-discovery scope and poor for clinical
effectiveness claims.

- Philips IntelliVue Information Center brochure describes retrospective review
  workflows including Event Review, Alarm Review, and Trend Review. This is
  strong evidence that retrospective event review is a standard monitoring
  workflow, but it is vendor marketing material and the brochure itself is old.
  Source: https://www.documents.philips.com/doclib/enc/fetch/2000/4504/577242/577243/577247/582646/583147/IntelliVue_Information_Center_Brochure.pdf
- Philips Clinical Surveillance marketing states that browser-based views expose
  waveforms, trends, events, history, device settings, and alert prioritization.
  This supports workflow relevance, not safety or efficacy.
  Source: https://www.usa.philips.com/healthcare/acute-care-informatics/clinical-surveillance
- GE HealthCare CARESCAPE Central Station marketing states that users can review
  up to 72 hours of trended data and other historic monitor data. This supports
  the general product hypothesis, but it reflects central-station workflows that
  are much broader than this single-patient pilot.
  Source: https://www.gehealthcare.com/en-ph/products/patient-monitoring/patient-monitors/carescape-central-station

Conclusion: the sources are sufficient to justify a narrow, non-copying MVP for
retrospective review. They are not sufficient to justify diagnostic claims,
alarm-management claims, or enterprise retention scope.

## Medical-safety impact

This change does not alter thresholds, NEWS2 logic, alarm limits, or treatment
guidance. Risk arises from presentation, not from new clinical calculations.

Primary safety benefit:

- makes recent transient instability visible after the patient returns to a
  normal latest-reading state

Primary safety risks:

- omitted, misordered, or stale event entries could understate recent
  deterioration during handoff or review
- ambiguous event wording could cause the user to confuse "historical review"
  with "current active alarm"
- silent reset or silent truncation could make the log appear complete when it
  is not

Overall medical-safety impact: low-to-moderate if designed as a read-only
adjunct to the existing live alert view, but not acceptable if the log becomes a
second unvalidated interpretation path.

## Security and privacy impact

Security/privacy impact is limited but real because the event log will expose
the same patient-linked observations already shown in the dashboard.

- No new network path or cloud sync should be introduced.
- No persistent longitudinal storage should be introduced in this MVP.
- The log must remain behind the existing authenticated session boundary.
- Event content must clear on logout, patient clear, patient change, and any
  session rollover behavior that reinitializes the patient record.
- If `patient_print_summary()` is extended, console output remains local-only
  evidence and must not be framed as a durable medical record export.

## Affected requirements or "none"

Existing requirements likely affected:

- `UNS-009` Vital Sign History
- `UNS-010` Consolidated Status Summary
- `SYS-009` Vital Sign Reading History
- `SYS-011` Patient Status Summary Display
- `SYS-012` Static Memory Allocation
- `SWR-ALT-001` through `SWR-ALT-004` because the event log should reuse the
  same validated alert semantics
- `SWR-PAT-002`, `SWR-PAT-003`, and `SWR-PAT-006`
- `SWR-GUI-003`, `SWR-GUI-004`, and likely a new GUI requirement for
  retrospective event review

New derived requirements are needed for:

- event-capture trigger rules
- event ordering and timestamp/sequence semantics
- bounded retention and visible truncation behavior
- clear-on-reset/logout/new-patient behavior
- parity between GUI event review and console summary evidence

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinicians and testers review what alarm-state changes occurred during
  the current local monitoring session

User population:

- bedside clinical staff, ward reviewers, and internal testers using the
  Windows workstation application

Operating environment:

- local Windows desktop application in the current pilot scope, using static
  in-process storage and either simulation or future device-fed readings

Foreseeable misuse:

- assuming the log is a complete longitudinal record
- assuming a cleared historical event is still an active alarm
- assuming missing rows mean stability rather than buffer rollover or reset
- using the log as an acknowledgment/escalation workflow, which is out of scope

## MVP boundary that avoids copying proprietary UX

The MVP should stay narrow and implementation-led:

- fixed-size local event buffer only
- one patient, one session, no cross-patient surveillance
- simple list entries derived from the app's own alert semantics
- reading index or elapsed-session marker, not competitor-specific waveform or
  central-station review layouts
- no alarm strips, no enterprise retention, no remote access, no telemetry
  pairing, no HL7/EMR behavior, no acknowledgment workflow

## Clinical-safety boundary and claims that must not be made

The feature may support retrospective review. It must not claim to:

- diagnose, predict, or recommend treatment
- replace the active alert list or live monitoring view
- provide a complete legal medical record
- confirm that no significant event occurred outside the retained session window
- prove alarm acknowledgment, escalation, or clinician response timing

## Whether the candidate is safe to send to Architect

Yes, with constraints. The candidate is safe to send to Architect because it is
read-only, session-scoped, and does not change clinical algorithms. Design must
explicitly preserve the existing alert engine as the single source of truth and
must disclose reset/truncation boundaries clearly.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a misleading retrospective view could delay recognition of recent instability during handoff or review, but the feature does not itself trigger or suppress treatment decisions. |
| Probability | Possible without controls because a new display path can drift from live alert semantics, truncate silently, or survive session reset incorrectly. |
| Initial risk | Medium |
| Key risk controls | Derive events only from existing validated alert logic; mark the view as session-only and read-only; clear on reset/logout/new patient; show truncation/reset indicators; preserve ordering; keep console/GUI evidence aligned. |
| Verification method | Unit tests for capture/reset/truncation rules, integration checks against current alert semantics, GUI/DVT evidence showing transient critical events remain reviewable after recovery, and traceability updates for new requirements. |
| Residual risk | Low if controls are implemented and verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the feature remains an adjunct review aid, not a control path or diagnostic engine, and because live current alerts remain available as the primary safety signal. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Recent deterioration is hidden | Critical or warning transition is never logged, or is overwritten without disclosure | Reviewer underestimates recent instability and delays escalation or follow-up |
| Historical entry is mistaken for current alarm | Event wording or visual styling fails to distinguish historical vs active state | User responds to stale information or ignores the active alert surface |
| Wrong-patient event narrative | Events survive patient clear, logout, auto-reset, or patient change | Patient mix-up and incorrect handoff summary |
| Log appears complete when it is not | Buffer truncation or session rollover drops old events silently | False confidence in review completeness |
| Review evidence conflicts across surfaces | GUI event log, active alerts list, and `patient_print_summary()` are derived differently | Confusion during testing, DVT, or clinical review |

## Existing controls

- Existing authentication and session-entry controls already limit access to
  patient data.
- Existing alert classification logic is centralized in `check_*()`,
  `overall_alert_level()`, and `generate_alerts()`.
- Existing patient data is already bounded by fixed-size static storage.
- Existing session clear and patient initialization paths already define
  discrete local-session boundaries.
- Existing active-alert and raw-history views reduce the chance that the new log
  becomes the only patient review surface.

These controls reduce implementation risk, but they do not by themselves define
safe event-review semantics.

## Required design controls

- Use the existing alert engine as the only source of event semantics. Do not
  create a second set of threshold rules.
- Define exactly when an event is appended:
  severity change, abnormal-parameter-set change, or both.
- Include an explicit session marker on each entry:
  reading index, elapsed-session time, or both.
- Mark the list clearly as "session review" or equivalent, not "active alerts".
- Clear the event log on logout, manual clear, patient change, and automatic
  patient/session reinitialization when the reading buffer rolls over.
- If entries are truncated because the event buffer is full, show that truncation
  explicitly to the user and in summary evidence.
- Keep the event log read-only. No acknowledge, dismiss, or escalation actions.
- If `patient_print_summary()` includes the event log, reuse the same source
  records and ordering as the GUI list.

## Validation expectations

- unit tests for event creation on warning->critical, critical->warning,
  abnormal-set changes, and stable readings that should not create duplicates
- unit/integration tests for session clear, logout, new patient, and timer
  rollover reset behavior
- verification that truncation or rollover is disclosed rather than silent
- DVT evidence showing a transient critical episode remains visible after the
  latest reading returns to warning or normal
- GUI review that labels, colors, and list placement do not imply current
  active-alarm status or diagnostic recommendation
- traceability updates for any new UNS/SYS/SWR items introduced

## Residual risk for this pilot

Residual risk remains that users may over-trust a bounded retrospective list,
especially if they do not understand session reset and retention limits. For the
pilot, this is acceptable only if the UI states those limits plainly and if the
existing live-alert surface remains primary.

## Human owner decision needed, if any

Product owner / architect should explicitly decide:

- the maximum event-buffer size and retention/discard wording
- whether automatic session rollover must display a visible "session reset"
  notice before design proceeds
- whether console summary parity is required in MVP or deferred to a later issue
