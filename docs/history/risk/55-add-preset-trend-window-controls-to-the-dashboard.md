# Risk Note: Issue 55

Issue: `#55`  
Branch: `feature/55-add-preset-trend-window-controls-to-the-dashboard`

## proposed change

Add a small preset trend-window control to the live dashboard so an operator can
switch the sparkline strip between a recent subset and the full currently
retained trend buffer without leaving the main screen. The intended
implementation should reuse the existing trend extraction helpers in
`src/trend.c` and the existing sparkline painting path in `src/gui_main.c`.

This approval is intentionally narrow. The current product stores only
`MAX_READINGS = 10` samples per patient record and the simulated live dashboard
auto-resets that record when the buffer becomes full. In this MVP, "full
history" can only mean the full currently retained 10-sample buffer, not
long-horizon review, not persistent history, and not a complete handoff record.

## product hypothesis and intended user benefit

The product hypothesis is plausible for a bounded dashboard UX improvement:
operators may interpret sudden excursions versus short drift more quickly if
they can view the same retained sparkline data through a shorter or longer
display window. That can improve glanceability during demo review and local
monitoring without changing the underlying clinical calculations.

The benefit is limited by the current storage model. With a 2-second simulated
update cadence and a 10-sample auto-reset buffer, this feature can plausibly
support short-interval visual comparison only. It does not yet support a strong
handoff or longer-drift review claim.

## source evidence quality

The external sources cited in the issue are adequate for product-discovery
context but not for safety or efficacy claims:

- Philips Horizon Trends public marketing material describes configurable trend
  views ranging from short intervals to longer graphical trends.
- The Mindray T1 operator manual describes a review window with configurable
  zoom length and cursor navigation for graphic trends.

Evidence quality is moderate for showing that configurable trend windows are a
common class of monitor UX, and low for justifying any clinical performance
claim. These vendor sources are sufficient to define a narrow non-copying MVP,
but they do not justify copying terminology, layout, or interaction patterns,
and they do not prove improved patient outcomes.

## medical-safety impact

This is a presentation-only change if kept in scope. It does not need to alter
vital acquisition, alert thresholds, NEWS2 scoring, alarm-limit evaluation, or
patient-record write behavior.

The safety risk is interpretive rather than computational. Changing the visible
window can make the same retained data look steeper, flatter, shorter, or more
stable depending on the selected scope. A user can therefore misread the
displayed context, especially if a short window is mistaken for the complete
available record or if "full history" wording overstates what the system
actually retains before auto-reset.

This candidate does not add, remove, or depend on any AI/ML model. The proposed
control is deterministic UI state only.

## security and privacy impact

No new patient data classes, network paths, credential flows, or external
services are required for the bounded MVP. Privacy impact is therefore none
expected if the feature remains a local dashboard control.

The main security and integrity risk is stale or cross-context UI state. If the
selected window persists across logout, patient refresh, clear-session, or the
automatic full-buffer reset, an operator may view a new patient or a new cycle
through an old display context and misinterpret what is on screen. The control
should therefore remain session-local, non-persistent, and reset with the
patient/session lifecycle.

## affected requirements or "none"

Existing approved requirements already touch the affected behavior:

- `UNS-009` vital-sign history
- `UNS-010` consolidated single-screen summary
- `UNS-014` graphical dashboard
- `SYS-014` graphical vital-signs dashboard
- `SWR-GUI-003` colour-coded vital-sign display
- `SWR-TRD-001` sparkline and trend extraction behavior
- `SWR-PAT-003` latest reading access
- `SWR-PAT-006` patient summary display

No new user need is obviously required if the change stays display-only, but a
new or amended SYS/SWR requirement is likely needed to define a user-selectable
trend review window, its reset behavior, and the rule that it does not affect
alerts, status, or stored readings.

## intended use, user population, operating environment, and foreseeable misuse

- Intended use: trained clinical staff use the dashboard sparkline control to
  compare recent versus currently retained trend context for the active patient
  during a local monitoring session.
- User population: bedside clinicians and ward/admin users already authorized
  to use the dashboard; not patient-facing.
- Operating environment: Windows workstation dashboard, local session state,
  bounded 10-sample in-memory record, 2-second simulated acquisition cadence in
  simulation mode.
- Foreseeable misuse: selecting a short window and assuming no earlier
  deterioration exists; selecting a longer view and missing a sudden recent
  excursion; interpreting the retained 10-sample buffer as a complete session
  history; relying on the sparkline window instead of current numeric values or
  alarms; carrying the same window choice across patient/session boundaries.

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: moderate. If a user misreads trend context, the most credible harm
  is delayed recognition of deterioration or overconfidence in apparent
  stability. Primary numeric values and alert states remain visible, which
  limits direct harm severity versus a computational defect.
- Probability: low to occasional without controls. The feature changes only
  visual context, but ambiguous labels or sticky state would make misuse more
  likely.
- Initial risk: medium for a medical-display UX change because the hazard is
  interpretation, not raw calculation.
- Risk controls:
  - Default to the full currently retained buffer whenever a patient is
    admitted/refreshed, a session is cleared, a user logs in, or the
    auto-reset-at-full condition occurs.
  - Display the selected window state plainly next to the sparkline or tile
    group using wording that does not imply more retained history than exists.
  - Keep the control read-only with respect to stored measurements, alerts,
    NEWS2, and alarm logic.
  - If fewer points are available than the selected preset expects, show the
    available retained data only; do not synthesize or imply hidden points.
  - Keep implementation deterministic, static-memory, and session-local.
- Verification method:
  - Unit coverage for window-selection bounds and extraction behavior over
    partial and full buffers.
  - GUI smoke review that toggles presets with zero, partial, and full sample
    counts and across the automatic reset boundary.
  - Regression check that alert banner, tile status, NEWS2, and patient data do
    not change when only the sparkline window selection changes.
- Residual risk: low if the above controls are implemented.
- Residual-risk acceptability rationale: acceptable for this pilot only as a
  bounded display aid because the feature does not change the clinical
  computation path, the primary current values and alerting remain visible, and
  the remaining hazard can be reduced to a clearly labelled UX limitation.

## hazards and failure modes

- A short preset hides earlier retained deterioration and the operator mistakes
  the displayed subset for the full available record.
- A longer preset compresses recent variation and the operator misses a sudden
  excursion that is still clinically visible in the numeric tiles.
- "Full history" wording implies complete session or handoff coverage even
  though the current implementation retains only 10 samples before reset.
- The selected window survives patient/session reset and applies stale context
  to a new monitoring cycle.
- Window-selection logic uses wrong sample bounds or off-by-one indexing and
  displays a different subset than the label indicates.
- The added control crowds or distracts from the primary alert/status surfaces
  on the minimum supported dashboard size.

## existing controls

- Current trend rendering already uses bounded extraction helpers and static
  arrays (`MAX_READINGS`) rather than dynamic memory.
- The issue explicitly keeps thresholds, NEWS2, alarm logic, and acquisition
  out of scope.
- The dashboard already presents primary numeric values, colour-coded status
  tiles, and aggregate alert context independently of the sparkline strip.
- `patient_add_reading()` enforces a hard capacity limit and the live dashboard
  already has defined reset behavior when the patient buffer is full.

## required design controls

- Define presets in terms that are true for the current implementation, such as
  "Recent" versus "All retained", rather than long-duration claims copied from
  competitor products.
- Make the selected preset continuously visible near the sparkline region.
- Reset the selected preset on login, admit/refresh, clear-session, logout, and
  the automatic full-buffer reset path.
- Preserve the current alert/banner/value layout as the primary clinical
  surface; the trend-window control must remain secondary.
- Ensure the control does not add persistence, export, print, cursor review,
  network sync, or other workflow expansion in this issue.
- Avoid copying vendor-specific naming such as Horizon Trends, Zoom review
  pages, deviation bars, cursor workflows, or proprietary long-horizon review
  patterns.

## validation expectations

- Confirm the changed file list stays limited to dashboard/trend presentation
  code, tests, and requirements/design artifacts needed for the new control.
- Add automated checks for preset-to-buffer mapping and reset behavior at the
  `MAX_READINGS` rollover boundary.
- Perform a manual GUI review in simulation mode to verify that toggling the
  preset changes only the sparkline scope and label, not the clinical status
  outputs.
- Confirm the chosen user-facing terminology does not claim persistent,
  complete, or long-horizon history that the current system cannot provide.

## MVP boundary that avoids copying proprietary UX

An acceptable MVP is a generic 2- or 3-preset selector over the current
retained sparkline data only, with neutral labels and no vendor-derived layout
or workflow. Examples include a short recent subset versus all retained points,
provided the terminology reflects actual retained data.

Not acceptable in this issue: copying Philips or Mindray product names,
time-range menus, cursor review panes, deviation bars, event review markers,
print/export review workflows, or any feature that implies persistent historical
storage beyond the current patient buffer.

## clinical-safety boundary and claims that must not be made

- Do not claim improved diagnosis, triage, or earlier detection from this UI
  change alone.
- Do not claim full-session, handoff-complete, or long-drift review while the
  system still retains only a 10-sample auto-reset buffer.
- Do not change alarms, thresholds, NEWS2, alert prioritization, or aggregate
  patient status based on the selected window.
- Do not hide the current numeric values or make the sparkline window appear to
  be the primary decision surface.

## whether the candidate is safe to send to Architect

Yes, with scope narrowing. The candidate is safe to send to Architect only as a
deterministic, session-local, display-only control over the current retained
trend buffer. If design expands toward persistent history, longer review
windows, handoff completeness, or workflow claims beyond the 10-sample buffer,
it needs new requirements and another risk pass before implementation.

## residual risk for this pilot

Residual risk is low for the narrowed MVP. The remaining risk is that users may
still over-read a presentation aid as a complete historical view, but that risk
is manageable if the display is clearly labelled, reset correctly, and kept
secondary to the current values and alert states.

## human owner decision needed, if any

The product owner should explicitly decide whether issue `#55` is:

- only a bounded dashboard selector over the existing 10-sample retained buffer
  for pilot/demo use, or
- the start of a separate longer-horizon trend-history capability that will
  require new storage, new requirements, and a broader safety review.

The user-facing wording should be approved accordingly. "Full in-session
history" is too strong unless the product team intends only "all currently
retained samples" in this release.
