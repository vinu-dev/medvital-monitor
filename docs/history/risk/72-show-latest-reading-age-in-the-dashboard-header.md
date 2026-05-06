# Risk Note: Issue 72

Date: 2026-05-06
Issue: `#72` - Feature: show latest-reading age in the dashboard header
Branch: `feature/72-show-latest-reading-age-in-the-dashboard-header`

## proposed change

Add a passive freshness cue to the live dashboard header or patient bar that
shows the age or timestamp of the latest accepted reading. The intended scope is
presentation-only metadata on top of the existing `patient_latest_reading()`
and dashboard refresh path.

The change must not alter alert thresholds, NEWS2 scoring, missing-reading
handling, alarm escalation, hardware acquisition behavior, or treatment
guidance. It must remain a read-only context aid for already accepted data.

## product hypothesis and intended user benefit

Operators can currently see live values and whether simulation is live or
paused, but they cannot tell at a glance how old the displayed reading is.
Adding a small deterministic freshness cue may reduce the chance that a paused
or delayed screen is mistaken for current data.

The user benefit is ergonomic and situational-awareness focused, not clinical
decision automation. It improves confidence about display recency without
changing what the system classifies as normal, warning, or critical.

## source evidence quality

Source evidence quality is moderate for product-discovery precedent and low for
clinical or safety justification.

- The issue cites public Mindray operator-manual material showing time-oriented
  trend review or trend-table displays. Search-indexed manual excerpts confirm
  that time context on patient monitors is a normal UX pattern.
- Those sources are still untrusted product-discovery context only. They do not
  justify copying layout, terminology, or claims, and they are not clinical
  evidence for safety effectiveness.
- Repo evidence is stronger for current-state fit: `README.md` and
  `include/hw_vitals.h` describe an approximately 2-second acquisition cadence,
  `src/gui_main.c` already exposes `* SIM LIVE` and `SIM PAUSED`, and the
  current `VitalSigns` / `PatientRecord` structures do not contain timestamp
  metadata.

Conclusion: there is enough public and repo-local evidence to support a narrow,
non-copying MVP for a freshness cue.

## medical-safety impact

Medical-safety impact is low to moderate.

The proposed feature can reduce an existing workflow ambiguity by helping a
human notice that displayed values are not recent. That is safety-positive.
However, if the cue is wrong or misleading, it can create false reassurance and
contribute to delayed recognition of stale data.

The risk remains bounded because the feature does not change thresholds, alert
generation, NEWS2, or patient-record contents if the scope is contained.

## security and privacy impact

No security-control or privacy-boundary change is expected if freshness is
derived locally from the accepted-reading event and not persisted or exported.

Main integrity risks:

- deriving display time from an ambiguous wall clock instead of accepted-reading
  state
- implying time synchronization, device connectivity health, or data validity
  that the system does not actually verify

Privacy impact is none expected because no new patient fields, user data,
credentials, or network flows are required for the MVP.

## affected requirements or "none"

Adjacent existing requirements:

- `UNS-014` Graphical User Interface
- `UNS-015` Live Monitoring Feed
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-003` Colour-Coded Vital Signs Display
- `SWR-GUI-005` Hardware Abstraction Layer Interface
- `SWR-GUI-006` Simulated Vital Signs Data Feed

No new clinical-behavior requirement is needed if the implementation remains a
passive display-only cue. If the design adds stale-data alarm semantics,
connectivity claims, or workflow gating, requirements must be updated before
implementation.

## intended use, user population, operating environment, and foreseeable misuse

Intended use: help trained users of the Windows dashboard judge whether the
currently displayed reading is recent enough to treat as the latest accepted
sample.

User population: trained clinical operators and administrators using the pilot
monitoring dashboard.

Operating environment: a Windows workstation running the Win32 GUI in
simulation mode today, with a future HAL-backed real-device mode possible.

Foreseeable misuse:

- treating the freshness cue as an alarm or safety interlock
- assuming the cue proves sensor connectivity or device-clock synchronization
- interpreting "fresh" as clinically stable rather than merely recently updated
- assuming paused or cleared sessions still represent current patient state

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: low to medium. A misleading freshness cue could contribute to a
  delayed human response to stale data, but it cannot directly change clinical
  classification if kept presentation-only.
- Probability: low, provided the cue is derived from the accepted-reading event
  and explicitly handles paused/no-data states.
- Initial risk: low-medium.
- Risk controls: derive freshness from the accepted-reading boundary rather than
  repaint timing; use neutral wording such as `Last update` instead of
  diagnostic language; distinguish no-reading, live, paused, and stale states;
  avoid reusing alarm colors or claims; keep static-memory behavior; do not
  change alerting or NEWS2 logic in the same work item.
- Verification method: targeted unit tests if a helper computes age/state;
  manual GUI verification for first-reading, steady live updates, paused feed,
  clear-session reset, full-buffer reset, and simulation/device-mode behavior;
  regression confirmation that alert outputs and NEWS2 are unchanged.
- Residual risk: low.
- Residual-risk acceptability rationale: with explicit non-clinical semantics
  and correct paused/no-data handling, the feature reduces ambiguity in the
  pilot dashboard and does not expand automated clinical decision-making.

## hazards and failure modes

- The cue refreshes from UI repaint cadence instead of the accepted-reading
  event, making stale data appear current.
- The cue continues to look "fresh" while the feed is paused or after the
  patient session is cleared.
- An absolute timestamp is shown without a defined provenance, locale, or clock
  policy, misleading users about real acquisition time.
- The implementation uses alert-like colors or wording that can be confused
  with warning/critical clinical status.
- The issue grows into stale-data alarm logic, workflow blocking, or device
  connectivity assertions without approved requirements.
- Designers copy competitor layout or interaction patterns too closely instead
  of implementing a repo-native cue.

## existing controls

- The issue body explicitly constrains the change to presentation-only
  metadata, separate from alarms, NEWS2, and interpretation.
- `src/gui_main.c` already distinguishes `* SIM LIVE` from `SIM PAUSED`,
  which gives the design an existing state signal to preserve.
- `include/hw_vitals.h` and the README document an approximately 2-second
  refresh cadence, so the product already has a stable concept of update
  frequency.
- `patient_latest_reading()` provides a single latest-reading access path.
- `VitalSigns` and `PatientRecord` currently have no timestamp member, which
  forces an explicit design decision instead of silently reusing nonexistent
  acquisition-time metadata.
- Production code already follows static-memory constraints, reducing the risk
  of an ad hoc state-management expansion.

## required design controls

- Record freshness at the accepted-reading boundary, not in `WM_PAINT` or other
  repaint-only code.
- Prefer an age-based MVP such as `Updated 4 s ago` over an absolute timestamp
  unless clock provenance and formatting rules are explicitly defined.
- Keep wording passive and non-clinical. Good examples: `Last update`,
  `Updated 4 s ago`, `Feed paused`. Bad examples: `Data valid`, `Patient OK`,
  `Sensor connected`.
- Distinguish at least four states: no reading yet, live recent reading,
  paused/frozen feed, and stale/no new reading beyond the agreed threshold.
- Keep freshness styling visually separate from the green/amber/red clinical
  alert semantics already used elsewhere in the dashboard.
- Do not add persistence, export, networking, or audit claims as part of this
  MVP.
- Do not change alert thresholds, aggregate status, NEWS2, or missing-reading
  policy in the same issue.

## MVP boundary that avoids copying proprietary UX

Implement one small repo-native freshness cue in the existing header or patient
bar, using the current typography and layout language of the Win32 dashboard.

Acceptable MVP examples:

- `Last update: 4 s ago`
- `Last reading: 14:32:08`
- `Feed paused - last update 18 s ago`

Out of scope:

- recreating competitor trend-table or trend-review layouts
- cloning vendor terminology, iconography, alarm treatments, or navigation
- introducing history browsers, cursor-driven time review, or printed trend
  workflows

## clinical-safety boundary and claims that must not be made

This feature must not:

- claim the patient is clinically stable, safe, or improving
- claim the feed is synchronized to a trusted external clock
- claim the sensor or hardware link is healthy
- suppress, escalate, or reinterpret alarms
- replace explicit missing-data, device-fault, or clinician-review workflows

It is a freshness cue only.

## validation expectations

- Inspect the changed file list and diff to confirm presentation-only scope.
- If new state is added, confirm it remains static-memory safe and bounded.
- Manually verify: no-reading state, first accepted reading, steady live
  refresh, paused feed, clear session, full-buffer rollover/reset, and
  simulation/device-mode transitions.
- Confirm the cue changes only when a reading is accepted or when an explicit
  paused/no-data state change occurs.
- Confirm alert tiles, alert list, aggregate status, and NEWS2 output are
  unchanged by the feature.
- If an absolute timestamp is chosen instead of age-only wording, verify the
  chosen format, locale behavior, and clock provenance explicitly.

## residual risk for this pilot

Residual risk is low. The main remaining risk is user overtrust if wording,
state handling, or visual treatment is sloppy. A bounded passive cue with
clear paused/no-data semantics is acceptable for this pilot.

## human owner decision needed, if any

Yes. A human product/design owner should choose and document one approved
semantic before design proceeds:

1. age-only display (`Updated X sec ago`) - preferred for MVP
2. absolute timestamp (`Last reading 14:32:08`)
3. both age and timestamp

If the team wants a visual stale threshold, the owner must also approve the
threshold and confirm that it is not an alarm or treatment recommendation.

## whether the candidate is safe to send to Architect

Yes, with the controls above. This issue is safe to send to Architect as a
passive freshness-metadata feature.

It is not safe to expand this issue into alarming, device-connectivity claims,
clock-synchronization claims, or other clinical workflow changes without new
requirements and explicit human approval.
