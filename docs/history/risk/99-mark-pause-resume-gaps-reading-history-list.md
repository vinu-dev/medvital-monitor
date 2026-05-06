## Risk Note: Issue #99 - Mark pause/resume gaps in the reading history list

Date: 2026-05-06
Issue: #99 "Feature: mark pause/resume gaps in the reading history list"
Branch: `feature/99-mark-pause-resume-gaps-reading-history-list`

## Proposed change

Add an explicit continuity marker to the dashboard reading-history surface when
live monitoring resumes after a user-initiated pause, so a paused session is
not misread as one uninterrupted monitoring run.

Repo observations supporting the change:

- `src/gui_main.c` rebuilds `IDC_LIST_HISTORY` from stored raw readings only,
  one row per `VitalSigns` entry.
- The pause button toggles `g_app.sim_paused`, and the timer stops appending new
  readings while paused.
- No pause/resume marker is currently stored or rendered, so the history list
  can make a resumed session look continuous.
- The current raw reading rows do not include wall-clock timestamps, so the UI
  does not otherwise disclose where monitoring continuity was interrupted.

This is a display and session-traceability change. It must not alter
acquisition, vital-sign classification, alarm generation, NEWS2 scoring, or
alert escalation behavior.

## Product hypothesis and intended user benefit

Hypothesis: a small continuity marker makes the session chronology more honest
and easier to review, especially during handoff, testing, or retrospective
session review.

Expected user benefit:

- faster recognition that monitoring was interrupted
- less risk that a reviewer infers uninterrupted physiological surveillance
- better alignment between the visible history list and the true session state

## Source evidence quality

Evidence quality is adequate for product-discovery scope and weak for any
clinical or usability effectiveness claim.

- The issue cites Philips Efficia CMS200 and DFM100 instructions for use as
  examples that trend or tabular views disclose unavailable or interrupted data.
- Those references were provided as short issue summaries, not as linked source
  excerpts in the issue thread, so they are sufficient only as untrusted
  workflow inspiration.
- The repo evidence is stronger for this decision than the competitor evidence:
  the current implementation clearly shows a continuity-disclosure gap in the
  local history list.

Conclusion: the candidate has enough evidence for a narrow, non-copying MVP,
but not enough evidence to justify vendor-specific visuals or stronger safety
claims.

## Medical-safety impact

This change does not change patient-state calculations, thresholds, alarms, or
treatment guidance. Risk is in presentation fidelity.

Primary safety benefit:

- reduces the chance that a clinician or tester mistakes a paused session for
  uninterrupted monitoring coverage

Primary safety risks:

- a missing marker could hide an interruption in surveillance continuity
- an incorrectly placed marker could imply a gap where none occurred
- ambiguous wording could imply patient stability during the pause rather than
  unknown or unobserved status
- repeated markers or markers lost on repaint could make the history list
  inconsistent across the same session

Overall medical-safety impact: low-to-moderate if implemented as a bounded,
read-only continuity disclosure. It is not acceptable if the marker is framed as
clinical interpretation or as proof that the patient remained safe during the
gap.

## Security and privacy impact

Security and privacy impact is limited.

- No new data category is introduced beyond existing patient-linked session
  observations.
- No new network, export, or persistence path should be added for this MVP.
- The marker must remain inside the existing authenticated local session and
  clear with the same session-reset boundaries as the current patient history.

## Affected requirements or "none"

Likely affected requirements:

- `UNS-009` Vital Sign History
- `UNS-014` Graphical User Interface
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-003` Colour-Coded Vital Signs Display

Likely unchanged if the design stays display-only:

- `SYS-009` Vital Sign Reading History storage semantics
- all alert-classification, NEWS2, and alarm-limit requirements

New or updated derived requirements are needed for:

- pause/resume continuity-marker trigger rules
- marker wording and visual distinction from real reading rows
- repaint-stable ordering and de-duplication behavior
- clear/reset behavior on logout, patient reset, simulation disable, and
  session rollover

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinical users and testers review the current session history and can
  see that monitoring continuity was interrupted by a pause

User population:

- bedside clinicians, ward reviewers, and internal testers using the Windows
  desktop application

Operating environment:

- local workstation session in the current pilot, using the existing GUI and
  session-scoped in-memory patient state

Foreseeable misuse:

- assuming the marker proves the patient was stable during the pause
- assuming the marker captures exact elapsed time when the system does not store
  that timing evidence
- confusing a continuity marker with a real physiological reading or active
  alarm
- assuming every missing section of coverage will be represented if the session
  is reset or truncated

## MVP boundary that avoids copying proprietary UX

The MVP should remain narrow:

- a plain-text separator row or other simple neutral marker
- no copied vendor colors, dashed-line motifs, or monitor-specific layouts
- no new timeline editor, retrospective gap repair, or enterprise audit trail
- no inferred clinical statement beyond "monitoring was paused/resumed" or
  equivalent continuity disclosure

## Clinical-safety boundary and claims that must not be made

The feature may disclose that continuity was interrupted. It must not claim to:

- confirm the patient remained stable during the paused interval
- estimate or summarize clinical deterioration during the paused interval unless
  the system truly records supporting time data
- replace active alerts, live monitoring, or clinician judgment
- act as a legal or complete longitudinal medical record

## Whether the candidate is safe to send to Architect

Yes, with constraints. The candidate is safe to send to Architect because it is
read-only, improves session traceability, and does not change clinical logic.
Design must keep the feature as continuity disclosure only and must define the
marker source of truth clearly enough to avoid duplication, omission, or false
precision.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a misleading history surface could cause a reviewer to over-trust monitoring continuity during handoff or retrospective review, but the feature does not itself trigger clinical actions. |
| Probability | Possible without controls because the current UI rebuilds the list on every refresh and does not store pause/resume boundaries in the displayed history model. |
| Initial risk | Medium |
| Key risk controls | Trigger the marker only from explicit pause/resume state transitions; keep it visually distinct from readings; avoid invented duration claims; clear it with the same session boundaries as the history list; verify no duplicate markers appear across repeated repaints. |
| Verification method | Unit tests for trigger and reset rules, integration or GUI checks for pause-then-resume ordering, repaint/idempotence checks, and traceability updates for any new GUI/session-history requirement. |
| Residual risk | Low if the marker remains a simple continuity disclosure and the reset/ordering rules are verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the change improves honesty of the displayed session record without changing any clinical inference engine, and live monitoring remains the primary safety surface. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Monitoring interruption is hidden | Resume occurs with no visible continuity marker | Reviewer assumes uninterrupted surveillance and underestimates uncertainty during the gap |
| False interruption is displayed | Marker is inserted at the wrong position or on ordinary refresh events | Reviewer questions valid session continuity or misreads chronology |
| Marker implies measured clinical stability | Wording suggests a calm or normal patient state during the pause | User over-trusts an interval with no live observations |
| History becomes internally inconsistent | Marker is duplicated, dropped on repaint, or not cleared with session reset | Confusing or conflicting review evidence across the same session |
| Mixed semantic surfaces | Marker styling resembles an alert row or reading row too closely | User misinterprets continuity metadata as active alarm or measured data |

## Existing controls

- The pause state is already explicit in `g_app.sim_paused`.
- The timer already gates reading acquisition on that pause state.
- The history list is already session-scoped and rebuilt deterministically from
  patient/session data.
- Authentication and local-session boundaries already exist for the dashboard.
- Alert semantics are already centralized elsewhere, which reduces the chance of
  this feature accidentally altering alarm logic if kept display-only.

These controls reduce implementation risk, but they do not yet define safe
continuity-marker semantics.

## Required design controls

- Derive the marker from explicit pause/resume transitions, not from heuristic
  timing gaps between readings.
- Keep the marker as session metadata with deterministic ordering, not as a
  transient one-off listbox side effect that can disappear or duplicate on
  repaint.
- Use wording that indicates unknown monitoring continuity, not patient
  condition. Example direction: "Monitoring paused" or "Monitoring resumed after
  pause", not "Patient stable during pause".
- Keep the marker visually distinct from both reading rows and active alerts.
- Do not show precise paused duration unless the system records it accurately.
- Clear marker state on logout, clear-session actions, patient re-admission, and
  automatic session rollover.
- Preserve existing alert and reading storage semantics unless a formal
  requirements update explicitly broadens them.

## Validation expectations

- unit tests for pause->resume marker creation rules
- unit or integration checks that repeated `update_dashboard()` calls do not
  duplicate or reorder markers
- verification that ordinary live ticks without pause do not create markers
- verification that clear, logout, patient reset, simulation disable, and
  buffer-rollover paths clear marker state
- GUI review that the marker is understandable and cannot be confused with an
  active alert or a measured reading
- traceability updates for any new or revised session-history GUI requirement

## Residual risk for this pilot

Residual risk remains that users may infer more timing precision than the
system actually records. For the pilot, that is acceptable only if the feature
stays simple, does not invent duration data, and is presented as continuity
metadata rather than clinical interpretation.

## Human owner decision needed, if any

Product owner and architect should explicitly decide:

- whether the MVP shows one row at resume time, one row at pause time, or a
  paired pause/resume disclosure
- whether exact clock time or elapsed paused duration is required before
  implementation
- whether the same continuity marker concept should apply only to simulation
  pause/resume or later to real-device acquisition interruptions
