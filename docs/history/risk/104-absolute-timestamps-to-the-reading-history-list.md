# Risk Note: Issue #104 - Absolute Timestamps in the Reading History List

Date: 2026-05-06
Issue: #104 "Feature: add absolute timestamps to the reading history list"
Branch: `feature/104-absolute-timestamps-to-the-reading-history-list`

## proposed change

Add a session-scoped capture timestamp to each stored vital-sign reading and
render it in the existing read-only reading history list.

Current repo observations show this is not a pure text-label change:

- `PatientRecord` currently stores `VitalSigns readings[MAX_READINGS]` plus
  `reading_count`, but no timestamp field.
- `src/gui_main.c` `update_dashboard()` currently renders each history row as
  `#<reading index> ... [severity]`, with no clock value.
- repository searches across `include/` and `src/` show no current time-source
  usage such as `GetLocalTime`, `SYSTEMTIME`, `time_t`, or `strftime`.
- the issue explicitly keeps clock synchronization, timezone handling, and
  cross-session persistence out of scope.

Therefore the MVP needs a new, explicit definition of what "absolute
timestamp" means in this pilot and how that timestamp is captured, stored,
displayed, and cleared.

## product hypothesis and intended user benefit

Hypothesis: showing the capture time for each stored reading will improve
handoff clarity and retrospective review because operators can correlate a
reading with pauses, alarm-state changes, and nearby notes without inferring
timing only from sequence order.

Expected benefit:

- faster interpretation of when a warning or critical value occurred
- less ambiguity during clinical handoff or tester review
- better alignment between the reading history and other session-scoped review
  surfaces

## source evidence quality

Evidence quality is adequate for a narrow workflow hypothesis and inadequate for
clinical-effectiveness or proprietary-UX claims.

- Philips Screen Trends states that trend and current data are shown together
  and that aligned time scales help staff evaluate several measurement trends at
  the same time. This supports the general workflow value of time-contextualized
  review. Source:
  https://www.usa.philips.com/healthcare/product/HCNOCTN175/screen-trends-patient-monitoring---decision-support
- A Philips IntelliVue FAST SpO2 application note states that displayed values
  may be continuous, timed, or "spot check" values and discusses trend views on
  the monitor. This supports the plausibility of time-associated measurement
  display, but it is not a direct UI specification for this feature. Source:
  https://www.documents.philips.com/assets/20250210/64a30873c01245d3bb7cb28001801c43.pdf

Important limitation: the issue text claimed Philips shows timestamps in
brackets. I did not verify that exact presentation claim from the cited
materials. The sources justify a narrow non-copying MVP, not a vendor-matching
layout.

## medical-safety impact

This change does not alter thresholds, alert generation, NEWS2 scoring, alarm
limits, or treatment guidance. Risk is introduced through chronology display,
not through new clinical calculations.

Primary safety benefit:

- improves the operator's ability to understand when a stored reading occurred
  during the current session

Primary safety risks:

- a wrong timestamp can make deterioration appear earlier or later than it was
- repaint-time or derived timestamps can misrepresent actual capture time
- a time-only display can appear "absolute" even though it becomes ambiguous if
  the session crosses midnight
- local workstation clock changes can create non-monotonic timestamps while the
  reading order itself remains correct
- session reset or rollover can make users assume old timestamps are still
  available when they are not

Overall medical-safety impact: low-to-moderate if the feature remains a
read-only adjunct to existing review surfaces and clearly discloses its local,
session-scoped timing semantics. It is not acceptable if the UI implies
audit-grade chronology, synchronized device time, or complete chart history.

## security and privacy impact

Security/privacy impact is limited but real because the feature adds finer
temporal detail to patient-linked observations already visible in the session.

- No new network path, export path, or cloud retention should be introduced.
- Timestamped rows must remain behind the existing authenticated session
  boundary.
- The same session-clear boundaries that already protect reading history must
  also clear timestamps.
- If future console or export surfaces include timestamps, they must not be
  framed as a durable legal medical record.

## affected requirements or "none"

Likely affected existing requirements:

- `UNS-009` Vital Sign History
- `UNS-010` Consolidated Status Summary
- `UNS-011` Data Integrity
- `SYS-009` Vital Sign Reading History
- `SYS-011` Patient Status Summary Display
- `SYS-012` Static Memory Allocation
- `SWR-PAT-002` Add Vital Sign Reading
- `SWR-PAT-003` Latest Reading Access
- `SWR-GUI-004` Patient Data Entry via GUI

New or revised requirements are needed for:

- timestamp source semantics
- timestamp storage and retention boundaries
- display format and local-time disclosure
- midnight / date-boundary behavior
- reset / clear / rollover behavior
- deterministic verification of clock-dependent behavior

No approved requirement currently defines timestamp capture or display semantics
for raw reading history.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinicians and testers review the local session history and see when
  each stored reading was captured

User population:

- bedside clinical staff, ward reviewers, and internal testers using the
  Windows workstation application

Operating environment:

- local Windows desktop application, using in-process bounded session storage
  with either simulation, manual entry, or future HAL-backed readings

Foreseeable misuse:

- assuming the displayed time is synchronized to a hospital clock or device
- assuming time-only rows remain unambiguous across midnight
- assuming the timestamp proves clinician response timing or alarm chronology
- assuming the session history is persistent or complete after reset/rollover
- using timestamped history as a legal medical record rather than a pilot review
  aid

## MVP boundary that avoids copying proprietary UX

The MVP should stay narrow:

- reuse the existing reading-history list and add a simple timestamp field or
  sublabel
- keep storage local, bounded, and session-scoped
- preserve the app's own visual language rather than copying vendor trend or
  central-station layouts
- do not add waveform review, trend overlays, enterprise retention, timezone
  controls, synchronization logic, acknowledgment workflows, or export/audit
  features

## clinical-safety boundary and claims that must not be made

The feature may support local retrospective review. It must not claim to:

- provide synchronized or authoritative clinical chart time
- replace the active alert surface or live monitoring view
- prove exact event order across device swaps, clock changes, or session resets
- provide a complete longitudinal patient record
- confirm clinician acknowledgment, escalation, or response timing

## whether the candidate is safe to send to Architect

Yes, with constraints. The issue is safe to send to Architect because it is a
bounded review-surface enhancement and does not propose new clinical logic.
However, architecture must treat "absolute timestamps" as a requirements and
risk question, not as a cosmetic string-format task.

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: misleading chronology during handoff or review could contribute to delayed recognition of deterioration, even though the feature does not itself generate or suppress alarms. |
| Probability | Possible without controls because the repo has no current time-source abstraction or timestamp storage model. |
| Initial risk | Medium |
| Key risk controls | Capture timestamp at the same moment the reading is accepted; store it with the reading; define local unsynchronized time semantics explicitly; keep reading order primary; clear timestamps with the session; avoid time-only "absolute" claims unless date context is included. |
| Verification method | Unit tests for capture/reset semantics, deterministic clock-injection tests, integration/manual GUI checks for display stability and ordering, and traceability updates for new timestamp requirements. |
| Residual risk | Low if the timestamp remains session-scoped, clearly labeled, and non-authoritative. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the feature remains an informational adjunct and existing live alerts stay the primary safety signal. |

## hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Recent deterioration is mis-sequenced | Timestamp captured at repaint, not at reading acceptance | Reviewer underestimates or mis-times instability |
| False precision about chronology | UI labels a local host time as "absolute" without disclosing limits | User over-trusts timing when correlating alarms or handoff events |
| Cross-midnight ambiguity | UI shows only `HH:MM` or `HH:MM:SS` without date context | Readings from different calendar days appear equivalent |
| Wrong-patient historical context | Timestamps survive patient clear, logout, or rollover | Patient mix-up during review |
| Order mismatch between time and sequence | Wall clock moves backward/forward during session and UI sorts by timestamp | History appears reordered incorrectly |
| Incomplete history looks complete | Bounded session reset removes old rows without clear disclosure | False confidence in retained chronology |

## existing controls

- Reading storage is already bounded by `MAX_READINGS`.
- Reading order is already explicit through `reading_count` and history-row
  numbering.
- Session data is already cleared through patient/session reinitialization
  paths.
- Authenticated session boundaries already limit local exposure of patient data.
- The current UI uses sequence order, which avoids any false implication of
  clock authority.

These controls constrain scope but do not yet define safe timestamp semantics.

## required design controls

- Define the pilot source of truth explicitly:
  capture local workstation date/time when a reading is successfully appended,
  not when the GUI repaints.
- Store one timestamp per retained reading in the same bounded session model as
  the reading itself.
- Keep list ordering based on reading sequence, not by re-sorting timestamps.
- If the feature is called "absolute timestamp", include enough date/time
  context to avoid cross-midnight ambiguity; otherwise rename it to local
  capture time.
- Disclose that the displayed time is local session time and not synchronized
  device or enterprise chart time.
- Clear timestamped history on the same boundaries that clear readings:
  logout, manual clear, patient change, and automatic rollover/reset.
- Use one shared capture path for manual entry, simulator-driven readings, and
  future hardware-fed readings.
- Introduce a small deterministic time-source seam or wrapper so verification
  does not depend on real wall-clock sleeps.
- Update requirements and traceability before implementation is considered
  complete.

## validation expectations

- unit tests proving timestamps are stored only on successful reading append
- unit tests proving timestamps clear with patient/session reset
- deterministic tests for date rollover / clock-source formatting behavior
- GUI review showing timestamps remain stable across repaint and pause/resume
- verification that manual entry and simulated readings use the same timestamp
  semantics
- requirements and RTM updates covering the new timestamp behavior

## residual risk for this pilot

Residual risk remains that operators may infer more clock authority than the
pilot actually provides. That risk is acceptable only if the UI keeps the
feature explicitly local, session-scoped, and secondary to live monitoring and
active alerts.

## human owner decision needed, if any

Product owner and architect should explicitly decide:

- whether "absolute timestamp" in this pilot means local workstation date/time
  at reading capture
- whether the MVP will show date plus time, or whether the feature should be
  renamed if only time-of-day is shown
- what future source of truth should apply when real hardware is integrated:
  host receive time or device-supplied measurement time
