# Risk Note: Issue #98 - explicit RR not-measured marker

Date: 2026-05-06
Issue: #98 "Feature: show an explicit not-measured marker for missing respiration-rate values"
Branch: `feature/98-explicit-not-measured-rr-marker`

## Proposed change

Add an explicit respiration-rate missing-state marker on clinician-facing
surfaces that currently hide the parameter when
`VitalSigns.respiration_rate == 0`, using the repository's existing pilot
semantic that `0` means "not measured" for that cycle.

The MVP should stay presentation-only:

- no change to RR thresholds
- no change to aggregate alert escalation
- no change to NEWS2 scoring
- no change to alarm generation
- no change to sensor transport, reconnect logic, or device-mode behavior

Repo observations supporting the change:

- `src/gui_main.c` already renders the live RR tile as `--` when
  `respiration_rate == 0`.
- `src/gui_main.c` still omits RR from reading-history rows when the value is
  missing.
- `src/patient.c` `patient_print_summary()` does not print an RR line at all,
  so the summary cannot distinguish "not measured" from "not shown."

## Product hypothesis and intended user benefit

Hypothesis: an explicit RR missing-state marker reduces ambiguity at bedside
review and handoff by showing that respiration rate was not acquired, instead of
making the field look absent, forgotten, or clinically normal.

Expected user benefit:

- faster recognition that RR data is unavailable for the current cycle
- less chance that a hidden RR field is mistaken for a valid zero or an
  intentionally omitted parameter
- better alignment between live UI review and console summary evidence

## Source evidence quality

Evidence quality is adequate for a narrow product-discovery MVP and inadequate
for any clinical-effectiveness claim.

- Philips Efficia CMS200 Instructions for Use provide strong workflow evidence
  that tabular trend rows should distinguish unsupported, invalid, and not-
  measured states. The manual states that rows begin with measurement time,
  unsupported parameters display as blank values, invalid measurements use
  `-?-`, and a dash indicates the parameter was not measured.
  Source: https://www.documents.philips.com/assets/Instruction%20for%20Use/20211217/8b704cfe388e4e6e9071ae0101520983.pdf?feed=ifu_docs_feed
- Philips Efficia DFM100 Instructions for Use provide moderate workflow
  evidence. The manual states that invalid trend data use `-?-`, unavailable
  data use blank space, and parameters not measured during the display period
  may be omitted from the listed trend rows.
  Source: https://www.documents.philips.com/assets/Instruction%20for%20Use/20220803/9a0d2f44662e41ca8df0aee600389d2f.pdf?feed=ifu_docs_feed

Conclusion: the sources support the product hypothesis that explicit
data-availability signaling is standard monitoring behavior, but they also show
that vendors vary between dash, blank, and omission. They are sufficient for a
non-copying MVP and not sufficient to claim improved clinical outcomes.

## Medical-safety impact

This is a display-only change if the scope stays bounded. It does not introduce
new thresholds, new interpretation logic, or new treatment advice.

Primary safety benefit:

- makes the absence of an RR measurement visible instead of silent

Primary safety risks:

- the user may misread `--` as "stable" rather than "not measured"
- the UI may conflate "not measured" with "no patient", "simulation off", or
  stale data
- the existing sentinel design (`respiration_rate == 0` means not measured)
  could be reinforced in a way that becomes unsafe if a future real-device path
  ever uses `0` for a transport fault or a true physiological zero condition

Overall medical-safety impact: low-to-moderate if the design remains a
presentation of an already-approved missing-data semantic, and not acceptable if
the issue expands into sensor-state inference or alarm-logic changes.

## Security and privacy impact

Security and privacy impact is low.

- no new network path should be introduced
- no new persistent storage should be introduced
- no authentication or authorization behavior should change
- the summary and dashboard views should remain within the current authenticated
  local-session boundary

## Affected requirements or "none"

Likely affected existing requirements:

- `UNS-010` Consolidated Status Summary
- `UNS-014` Graphical User Interface
- `UNS-015` Live Monitoring Feed
- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard
- `SYS-018` Respiration Rate Monitoring and Classification
- `SWR-VIT-008` Respiration Rate Check and RR not-measured handling
- `SWR-PAT-006` Patient Summary Display

Potential requirements clarification needed:

- if the approved MVP includes dashboard reading-history rows, the repo likely
  needs explicit GUI-level wording for how missing RR is shown there, because
  current approved GUI text does not clearly define missing-state presentation
  outside the existing RR tile behavior

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinical users review whether RR is present for the current patient
  and current monitoring cycle

User population:

- bedside clinicians, ward reviewers, and internal testers using the Windows
  workstation application

Operating environment:

- current local desktop pilot using in-process static storage, simulation mode,
  and future HAL-backed device mode

Foreseeable misuse:

- assuming `--` means the patient is clinically normal
- assuming `--` means the sensor is disconnected when the real condition is
  "simulation off", "no patient", or another data-source state
- assuming omitted RR in history or summary means it was not clinically
  relevant
- carrying the `RR == 0` sentinel into future real-device integration without
  revalidating whether `0` can ever mean something else

## MVP boundary that avoids copying proprietary UX

The MVP should remain narrow and implementation-led:

- use a plain local marker such as `RR --` or `RR not measured`
- standardize one marker within this app rather than reproducing a competitor's
  tabular layout
- no new icons, waveform conventions, central-station layouts, or retrospective
  analytics
- no new alarm, fault, or advisory category
- no changes to non-RR parameter presentation unless required for consistency

## Clinical-safety boundary and claims that must not be made

This feature may indicate data availability only. It must not claim to:

- detect or diagnose apnea
- confirm sensor disconnection
- improve alarm performance or NEWS2 accuracy
- replace active alerts or live monitoring
- prove that the patient had a normal respiration rate

The design must keep a visible distinction between:

- RR not measured
- no patient / no reading available
- simulation or device mode states that already use `N/A`
- an actual numeric RR value

## Whether the candidate is safe to send to Architect

Yes, with constraints.

The candidate is safe to send to Architect because it is bounded to existing
missing-data semantics and does not require new clinical algorithms. It is not
safe to broaden this issue into sensor-fault inference, new alerting behavior,
or a new real-device data contract without separate human approval.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: hiding a missing RR value can create false confidence that the displayed patient state is complete, which may delay recognition that a clinically important parameter was not acquired. |
| Probability | Possible without controls because the current repo already shows inconsistent behavior across surfaces and uses a sentinel value that can be misunderstood. |
| Initial risk | Medium |
| Key risk controls | Treat the change as presentation-only; show one approved marker consistently on the scoped surfaces; preserve existing alert and NEWS2 behavior; distinguish `not measured` from `N/A` and no-patient states; keep `respiration_rate == 0` reserved for the approved pilot sentinel unless a future hardware design explicitly changes that contract. |
| Verification method | Add summary-level tests for RR present vs missing output; add or update GUI/manual evidence for the selected dashboard surfaces; run RR alert and NEWS2 regression checks to confirm that `0` still suppresses spurious escalation only in the approved not-measured path; update requirements text if new GUI surfaces are included. |
| Residual risk | Low if controls are implemented and verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the feature does not become a new clinical decision path and because the existing live alert logic remains the primary safety signal. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Missing measurement remains hidden | Summary or selected UI surfaces continue to omit RR entirely | User assumes patient state is complete and misses that RR was not acquired |
| Wrong state is labeled as not measured | `--` is shown for no patient, simulation-off, stale data, or another non-RR-specific condition | User misinterprets system state and delays corrective action |
| Sentinel collision in future hardware work | A future device path uses `0` for a transport fault or physiologic zero, but UI and aggregate logic still treat it as not measured | Critical deterioration or acquisition fault is masked |
| Surface inconsistency | RR tile, summary, and history surfaces use different missing-state rules or wording | Confusion during bedside review, testing, or handoff |
| Scope creep changes clinical behavior | Design quietly modifies alerts, NEWS2, or sensor-state logic while pursuing the UI fix | Unverified safety behavior enters implementation |

## Existing controls

- `src/vitals.c` treats `respiration_rate == 0` as not measured in
  `overall_alert_level()` so missing RR does not cause spurious escalation.
- `src/alerts.c` skips RR alerts when `respiration_rate == 0`.
- `src/news2.c` assigns RR sub-score `0` when `respiration_rate == 0`.
- Automated tests already cover the sentinel behavior, including:
  `OverallAlert.SWR_VIT_008_RRZeroSkipped` in `tests/unit/test_vitals.cpp`,
  `News2Calc.RR_Zero_SkippedInScore` in `tests/unit/test_news2.cpp`, and RR
  missing fixtures in `tests/unit/test_alerts.cpp` and
  `tests/unit/test_patient.cpp`.
- `src/gui_main.c` already shows `--` on the live RR tile, reducing the amount
  of new behavior required for the MVP.
- The issue explicitly constrains scope to display-only behavior and leaves
  alarm logic unchanged.

## Required design controls

- Define one approved missing-state label and use it consistently on every
  surface included in scope.
- Keep the change presentation-only. Do not change thresholds, alert messages,
  NEWS2 scoring, or aggregate alert semantics in this issue.
- Preserve the distinction between RR missing, `N/A`, and no-patient states.
- Extend approved requirement text if the MVP includes surfaces not already
  governed clearly by `SWR-PAT-006` and current GUI requirements.
- Document that `respiration_rate == 0` is a pilot sentinel, not a general
  physiological truth, and require re-review before any future HAL/device
  integration changes that contract.
- Verify that a present RR value still prints numerically and that a missing RR
  value is never silently omitted on scoped surfaces.

## Validation expectations

- unit-test evidence that `patient_print_summary()` prints an explicit RR
  missing marker when `respiration_rate == 0`
- unit-test or targeted review evidence that a numeric RR still prints
  correctly when present
- GUI or DVT evidence for any selected dashboard/history surfaces in scope
- regression confirmation that RR missing-state handling in alerts and NEWS2 is
  unchanged
- manual verification that simulation-off and no-patient states still show the
  appropriate non-patient placeholder rather than the RR missing marker
- traceability updates if the approved design introduces new surface-specific
  wording

## Residual risk for this pilot

Residual risk is low if the team exposes only the already-approved not-measured
state and keeps the design disciplined about state distinctions. Residual risk
returns to medium if the sentinel semantics remain implicit or are broadened
without a hardware-contract review.

## Human owner decision needed, if any

Product owner / architect should explicitly decide:

- whether the approved label is `--`, `Not measured`, or another plain-text
  equivalent
- whether the MVP includes only the latest summary/live display surfaces or
  also includes dashboard reading-history rows
- whether `respiration_rate == 0` remains the approved not-measured sentinel
  for the current pilot and must be revisited before real-device integration
