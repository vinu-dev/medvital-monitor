# Risk Note: Issue #83 - Surface Active Alert Source in Status Banner

Date: 2026-05-06
Issue: #83 "Feature: surface the active alert source in the status banner"
Branch: `feature/83-surface-active-alert-source-in-status-banner`

## Proposed change

Add a short, deterministic source label to the existing status banner so the
dashboard shows which monitored parameter is currently driving the aggregate
warning or critical state.

Repo observations that matter for risk:

- `src/gui_main.c` `paint_status_banner()` currently uses banner colour to
  reflect `patient_current_status(&g_app.patient)`, but the text path is
  already occupied by device-mode messaging when simulation is off and by a
  rolling simulation message when simulation is on.
- `src/patient.c` `patient_current_status()` exposes only the aggregate
  `AlertLevel`; it does not identify a source parameter.
- `src/alerts.c` `generate_alerts()` emits abnormal parameters in fixed
  parameter order (`Heart Rate`, `Blood Pressure`, `Temperature`, `SpO2`,
  `Resp Rate`), not highest-severity order.
- `src/vitals.c` `overall_alert_level()` skips respiration rate when
  `respiration_rate == 0`, so the source label must preserve that "not
  measured" behavior and must not invent an RR-driven cause from missing data.

This is a presentation-only change if kept narrow, but it touches a
high-visibility safety surface and therefore needs explicit design controls.

## Product hypothesis and intended user benefit

Hypothesis: when the dashboard is already in a warning or critical state, a
small source label reduces the time needed to orient to the main abnormal
parameter without opening a second review path.

Expected user benefit:

- faster first-glance orientation to the active problem source
- less scanning between the aggregate banner and the active-alert list
- improved reviewer confidence that the banner and active alerts refer to the
  same latest reading

## Source evidence quality

Evidence quality is adequate for product-discovery scope and weak for clinical
or usability-effectiveness claims.

- Philips public marketing page for Clinical Surveillance was reachable during
  this review and supports the general claim that at-a-glance surveillance
  products expose prioritized patient context. Source:
  https://www.usa.philips.com/healthcare/acute-care-informatics/clinical-surveillance
- The GE HealthCare page returned `Access Denied` to a non-interactive fetch
  during this review, which reduces auditability of the cited claim. Source:
  https://www.gehealthcare.com/en-us/products/patient-monitoring
- The Masimo UniView page returned a bot-challenge landing page (`Just a
  moment...`) during non-interactive fetch, so the source is public but not
  easily reviewable in this environment. Source:
  https://www.masimo.com/products/hospital-automation/uniview/

Conclusion: the sources are enough to justify a narrow, non-copying UI
clarification. They are not enough to justify claims that this label improves
clinical outcomes, alarm response time, or diagnostic accuracy.

## Medical-safety impact

This change does not alter thresholds, NEWS2 logic, alert generation rules, or
treatment guidance. Risk arises from misrepresentation on a high-visibility
banner, not from new clinical computation.

Primary safety benefit:

- quicker recognition of which parameter is currently driving the aggregate
  warning or critical state

Primary safety risks:

- the wrong source parameter is shown because implementation chooses the first
  alert in fixed parameter order instead of the highest-severity parameter
- a stale label remains visible after recovery, patient clear, or patient
  change
- the banner label hides or dilutes the existing device-mode or simulation-mode
  disclosure already required on that surface
- users over-trust a single label and miss that multiple parameters are
  simultaneously abnormal

Overall medical-safety impact: low-to-moderate if the label remains a small
adjunct to the existing alert list and is derived deterministically from the
same latest-reading semantics.

## Security and privacy impact

Security and privacy impact is limited.

- No new patient data category is introduced; the label is derived from the
  same vital-sign values already shown in the dashboard.
- No new storage, export, clipboard, or network path should be introduced in
  this MVP.
- The label must remain behind the existing authenticated local session.
- If the label text is ever reused in logs, summaries, or exports later, that
  should be reviewed separately; this issue should stay display-only.

## Affected requirements or "none"

This issue is not requirement-neutral in the current repo because the banner
text already has explicit behavior.

Likely affected or adjacent requirements:

- `SYS-014` Graphical Vital Signs Dashboard
- `SYS-005`, `SYS-006`, and `SYS-011` as reused alert semantics and status
  summary context
- `SWR-GUI-003` colour-coded vital signs display and aggregate banner behavior
- `SWR-GUI-010` device-mode banner text
- `SWR-GUI-011` rolling simulation-mode banner text
- `SWR-VIT-005` aggregate alert level
- `SWR-VIT-008` respiration-rate "not measured" behavior
- `SWR-ALT-001` through `SWR-ALT-004` as the closest existing structured alert
  semantics

If the team decides this feature is permanent, the banner-source behavior
should be formalized rather than treated as "none."

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinicians and testers use the label as a quick cue for the current
  source of aggregate deterioration on the latest reading only

User population:

- bedside clinicians, ward reviewers, and internal testers using the Windows
  workstation application

Operating environment:

- local authenticated desktop session in the current pilot, with live simulated
  or future device-fed readings and no remote monitoring scope in this issue

Foreseeable misuse:

- treating the label as a full explanation rather than a shorthand cue
- assuming the named source is the only abnormal parameter
- assuming a label in simulation mode implies real-device monitoring
- treating missing RR data as an RR problem if the labeling logic does not
  preserve the existing `respiration_rate == 0` exception

## MVP boundary that avoids copying proprietary UX

The MVP should remain implementation-led and narrow:

- one short text token or badge inside the existing banner
- source tokens limited to the app's own parameter abbreviations such as `HR`,
  `BP`, `Temp`, `SpO2`, or `RR`
- deterministic selection from the latest reading only
- no trend ranking, no waveform drill-down, no remote surveillance patterns,
  no event timeline, and no alarm acknowledgment workflow
- no competitor-specific central-station layouts or enterprise review features

## Clinical-safety boundary and claims that must not be made

The feature may support quicker orientation. It must not claim to:

- diagnose the cause of deterioration
- recommend treatment, escalation priority, or intervention timing
- replace the active-alert list or patient summary
- imply that only one parameter is abnormal when several are out of range
- suppress or override device-mode or simulation-mode provenance messaging

## Whether the candidate is safe to send to Architect

Yes, with constraints.

The candidate is safe to send to Architect because it is a deterministic,
display-only change that can reuse existing alert semantics. It is only safe if
design explicitly handles three points:

1. Highest-severity selection must be implemented deliberately, not inferred
   from the fixed output order of `generate_alerts()`.
2. The label must clear when the patient is absent, the latest reading is
   normal, or the source parameter is not measured.
3. Existing device-mode and simulation-mode banner disclosures must remain
   visible or be preserved equivalently.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a misleading banner label could delay correct interpretation on a highly visible clinical surface, but the feature does not change thresholds or trigger logic. |
| Probability | Possible without controls because the current code does not expose a canonical "source parameter" and the obvious implementation shortcut would be to reuse fixed-order alert output. |
| Initial risk | Medium |
| Key risk controls | Derive the source from the same latest-reading rules as the aggregate alert; choose highest severity first and use a documented tie-break; clear the label on normal/absent/no-data states; preserve device/simulation disclosures; keep the active-alert list primary. |
| Verification method | Unit tests for source selection and clear rules; targeted tests for RR-not-measured handling; GUI manual review for banner wording/layout in simulation and device modes; traceability updates if the feature is formalized. |
| Residual risk | Low if controls are implemented and verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot if the label stays supplementary, deterministic, and visibly subordinate to the full active-alert list and existing mode/provenance messaging. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Wrong parameter is emphasized | Banner picks first abnormal parameter rather than highest-severity parameter | User attends to the wrong vital sign first and delays review of the more severe abnormality |
| Stale source remains after state change | Label persists after recovery, patient clear, logout, or patient switch | User acts on outdated patient context |
| Mode/provenance message is obscured | Source label replaces device-mode or simulation-mode text without equivalent disclosure | User misreads simulated or unavailable data as live device monitoring |
| Multiple abnormalities are oversimplified | Single label implies exclusivity when several parameters are abnormal | User underestimates broader deterioration pattern |
| Missing RR is misclassified | `respiration_rate == 0` is treated as RR-driven abnormality | False cue toward respiratory failure despite absent measurement |
| Cross-surface inconsistency | Banner source, active-alert list, and aggregate colour are derived by different logic | Reviewer confusion and reduced trust in the UI |

## Existing controls

- Aggregate severity is already centralized in `overall_alert_level()`.
- Structured per-parameter abnormality detection already exists in
  `generate_alerts()`.
- RR missing-data handling is already defined in `overall_alert_level()` and
  `generate_alerts()`.
- Banner colour already reflects aggregate severity and therefore provides a
  stable severity surface independent of the proposed source label.
- Authentication, local-session scope, and static storage already reduce
  privacy and persistence risk.

These controls reduce risk, but they do not by themselves define safe banner
source-selection semantics.

## Required design controls

- Add one canonical helper or equivalent design rule for banner-source
  selection from the latest reading.
- Select the source parameter by highest severity first, then by a documented
  deterministic tie-break. Do not treat the first entry produced by
  `generate_alerts()` as the source unless the helper explicitly enforces that
  severity policy.
- Reuse existing parameter classification rules; do not create a second set of
  thresholds or special banner-only logic.
- Preserve `respiration_rate == 0` as "not measured" and never show `RR` as the
  active source in that case.
- Clear the label when no patient is present, no reading exists, or overall
  status is `ALERT_NORMAL`.
- Preserve the current device-mode and simulation-mode safety/provenance
  messaging on the banner, either concurrently or through an explicit approved
  replacement.
- Keep the active-alert list and patient summary as the authoritative detailed
  views when multiple abnormalities exist.
- Keep the label short and unambiguous; avoid words that imply diagnosis,
  causality, or clinician action.

## Validation expectations

- unit tests for source selection where the highest-severity parameter is not
  the first abnormal parameter in fixed alert order
- unit tests for deterministic tie-break behavior when two parameters share the
  same severity
- unit tests proving label clearing on normal state, no patient, no reading,
  patient reset, and RR-not-measured cases
- GUI manual review in simulation mode and device mode to verify the source
  label does not suppress required banner disclosures
- manual review with multi-parameter abnormal readings to confirm the active
  alert list still communicates the full abnormal set
- traceability and requirement updates if banner text behavior changes from the
  currently approved SWR wording

## Residual risk for this pilot

Residual risk remains that users may over-read a single short label as a full
clinical explanation. For the pilot, that is acceptable only if the UI keeps
the label visually secondary, preserves the existing alert list, and does not
hide mode/provenance information already carried by the banner.

## Human owner decision needed, if any

Product owner / architect should explicitly decide:

- whether the banner must preserve the current rolling simulation message,
  replace it, or display both
- the approved deterministic tie-break order when multiple parameters share the
  same highest severity
- whether permanent implementation requires immediate SWR/traceability updates
  or can proceed as a documented design refinement first
