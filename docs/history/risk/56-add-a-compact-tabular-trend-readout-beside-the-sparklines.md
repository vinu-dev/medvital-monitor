# Risk Note: Issue #56

Date: 2026-05-06
Issue: `#56` - Feature: add a compact tabular trend readout beside the sparklines
Branch: `feature/56-add-a-compact-tabular-trend-readout-beside-the-sparklines`

## proposed change

Add a compact, read-only tabular trend readout beside the existing sparkline
tiles so operators can see the exact recent values behind the current trend
shape without leaving the main dashboard.

This candidate is safe to advance only as a presentation-layer change. The MVP
must derive its rows directly from the same bounded in-memory patient history
already used by the sparkline renderer and the existing reading-history list in
`src/gui_main.c`. It must not change measurement capture, patient-record
storage, alert thresholds, alarm-limit checks, NEWS2 scoring, authentication,
or export behavior.

The current product retains at most `MAX_READINGS = 10` samples and the live
simulation path auto-resets the patient record when that buffer is full. Any
"trend table" approved under this issue therefore represents only the current
retained session window, not a complete handoff record, not persistent history,
and not a long-horizon review workflow.

## product hypothesis and intended user benefit

The product hypothesis is plausible for a bounded dashboard usability change:
operators may interpret the sparkline more quickly if the exact recent values
are visible at the same glance point instead of requiring mental reconstruction
from the line shape alone.

The intended benefit is faster review of already displayed measurements during
local monitoring, demo review, and DVT-style inspection. That benefit is
limited by the current storage model. With only ten retained samples and no
timestamp field in the current patient record, this feature supports short,
session-local numeric review only.

## source evidence quality

The cited Mindray operator manual and Philips public marketing page are
acceptable product-discovery evidence that numeric trend review and tabular
trend presentation are common patterns in patient-monitoring interfaces.

Evidence quality is still limited:

- the sources are vendor material, not neutral usability research
- they do not justify copying vendor terminology, layout, or interaction flows
- they do not provide clinical outcome evidence
- they do not justify claims about improved safety, diagnosis, or handoff
  completeness in this repository's current 10-sample design

These sources are sufficient to support a narrow non-copying MVP, not broader
clinical or workflow claims.

## MVP boundary that avoids copying proprietary UX

An acceptable MVP is a small generic row or column table that shows only the
already displayed vital parameters for the current retained sample set beside
the existing sparkline area, using neutral labels and the repository's existing
dashboard visual language.

Not acceptable in this issue:

- vendor-specific trend-review names, layouts, or cursor workflows
- event-position review, deviation bars, or zoom/review panes
- export, print, persistence, or handoff package features
- any design that implies long-horizon or complete-session history beyond the
  current retained buffer
- any separate data model that can diverge from the sparkline or history list

## clinical-safety boundary and claims that must not be made

- Do not claim this feature improves diagnosis, triage, or earlier detection by
  itself.
- Do not claim complete handoff evidence or full-session historical review
  while the system still retains only a bounded 10-sample buffer that resets.
- Do not imply time-based precision that the current data model does not carry.
  Unless a future requirement adds timestamps, the table should present sample
  order or retained-window context, not unsupported elapsed-time claims.
- Do not change alarms, thresholds, aggregate status, NEWS2, or alert
  prioritization based on the table.
- Do not let the table visually replace the current numeric values, color
  badges, or alert banner as the primary decision surface.

## whether the candidate is safe to send to Architect

Yes, with scope narrowing.

The candidate is safe to send to Architect only as a deterministic,
session-local, read-only dashboard aid over the current retained patient
history. If the design expands toward persistent history, explicit handoff
artifacts, exported review tables, timestamped audit claims, or new clinical
interpretation semantics, it needs new requirements and another risk pass
before implementation.

## medical-safety impact

Direct medical-safety impact is low but not zero. The proposed feature changes
how existing values are emphasized and compared, not how they are measured or
classified.

The main hazard is interpretive. A compact table can create false confidence if
the active retained window, ordering, or session boundary is unclear. It also
introduces a second exact-value surface near the sparkline, which can mislead a
user if it becomes inconsistent with the lower reading-history list or if the
user mistakes a retained subset for the complete available record.

This candidate does not add, remove, or depend on any AI or ML model.

## security and privacy impact

No new patient-data category, network path, external service, credential flow,
or role behavior is needed for the bounded MVP. Privacy impact is therefore
none expected if the feature remains a local in-memory dashboard surface only.

The main privacy and integrity risks are scope creep and duplicate display
state:

- a denser on-screen summary can expose more exact values at once if the screen
  is visible to unauthorized bystanders
- a separate cached table model could display stale or mismatched values after
  admit, clear-session, logout, or buffer rollover
- export or persistence expansion would change the privacy and traceability
  profile materially and is out of scope for this issue

## affected requirements or "none"

Existing approved requirements already touch the affected behavior:

- `UNS-009` vital-sign history
- `UNS-010` consolidated status summary
- `UNS-014` graphical dashboard
- `SYS-009` vital-sign reading history
- `SYS-011` patient status summary display
- `SYS-014` graphical vital-signs dashboard
- `SWR-GUI-003` color-coded vital-sign display
- `SWR-PAT-003` latest reading access
- `SWR-PAT-006` patient summary display
- `SWR-TRD-001` sparkline and trend extraction behavior

No new user need is obviously required if the change stays display-only, but a
new or amended SYS/SWR requirement is likely needed to define the compact
tabular readout's ordering, retained-window semantics, reset behavior, and the
rule that it does not alter stored readings or clinical calculations.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- support trained clinical users reviewing exact recent values for the active
  patient on the existing live dashboard

User population:

- bedside clinicians
- ward staff
- authorized admin or clinical users already permitted to use the dashboard

Operating environment:

- Windows workstation dashboard
- local in-memory patient session
- bounded 10-sample retained record
- simulation mode with automatic rollover reset, plus manual-entry scenarios

Foreseeable misuse:

- assuming the compact table is the complete session history
- reading the ordering backward and mistaking oldest for newest
- interpreting sample rows as precise elapsed time despite no current timestamp
  model
- relying on the table instead of the current alert state or latest numeric tile
- mistaking a retained subset for the same scope as another history surface
- carrying stale table context across admit, clear-session, logout, or
  auto-reset boundaries

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity:

- moderate, because misreading recent numeric trend context could delay
  recognition of deterioration even though the underlying calculations remain
  unchanged

Probability:

- low to occasional without controls; the feature is display-only, but ordering
  ambiguity or stale state would make misuse credible

Initial risk:

- medium

Risk controls:

- derive the table directly from `g_app.patient.readings` and
  `g_app.patient.reading_count`, with no independent copy that can drift from
  the sparkline or history list
- make the retained-window scope and row ordering continuously visible next to
  the table
- if fewer than ten samples are available, show only the available retained
  rows; do not imply hidden values
- reset all table state on login, logout, admit/refresh, clear-session, and
  automatic rollover reset when the retained buffer is full
- avoid unsupported timestamp or duration claims unless future requirements add
  real time metadata
- show `RR` not-measured entries truthfully, consistent with existing dashboard
  and alert behavior
- keep the table visually secondary to the current tile values, status badges,
  and alert banner
- keep the implementation deterministic and static-memory only

Verification method:

- targeted unit coverage for any helper that maps retained readings into table
  order and visible subset
- manual GUI smoke review with zero, one, partial, and full retained samples
- manual review across clear-session, admit/refresh, logout, and automatic
  rollover reset boundaries
- regression check that alerts, NEWS2, alarm behavior, and current status do
  not change when only the tabular readout is added

Residual risk:

- low if the above controls are followed

Residual-risk acceptability rationale:

- acceptable for this pilot because the feature remains a passive review aid,
  the primary current-value and alert surfaces remain available, and the main
  remaining hazard is bounded to human interpretation rather than clinical
  computation

## hazards and failure modes

- the compact table is read as a complete session or handoff record
- row ordering is unclear and the operator mistakes old values for the newest
  state
- the table implies precise time spacing even though current data is only a
  retained sample sequence
- the table diverges from the sparkline or existing reading-history list
- stale rows survive a patient/session reset and are interpreted as current
- dense table content crowds the existing alert or value surfaces on smaller
  window sizes
- future scope creep adds export, persistence, or workflow claims without a new
  safety review

## existing controls

- the current dashboard already shows primary exact values, color-coded status
  tiles, and an aggregate alert banner
- `update_dashboard()` already populates a retained reading-history list from
  the active patient record
- the sparkline renderer already derives from the same bounded patient-history
  buffer through `trend_extract_*()` helpers
- `patient_add_reading()` enforces the `MAX_READINGS` cap
- the live simulation path already defines a rollover reset when the retained
  buffer becomes full
- the issue body explicitly keeps thresholds, alerts, and clinical logic out of
  scope

## required design controls

- define one authoritative retained-data source for sparkline, compact table,
  and reading-history list behavior
- make newest-versus-oldest ordering explicit in the design and the UI
- label the table in terms that are true for the current implementation, such
  as retained samples or current retained window
- keep the table read-only and session-local; no export, print, persistence, or
  audit-trail expansion in this issue
- ensure the compact table does not reduce the prominence of the current value
  tiles, alert banner, or active alerts list
- keep user-facing wording generic and avoid vendor-specific review semantics or
  marketing language
- if future design wants true time labels, persistent history, or handoff
  evidence claims, stop and route that to new requirements instead of hiding it
  in this issue

## validation expectations

- confirm the changed file list stays limited to dashboard presentation code,
  tests, and the design/requirements artifacts needed for the feature
- add automated checks for any new table-ordering or retained-subset helper
- manually verify behavior with simulation off, no patient admitted, RR not
  measured, and full-buffer rollover
- manually verify that the table resets correctly when the patient record is
  reinitialized
- confirm the final UX text does not claim persistent, complete, or timestamped
  history that the current system does not support

## residual risk for this pilot

Low after the above controls.

The remaining risk is that users may still over-read a compact review table as
more authoritative or more complete than it really is. That residual risk is
acceptable for this pilot only if the table is clearly bounded to the current
retained session context and kept secondary to the approved current-value and
alerting surfaces.

## human owner decision needed, if any

The product owner should explicitly decide whether this issue is:

- only a bounded exact-value companion to the current retained sparkline data,
  or
- the start of a broader trend-review or handoff-history capability

The product owner should also approve the user-facing terminology for retained
sample scope and ordering. Claims such as full-session review, handoff-ready
history, or exact time-context review are too strong for the current data model
unless new requirements add persistence and timestamp semantics first.
