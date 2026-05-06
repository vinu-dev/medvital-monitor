# Risk Note: Issue 80

Issue: `#80`
Branch: `feature/80-recent-readings-trend-table`

## proposed change

Add a compact, read-only dashboard table beneath the existing trend display so
operators can review the latest 5 stored readings from the current patient
session without relying on the sparkline alone.

The bounded MVP should display only values that already exist in
`PatientRecord.readings`, plus an explicit row order indicator and the existing
aggregate alert level. It should not add new calculations, alarms, thresholds,
decision support, export, persistence, or cross-session history.

## product hypothesis and intended user benefit

The product hypothesis is that a short tabular view of recent readings will
make adjacent-value comparison easier during review, handoff, and simulated
deterioration assessment than a sparkline-only surface.

Expected benefit is ergonomic rather than algorithmic: faster confirmation of
whether recent readings are stable, rising, or falling using values already
present in the session buffer.

## source evidence quality

Internal evidence quality is moderate:

- the repo already documents trend sparkline review, patient history retention,
  and a single-screen monitoring dashboard
- the current data model already stores sequential readings with a bounded
  static buffer (`MAX_READINGS = 10`)

External evidence quality is low to moderate and suitable only for product
discovery:

- the Philips quick guide and GE marketing page support the general product
  idea that tabular trend review exists in commercial monitors
- neither source is clinical evidence that this specific UI change improves
  outcomes, and neither should be treated as justification for copying layout
  or workflow details

## MVP boundary that avoids copying proprietary UX

The MVP boundary should remain narrow:

- show only the latest 5 in-session readings already retained by the product
- use straightforward columns for sequence order, vital values, and aggregate
  alert level
- avoid competitor-specific timeline controls, 72-hour review workflows,
  filtering, export, alarm review consoles, or disclosure-style replay
- avoid any suggestion that the table is a replacement for the existing tiles,
  status banner, alerts list, or NEWS2 display

## clinical-safety boundary and claims that must not be made

This feature is safe only as a secondary review surface. It must not be
described as:

- diagnosing deterioration
- confirming trend stability beyond the displayed rows
- replacing clinical judgment, alarm review, or NEWS2 assessment
- providing true timestamps or elapsed-time precision when the current data
  model stores only sequence order, not capture time

## whether the candidate is safe to send to Architect

Yes, with controls. The issue is specific enough and medically bounded enough
to proceed to architecture and design, provided the implementation stays
read-only, uses only the existing session data, and does not imply timing or
clinical interpretation that the model does not support.

## medical-safety impact

Direct clinical-logic impact is low because the change does not alter
acquisition, validation thresholds, alert generation, NEWS2 scoring, or
treatment guidance.

The real safety risk is display integrity. A stale, misordered, or mislabeled
table could cause a clinician to misread recent patient direction and delay
appropriate review. In a monitoring product, even read-only presentation
changes need explicit controls because misleading display can still contribute
to unsafe human decisions.

## security and privacy impact

Security and privacy impact is low if the feature remains read-only and
session-local.

The table will duplicate patient data already visible in the dashboard, so it
must not introduce:

- new persistence
- export or copy features
- logging of readings to files
- network transmission

If future scope adds export, printing, or remote sharing, privacy review should
be reopened.

## affected requirements or "none"

This should not be treated as requirement-neutral.

Likely affected or adjacent requirements:

- `UNS-009` vital sign history
- `UNS-010` consolidated summary
- `SYS-009` vital sign reading history
- `SYS-011` patient status summary display
- `SYS-014` graphical vital signs dashboard
- `SWR-GUI-003` colour-coded dashboard display
- `SWR-GUI-004` patient data entry and dashboard behavior
- `SWR-TRD-001` existing trend review surface, if the table is presented as a
  paired trend-review feature

At minimum, architecture and design should confirm whether an existing GUI
requirement can absorb this behavior or whether a new software requirement is
needed for traceable verification.

## intended use, user population, operating environment, and foreseeable misuse

Intended use remains unchanged: trained clinical users review patient vital
signs on a Windows workstation within the existing monitoring dashboard.

Foreseeable misuse includes:

- assuming row order implies exact timestamps or equal sampling intervals
- trusting the table over the current alert banner when displays disagree
- using the table as a clinical interpretation aid rather than a raw review aid
- assuming the table contains more history than the bounded session buffer
- reading stale rows after patient refresh, session clear, or simulation-state
  transitions

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
|------|------------|
| Severity | Moderate if misleading presentation contributes to delayed review of deterioration; low for bounded display-only defects that are obvious to the user |
| Probability | Low, because the feature can reuse existing stored data and does not require new clinical calculations |
| Initial risk | Medium |
| Risk controls | Explicit row ordering; no timestamps unless the data model changes; read-only behavior; fixed row cap of 5; reuse existing alert/status logic; no new persistence or export; clear empty-state handling; requirement traceability update |
| Verification method | Targeted unit tests for any row-mapping helper, GUI verification of ordering and refresh behavior, regression checks that alert and NEWS2 behavior are unchanged, and manual review for patient/session transitions |
| Residual risk | Low |
| Residual-risk acceptability rationale | Acceptable for the pilot if the table remains a bounded review aid and the design prevents implied timing, stale data, and conflicting status presentations |

## hazards and failure modes

- newest and oldest rows are reversed or ambiguously labeled
- table rows do not refresh from the same snapshot as the sparkline, tiles, or
  alert banner
- the UI presents timestamps or elapsed-time cues that are not supported by the
  current data model
- the table survives patient refresh, logout, or session clear and shows the
  wrong patient's values
- the aggregate alert level shown in the table diverges from existing
  `overall_alert_level()` behavior
- row truncation, missing units, or inconsistent formatting causes misreading
  of temperature, SpO2, or respiration values
- the design tries to exceed the bounded history buffer or adds dynamic storage
  to preserve more rows than the current safety envelope allows

## existing controls

- `PatientRecord` stores sequential readings in fixed static storage with
  `MAX_READINGS = 10`
- `patient_add_reading()` rejects additions when the buffer is full instead of
  silently overwriting historical data
- `patient_latest_reading()` and the trend helpers already consume the same
  session-local reading buffer used by the current dashboard
- current alert and NEWS2 logic already define the clinically meaningful status
  outputs; the proposed feature does not need new clinical calculations
- the issue scope already excludes thresholds, diagnosis, treatment advice,
  export, and long-term persistence

## required design controls

- Use only existing `PatientRecord.readings` data; do not add new persistence,
  shadow history stores, or dynamic allocation.
- Cap the table at `min(reading_count, 5)` rows.
- Make row order explicit and human-readable. If the system cannot show true
  timestamps, label rows by sequence or recency only.
- Render the table from the same patient/session snapshot used for the tiles
  and sparkline so the dashboard cannot disagree with itself after a refresh.
- Reuse existing units and alert-level semantics; do not introduce a new risk
  score, trend classifier, or interpretation label in the table.
- Handle zero readings, patient refresh, logout, clear session, paused
  simulation, and full-buffer behavior explicitly.
- Update requirement traceability before verification claims are made.

## validation expectations

- Verify that the table never shows more than 5 rows and never mutates
  `PatientRecord`.
- Verify row ordering and row labeling against known scenario sequences.
- Verify that values in the table match the current patient buffer after manual
  entry, simulation updates, scenario loads, and session reset actions.
- Verify that no timestamps, time deltas, or timeline claims appear unless a
  reviewed requirement and data model change add true capture-time support.
- Verify that alert/status presentation remains consistent with existing
  `overall_alert_level()` and NEWS2 outputs and that no clinical logic changes
  occurred.

## residual risk for this pilot

Residual risk is low if the implementation remains a bounded review-only table.
The main remaining risk is human over-interpretation of short-run tabular data,
which is acceptable for the pilot when the UI avoids claims about timing,
prediction, or clinical recommendation.

## human owner decision needed, if any

None for the bounded MVP described above.

If stakeholders want wall-clock timestamps, export, longer review history, or
any claim that the table supports clinical decision-making beyond raw review,
that expansion should be raised as a separate issue with updated requirements
and risk review.
