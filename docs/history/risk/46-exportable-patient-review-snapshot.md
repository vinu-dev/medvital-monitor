# Risk Note: Issue #46

Date: 2026-05-06
Issue: `#46` - Feature: add exportable patient review snapshot

## proposed change

Add a read-only, user-initiated export that writes a compact plain-text patient
review snapshot for the active session.

The acceptable pilot scope is:

- local export only
- deterministic text output
- current patient demographics
- latest vital signs
- aggregate status and active alerts
- bounded trend direction derived from the existing trend helpers

The change must not modify acquisition, thresholds, NEWS2 scoring, alarm logic,
or network behavior.

## product hypothesis and intended user benefit

The hypothesis is that operators need a durable handoff/review artifact because
the current product shows live dashboard state and session history but does not
produce a compact record they can save or hand off without manual transcription.

The intended user benefit is reduced transcription burden and fewer handoff
omissions. The intended benefit is not new clinical decision support.

## source evidence quality

The issue cites public Philips and Drager product literature showing that
retrospective review, trend review, and export-style capabilities are common in
this category.

Evidence quality is low-to-moderate:

- it is vendor marketing or technical literature, not independent clinical
  evidence
- it supports the product hypothesis that review/export is expected
- it does not justify copying vendor UX, HL7 scope, or medical-record claims

This is enough to justify a narrow MVP, but not enough to justify a richer
workflow such as event export, transport review, or EMR integration.

## medical-safety impact

The main medical-safety risk is not new threshold behavior. The main risk is
that an exported artifact may be mistaken for a current, complete, or
authoritative clinical record after the live dashboard has moved on.

This issue also proposes exporting textual trend direction. The repository
already contains deterministic bounded trend logic in `trend.c`, but the GUI
currently presents trend visually via sparklines rather than as explicit
textual interpretation. Converting that helper into an exported phrase increases
the risk of over-interpretation unless the output stays clearly scoped to a
small local session history.

If the feature remains a clearly labeled snapshot of current session data, the
change is assessable and does not require blocking at the risk stage.

## security and privacy impact

The exported snapshot would contain patient identifiers and current physiological
data, so it creates privacy and confidentiality risk beyond the present
dashboard-only workflow.

Current controls limit exposure by requiring authentication before patient-data
access and by keeping the live session local. The proposed export adds durable
data persistence, so the design must control where data can be written, how
write failures are handled, and how the artifact is framed to the operator.

## affected requirements or none

- New UNS, SYS, and SWR entries are likely required for the export artifact,
  export gating conditions, output content, error handling, and privacy/scope
  boundary.
- Existing related requirements and evidence:
  `UNS-008`, `UNS-010`, `UNS-013`, `UNS-014`
  `SYS-011`, `SYS-013`, `SYS-014`
  `SWR-PAT-006`, `SWR-TRD-001`, `SWR-GUI-003`, `SWR-GUI-004`
- This change should not alter any existing clinical-threshold requirement.

## whether the change adds, changes, removes, or depends on an AI/ML model

This change does not add, change, remove, or depend on any AI/ML model.

It depends only on existing deterministic formatting, alerting, and trend
helpers. No training, tuning, validation dataset, bias-monitoring plan, or
predetermined change control plan is needed for the narrow MVP.

The human remains fully responsible for clinical interpretation. The design
must not present the exported trend direction as a prediction or recommendation.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- allow trained clinical staff to export a compact review snapshot of the active
  local session for handoff or retrospective review

User population:

- trained clinical staff using the workstation-based pilot

Operating environment:

- local Windows workstation
- authenticated in-product session
- pilot workflow with no network sync or EMR integration

Foreseeable misuse:

- treating the snapshot as a complete or current chart after more readings
  arrive
- using the snapshot as an official medical record or audit log
- exporting the wrong patient after session reset or patient refresh
- over-trusting textual trend direction as clinical advice
- exporting with insufficient history and assuming the trend is meaningful
- saving the file to an insecure shared location

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: medium, because misuse could contribute to incorrect handoff,
  delayed recognition of deterioration, or unauthorized disclosure of patient
  data.
- Probability: medium without controls, because the purpose of the feature is to
  create a durable artifact that may be reused outside the live screen.
- Initial risk: medium.
- Risk controls:
  user-initiated export only; no background or automatic export
  export disabled when no patient is admitted or when no reading exists
  explicit "snapshot" framing and session-local scope in the artifact
  deterministic content based on existing read-only data paths
  trend direction only when sufficient readings exist; otherwise report
  insufficient trend data
  no network send, EMR integration, PDF generation, or hidden persistence in
  this MVP
  authenticated session boundary preserved
  explicit write-success and write-failure handling
- Verification method:
  unit tests for export formatting, deterministic output, insufficient-data
  cases, and identity/alert content
  targeted tests or review evidence for trend-direction wording and gating
  GUI and CLI workflow verification for no-patient, paused session, logout, and
  write-failure cases
  requirements and traceability review before implementation
- Residual risk: low for the pilot if the above controls are implemented.
- Residual-risk acceptability rationale:
  acceptable for the pilot because the feature remains read-only and local, does
  not change clinical logic, and can be clearly framed as a bounded snapshot
  rather than a live or authoritative record.

## hazards and failure modes

- Wrong-patient export caused by session reset, patient refresh, or default demo
  patient confusion.
- Stale snapshot reused after later readings change the patient's condition.
- Exported output omits important context, such as the limited session history,
  missing respiration-rate measurements, or absence of event chronology.
- Textual trend direction is interpreted as a prediction or recommendation
  rather than a bounded summary of local history.
- Silent partial write or file-save failure produces a corrupt or misleading
  artifact.
- Patient data are saved or shared in an insecure location.

## existing controls

- Authentication is required before access to patient data (`SYS-013`,
  `SWR-GUI-001`, `SWR-GUI-002`).
- `patient_print_summary()` already produces deterministic formatted summary
  content from the active patient record.
- `generate_alerts()` already derives active-alert text from the latest reading
  without changing underlying data.
- `trend_extract_*()` and `trend_direction()` already operate on bounded local
  session history with automated tests.
- Session data are local and reset on logout; there is no existing network sync
  or export path for this content.
- Production code uses static memory only, which reduces unexpected allocation
  behavior in any narrow export implementation.

## required design controls

- Add new requirements before implementation so the export artifact is formally
  specified and testable.
- Keep the feature user-initiated and local. Do not add auto-save, auto-send,
  cloud sync, HL7, or EMR behavior in this issue.
- Disable export unless there is an active admitted patient and at least one
  recorded reading.
- Label the artifact as a patient review snapshot and make clear that it is not
  a live dashboard and not a complete event history.
- Include enough context to reduce misidentification and misinterpretation:
  patient identity, reading count, latest values, active alerts, and whether
  trend direction is based on sufficient local history.
- Do not invent authoritative timestamp or chronology semantics in the MVP.
  If wall-clock timestamps are proposed, treat that as a follow-on design topic
  with explicit human review of clock provenance and record semantics.
- Reuse existing deterministic alert and trend logic rather than introducing new
  interpretive rules.
- Surface explicit success and failure feedback for file writes; never fail
  silently.
- Keep the export format simple and locally authored so the team does not copy
  proprietary vendor layouts or imply compatibility with vendor data formats.

## validation expectations

- Add unit tests for export content, field order, deterministic formatting, and
  no-patient/no-reading handling.
- Verify trend-direction export for rising, falling, stable, and insufficient
  history cases using the existing bounded trend helper behavior.
- Verify that missing respiration rate does not create misleading exported trend
  or alert content.
- Verify authenticated-session gating and logout/session-reset behavior so an
  export cannot be generated from a cleared session.
- Verify operator-visible error handling for invalid output paths or write
  failures.
- Review the final design against the new export requirements and related
  summary, GUI, and traceability requirements before implementation begins.

## residual risk for this pilot

Residual pilot risk is low if the implementation remains a local, explicit,
plain-text snapshot export with no new chronology claims, no network behavior,
and clear limits on what the artifact represents.

Residual risk becomes medium if the design drifts toward durable record
semantics, external integration, or predictive wording around trends.

## clinical-safety boundary and claims that must not be made

The feature must not claim or imply:

- diagnosis, treatment advice, or deterioration prediction
- that the artifact is the official patient chart or a complete audit trail
- that the artifact contains a complete event chronology
- that the output is synchronized with external systems or authoritative clocks
- that the export changes alarm logic, NEWS2 logic, or threshold behavior

## MVP boundary that avoids copying proprietary UX

Acceptable MVP boundary:

- one plain-text export path
- one compact locally authored field layout
- reuse of this product's own summary, alert, and bounded trend logic
- no vendor-specific terminology, iconography, or layout cloning

Out of scope for this issue:

- PDF or print workflows
- graphical trend export
- HL7, EMR, or cloud integration
- event markers, transport review, or long-horizon retrospective review
- copycat recreation of vendor review screens

## whether the candidate is safe to send to Architect

Yes, with constraints.

It is safe to send to Architect if the architect treats the feature as a
deterministic, local, user-initiated snapshot export of the current session and
keeps record-grade persistence, timestamp semantics, and external integration
out of scope.

## human owner decision needed, if any

A non-blocking human product/privacy/quality decision is still needed on the
intended status of the exported artifact:

- informal handoff aid only
- durable local note with constrained use
- or something closer to a formal record artifact

That decision does not need to block design for the narrow MVP above.

It does become blocking before implementation if the team wants the export to:

- function as part of the formal medical record
- include authoritative timestamps or chronology claims
- be transmitted, synchronized, or integrated outside the local workstation
