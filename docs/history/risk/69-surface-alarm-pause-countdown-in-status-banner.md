# Risk Note: Issue 69

Issue: `#69`  
Branch: `feature/69-surface-alarm-pause-countdown-in-status-banner`

## proposed change

Add a read-only countdown indicator to the existing dashboard status banner so
the operator can see when a temporary alarm-audio pause will expire. The issue
scope is display-only:

- show paused state only while the alarm pause is active
- show the remaining pause time in the existing banner
- clear the indicator immediately when the pause ends or alarms are resumed
- do not change alarm thresholds, alarm generation, escalation, persistence,
  acknowledgement, or alarm-limit configuration behavior

## product hypothesis and intended user benefit

The product hypothesis is credible and narrow. Operators already rely on the
dashboard banner for at-a-glance system state, and a visible pause countdown
should reduce ambiguity about whether alarm audio is still muted and for how
long. The expected user benefit is lower risk of assuming a paused alarm has
already resumed or of forgetting that a temporary pause is still active.

## source evidence quality

Source evidence quality is moderate for UX precedent and low for clinical
effectiveness claims:

- the Philips Telemetry Monitor 5500 instructions for use describe an alarm
  banner that displays an alarms-paused message together with a countdown timer
- the GE B125/B105 clinical reference guide documents a two-minute audio-pause
  state and alarm-pause visual signaling

These sources are useful only as untrusted product-discovery context showing
that pause-state visibility is a common monitor pattern. They are not clinical
evidence, do not define this product's requirements, and do not justify copying
vendor wording, layout, icons, or ancillary behaviors.

## MVP boundary that avoids copying proprietary UX

The MVP boundary should stay inside the current MedVital status banner and
surface only the minimum state needed for safe review:

- use the existing banner region instead of introducing vendor-like overlays,
  dimming, waveform treatments, or new alarm controls
- present neutral MedVital wording and styling rather than copying competitor
  text, iconography, color composition, or countdown placement
- limit the feature to read-only visibility of active pause state plus
  remaining time
- keep alarm interaction semantics unchanged

## medical-safety impact

This is a display-only change, but it touches alarm-status visibility, so the
safety impact is not zero. A correct countdown can improve operator awareness
of a temporary mute window. A wrong, stale, or misleading countdown could
create false reassurance and delay response when alarm audio resumes or when an
operator believes alarms are active when they are still paused.

The change is acceptable for design review only if the countdown is tied to the
authoritative pause state and cannot drift independently from the real alarm
pause behavior.

## security and privacy impact

No new patient-data collection, storage, export, authentication path, or
network interface is proposed. Security and privacy impact is expected to be
none if implementation remains a local UI-only state presentation. The main
process risk is integrity of status communication: the UI must not misstate
alarm state.

## affected requirements or "none"

- `SYS-014` is the closest current system-level anchor because it governs the
  graphical dashboard and aggregate status banner.
- `SWR-GUI-003` is the closest current software-level anchor because it governs
  status-banner rendering behavior.

Architect should decide whether to extend those existing GUI requirements or
add a dedicated follow-on SWR for alarm-pause countdown behavior, clear
conditions, refresh cadence, and wording.

## intended use, user population, operating environment, and foreseeable misuse

Intended use: show trained clinical users that alarm audio is temporarily
paused and when the pause is expected to end.

User population: bedside clinicians, ward nurses, intensivists, and other
trained operators using the monitoring dashboard.

Operating environment: the Windows desktop monitoring dashboard described in
`README.md`, initially in the current pilot environment and later in the same
UI path that presents live monitored status.

Foreseeable misuse:

- interpreting the banner as meaning all alarms are disabled when only audible
  alarm output is paused
- relying on the displayed time when it is stale, truncated, or not derived
  from the true pause-expiry source
- assuming the countdown changes alarm thresholds or alarm persistence rules
- ignoring the patient because a paused-state banner visually resembles a
  normal-status banner

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity: serious. A misleading pause indicator could contribute to delayed
recognition of resumed alarm audio or of an alarm still being in a muted state.

Probability: low. The feature is read-only, but the harm path exists if the UI
displays the wrong state and the operator relies on it.

Initial risk: medium.

Risk controls:

- derive the displayed remaining time from the authoritative alarm pause state
  and expiry timestamp, not from an independent decrement-only UI counter
- show a distinct paused-state banner that is visually non-normal and cannot be
  confused with the normal aggregate status
- clear the countdown immediately on manual resume, automatic expiry, logout,
  patient/session reset, or any state transition that ends the pause
- if the underlying function is audio-only pause, the text must say so; do not
  overstate the suppression scope as "all alarms off"
- clamp invalid or expired values so the UI never shows negative time or stale
  "paused" state after the pause ends

Verification method:

- targeted unit or integration checks for pause-start, mid-countdown,
  auto-expiry, manual resume, repeated pause, and stale-state reset paths
- manual GUI verification that the banner updates monotonically, clears at the
  right moment, and remains visually distinct from normal state
- regression confirmation that alarm generation, thresholds, escalation, and
  existing active-alert presentation remain unchanged

Residual risk: low.

Residual-risk acceptability rationale: the change does not alter the alarm
engine itself and can be kept within the existing banner path. With a single
source of truth, fail-safe clearing behavior, and explicit verification of
boundary conditions, residual risk is acceptable for this pilot as a
display-only enhancement.

## hazards and failure modes

- countdown remains visible after pause expiry or manual resume
- countdown reaches zero later or earlier than the real pause state transition
- banner says or implies alarms are fully off when only audible output is
  paused
- paused-state styling looks like normal/green status and creates false
  reassurance
- localization, truncation, or refresh lag makes the remaining time ambiguous
- implementation adds hidden behavior changes to alarm controls while claiming a
  display-only scope

## existing controls

- the issue explicitly scopes the feature as display-only and excludes alarm
  threshold, persistence, and acknowledgement changes
- the current architecture separates GUI presentation from core classification
  and alert-generation logic, limiting this change to the presentation path if
  implemented correctly
- the existing dashboard already has a centralized status-banner rendering path
  in `src/gui_main.c`
- existing requirements and tests already cover alert generation, severity
  classification, and alarm-limit behavior outside this new banner detail

## required design controls

- define the exact source-of-truth field for pause-active state and pause-expiry
  time before UI design starts
- specify the banner wording for the real behavior: "alarm audio paused" if the
  feature pauses audio only, not all alarm processing
- specify countdown format and refresh cadence, including what happens at
  sub-minute values and at zero
- keep the feature read-only; no new pause, resume, acknowledge, or silence
  controls should be introduced under this issue
- update or add requirements so verification can assert clear-on-expiry,
  clear-on-resume, and non-normal paused-state presentation
- review the implementation diff to ensure alarm logic, thresholds, and
  persistence are unchanged

## validation expectations

- run the issue's suggested validation commands after implementation:
  `run_tests.bat`
- run targeted regression checks:
  `ctest --test-dir build --output-on-failure -R GUI|Config|Alarm|Patient`
- perform a manual GUI scenario:
  pause alarms, confirm the banner appears, confirm the countdown decreases,
  confirm it clears exactly when the pause ends, and confirm manual resume
  clears it immediately
- verify that active visual alarm severity presentation remains correct while
  audio is paused
- verify that the banner never shows a negative, frozen, or contradictory
  countdown after pause expiry or session reset

## clinical-safety boundary and claims that must not be made

- do not claim the feature improves alarm detection accuracy, alarm timing, or
  clinical outcomes by itself
- do not present the countdown as a substitute for operator vigilance or alarm
  review
- do not imply any change to alarm thresholds, alert severity logic, or patient
  risk scoring
- do not imply regulatory equivalence to competitor monitors based on the cited
  manuals

## residual risk for this pilot

Low, provided the implementation remains display-only and the countdown is
driven by a single authoritative pause-state source. The main residual hazard is
user misinterpretation caused by stale or overstated wording, which is
manageable with the controls above.

## whether the candidate is safe to send to Architect

Yes, with conditions. This candidate is safe to send to Architect because the
product hypothesis is narrow, the source evidence is adequate for non-copying UX
precedent, and the safety boundary is clear. Architect should preserve
display-only scope and make the pause-state semantics explicit before design.

## human owner decision needed, if any

- confirm whether the underlying behavior being surfaced is "alarm audio
  paused" or "alarms paused" so the banner does not overstate the suppression
  scope
- confirm whether the pause duration is fixed or configurable, because that
  affects countdown wording and verification
- confirm whether this behavior should extend `SYS-014` / `SWR-GUI-003` or land
  as a new dedicated GUI requirement
