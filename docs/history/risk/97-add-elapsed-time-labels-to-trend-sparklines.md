# Risk Note: Issue #97 - Add elapsed-time labels to trend sparklines

Date: 2026-05-06
Issue: #97 "Feature: add elapsed-time labels to trend sparklines"
Branch: `feature/97-add-elapsed-time-labels-to-trend-sparklines`

## Proposed change

Add bounded recency markers to the dashboard trend sparklines so clinicians can
read both shape and recency without counting points manually. For this pilot,
the safe MVP is a display-only enhancement that either:

- shows truthful elapsed-time labels only when the underlying history carries a
  validated time basis for every displayed point, or
- falls back to explicit reading-order markers when the app only knows sample
  order

Current repo constraints matter:

- `VitalSigns` stores physiologic values only; it has no timestamp field.
- `PatientRecord` keeps up to `MAX_READINGS` (10) sequential samples.
- `paint_sparkline()` in `src/gui_main.c` renders a bare polyline from ordered
  history values.
- The app mixes auto-generated simulation samples on a 2-second timer with
  manual user-entered readings through `do_add_reading()`.

Because of that mixed input model, a label that implies real minutes or hours
would be unsafe unless the data model or mode semantics are tightened first.

## Product hypothesis and intended user benefit

Hypothesis: adding recency context to the existing sparkline helps clinicians
scan deterioration and recovery faster because they can judge whether the trend
shape reflects the latest few samples or older session history.

Expected user benefit:

- less manual counting of sparkline points
- faster recognition of whether a change is recent or older within the current
  session
- better usability of the existing sparkline without redesigning the dashboard

## Source evidence quality

Evidence quality is adequate for product-discovery scope and weak for any
clinical-effectiveness claim.

- The linked Philips Screen Trends product page states that trend data and
  current data are shown together and that aligned gridlines and time scales
  help evaluate measurement trends. This is relevant evidence that recency
  context is a real bedside-monitor workflow need, but it is vendor marketing
  rather than independent human-factors or safety evidence.
  Source: https://www.usa.philips.com/healthcare/product/HCNOCTN175/screen-trends-patient-monitoring---decision-support
- Philips Horizon Trends marketing also describes trend indicators over defined
  recent windows and optional longer graphic trends. This supports the product
  hypothesis that time context matters, but it does not justify copying the UX
  or claiming equivalent timing accuracy.
  Source: https://www.usa.philips.com/healthcare/product/HCNOCTN172/horizontrendstrendmeasurementdisplay

Conclusion: the evidence is sufficient to justify a narrow, non-copying pilot
MVP. It is not sufficient to justify precise elapsed-time claims without a
validated local time basis.

## Medical-safety impact

This feature does not change thresholds, alert generation, NEWS2 scoring, or
treatment guidance. The safety risk is entirely in presentation semantics.

Primary safety benefit:

- clinicians can better judge whether a visible rise or fall happened recently
  within the retained session window

Primary safety risks:

- a label can imply false recency if the app shows minutes or hours without
  storing actual timestamps
- mixed manual and simulation readings can make equal spacing look clinically
  meaningful when it is not
- extra labels can crowd tiles and obscure the current numeric value or status
  badge
- rollover reset can make the trend appear continuous when earlier session
  samples were cleared

Overall medical-safety impact: low if the feature stays display-only and uses
truthful time semantics; not acceptable if the UI implies real elapsed time that
the data model cannot support.

## Security and privacy impact

Security and privacy impact is limited.

- No new clinical algorithm, network path, or external integration is required.
- If the design remains reading-order based, it adds no new protected data
  category beyond what the dashboard already shows.
- If real timestamps are introduced later, they remain local session metadata
  and must stay behind the existing authenticated workstation boundary.
- This MVP should not add exports, durable logs, or cross-session retention.

## Affected requirements or "none"

Existing requirements likely affected:

- `UNS-009` Vital Sign History
- `UNS-014` Graphical User Interface
- `SYS-009` Vital Sign Reading History
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-TRD-001` Trend Sparkline and Direction Detection
- `SWR-GUI-003` Colour-Coded Vital Signs Display
- `SWR-GUI-004` Patient Data Entry via GUI

New derived requirements are likely needed for:

- the approved source of time context for each displayed point
- fallback behavior when timestamps are unavailable or mixed-mode history exists
- rollover/reset disclosure when the visible sparkline no longer represents the
  earlier session
- readability limits so labels do not hide current values or alert state

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinicians use the sparkline as an adjunct visual cue for recent
  within-session trend context on the local dashboard

User population:

- bedside clinical staff and internal testers using the Windows workstation app

Operating environment:

- local authenticated Win32 dashboard in the current pilot, with both
  simulation and manual-entry workflows

Foreseeable misuse:

- interpreting sample-order markers as actual elapsed minutes
- treating equal point spacing as proof of equal acquisition intervals
- relying on sparkline labels instead of the current numeric reading and alert
  banner
- assuming the view is a complete longitudinal chart rather than a bounded
  session display

## MVP boundary that avoids copying proprietary UX

The MVP should remain narrow and implementation-led:

- no waveform overlays, central-station layouts, or multi-parameter trend
  review screens
- no mimicry of Philips gridline, color, or bedside-monitor compositions
- no multi-hour or clinically precise time scale unless the product first adds
  validated timestamp support
- allow a reading-index or relative-order marker design if the current model
  remains unchanged
- keep the sparkline a local, bounded, session-only aid tied to the existing
  10-reading limit

## Clinical-safety boundary and claims that must not be made

The feature may support quicker visual review of recent samples. It must not
claim to:

- show true elapsed minutes or hours when the displayed points do not have an
  explicit validated time basis
- provide chart-grade timing accuracy for manual-entry histories
- diagnose deterioration rate or recommend intervention
- replace the current numeric values, live alert surfaces, or formal patient
  charting
- preserve a complete history beyond the current bounded session

## Whether the candidate is safe to send to Architect

Yes, with constraints. The candidate is safe to send to Architect if design
treats time semantics as the primary safety constraint:

- if the implementation keeps the current data model, it should use reading
  order or a clearly relative recency cue rather than real elapsed-time claims
- if the product requires actual elapsed-time labels, Architect must first add
  a trustworthy per-reading time basis and define how manual-entry and
  simulation modes differ

It is not safe to design a minute- or hour-based label system on top of the
current untimestamped mixed-mode history.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: false recency could distort clinician understanding of how quickly a patient changed, but the feature does not change alerting or treatment logic. |
| Probability | Possible without controls because the current model stores only sample order and mixes simulation with manual entry. |
| Initial risk | Medium |
| Key risk controls | Use truthful time semantics only; prefer reading-order markers when timestamps are absent; keep labels visually secondary; preserve prominence of current values and alert state; disclose session rollover/reset boundaries; keep the feature read-only and outside alert logic. |
| Verification method | Unit tests for any new label-format or time-basis helper logic; GUI/manual verification in both simulation and manual-entry flows; boundary checks at 1, 2, and 10 readings; reset/rollover review; traceability updates for any new requirements. |
| Residual risk | Low if controls are implemented and verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the feature remains an adjunct visualization aid and can be constrained to honest, bounded, session-local recency cues. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| False recency is displayed | Untimestamped samples are labeled as elapsed minutes or hours | Clinician misjudges speed of deterioration or recovery |
| Mixed-mode history looks uniform | Manual-entry and timer-driven samples are rendered with the same implied spacing | User over-trusts the spacing as clinically meaningful |
| Current state is visually obscured | Added labels clutter the tile and hide the latest value or status badge | Recent abnormal status is harder to detect quickly |
| Session boundary is hidden | Automatic reset after buffer rollover clears earlier samples without enough disclosure | User assumes the displayed trend spans more history than it does |
| Precision is over-claimed | UI copy implies verified timing rather than relative order | User treats the sparkline as chart-grade evidence |

## Existing controls

- `PatientRecord` already uses fixed-size static storage with a 10-reading cap.
- The current sparkline is read-only and derived from the same ordered history
  used elsewhere in the dashboard.
- The latest numeric values and alert state already remain primary UI elements.
- The simulation path already has a defined 2-second timer cadence.
- Session reset paths already exist when the buffer rolls over or the user
  clears/logs out.

These controls reduce implementation risk, but they do not solve truthful
elapsed-time labeling on their own.

## Required design controls

- Define one approved time-semantic policy per mode:
  actual timestamps, fixed-cadence elapsed time, or sample-order markers.
- Do not display minute/hour labels for histories that lack a validated time
  basis.
- For manual-entry histories, use explicit order markers unless and until the
  product records actual acquisition time.
- Keep labels short, bounded, and visually secondary to the current value and
  status badge.
- Distinguish the newest point explicitly if a label is shown.
- Disclose session-reset boundaries whenever earlier sparkline history has been
  cleared by rollover or session reset.
- Keep all timing cues read-only and outside alarm generation, NEWS2, and other
  clinical calculations.
- Update requirements and traceability before implementation if new time
  semantics are introduced.

## Validation expectations

- unit tests for any helper that selects between elapsed-time and sample-order
  labeling
- unit tests if timestamps or per-reading timing metadata are added to the data
  model
- GUI/manual verification for simulation-only history, manual-entry history,
  and mixed workflows after admit/clear/reset
- review at tile-width extremes and full 10-point history so labels remain
  readable and do not occlude primary status information
- verification that rollover/session reset does not silently imply a longer
  retained trend than actually exists
- traceability updates for any changed or new UNS/SYS/SWR items

## Residual risk for this pilot

Residual risk remains that some users may over-interpret any trend label as more
precise than it really is. For this pilot, that is acceptable only if the UI
uses honest semantics, keeps the cue secondary, and avoids claiming real
elapsed time without supporting data.

## Human owner decision needed, if any

Product owner and Architect should explicitly decide:

- whether the MVP is limited to sample-order markers on the current data model
- whether real elapsed-time labels are worth the added design and verification
  scope of storing trustworthy per-reading timing data
- what the manual-entry path should show when users add readings at arbitrary
  times
- how session rollover/reset is disclosed when older sparkline history is no
  longer present
