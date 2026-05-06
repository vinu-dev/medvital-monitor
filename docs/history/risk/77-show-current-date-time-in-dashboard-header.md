# Risk Note: Issue 77

Issue: `#77`  
Branch: `feature/77-show-current-date-time-in-dashboard-header`

## proposed change

Add a compact read-only current date/time display to the dashboard header so an
operator can orient the screen without relying on the Windows title bar or OS
chrome.

The assessed scope is limited to presentation only:

- no change to clinical thresholds, alarms, NEWS2, patient records, or alert logic
- no change to timekeeping source, persistence, or synchronization behavior
- no patient-specific timers, elapsed-time displays, or event timestamps

Repo evidence reviewed for this assessment:

- `src/gui_main.c` already has a dedicated `paint_header()` path for small
  header context items.
- `src/gui_main.c` already refreshes the dashboard on `WM_TIMER`, so a clock can
  reuse the existing redraw path instead of introducing a second timing loop.

## product hypothesis and intended user benefit

Hypothesis: a continuously visible system clock improves operator orientation
during review, handoff, and alarm correlation on a shared monitoring station.

Intended user benefit:

- faster time-of-day orientation during handoff and review
- less dependence on the OS window chrome or external wall clock
- no workflow disruption because the change stays in the existing header area

## source evidence quality

Evidence quality is adequate for product discovery and MVP scoping, but not for
clinical-effectiveness claims.

- The issue cites an official Philips EarlyVue VS30 Instructions for Use PDF
  accessed on 2026-05-06. That IFU describes a Date/Time pane on the main
  screen toolbar and indicates that the pane is always displayed. This is a
  credible example of market practice for monitor UI context, not proof of
  clinical benefit.
- Internal repo evidence is stronger for implementation feasibility than for
  clinical value: the current header paint path and periodic redraw loop show
  that the feature fits the existing UI architecture with low technical
  intrusion.
- No public evidence was provided that an always-visible clock improves
  clinical outcomes directly. The pilot should therefore frame this as operator
  orientation support, not as a safety-performance claim.

## MVP boundary that avoids copying proprietary UX

- Reuse only the generic concept of an always-visible read-only system clock.
- Do not copy Philips layout, exact placement, iconography, wording, menu
  behavior, or toolbar structure.
- Keep the pilot MVP to plain text in the existing header paint path.
- Exclude time-setting controls, synchronization controls, records integration,
  and any claim that the product matches another monitor's UX.

## clinical-safety boundary and claims that must not be made

- The display must be presented as system time for operator orientation only.
- The feature must not be described as a patient-event timestamp, charting
  timestamp, elapsed timer, alarm timer, or treatment timer.
- The feature must not imply synchronized time with EMR, SNTP, bedside devices,
  or other hospital systems unless that capability is separately designed,
  implemented, and risk-reviewed.
- The feature must not be used to justify any clinical claim beyond general
  review and handoff orientation.

## whether the candidate is safe to send to Architect

Yes, conditionally. It is safe to send to Architect if scope remains a
read-only system-time display in the existing header and the resulting design
adds explicit requirements for label, format, refresh behavior, and
non-interference with existing header elements.

## medical-safety impact

Direct medical-safety impact is low because the issue does not alter measured
values, thresholds, alarms, alert routing, NEWS2, persistence, or patient
identity. The plausible safety concern is indirect: if the time is stale,
ambiguous, or misread as a patient-specific time marker, a clinician could
mis-correlate observations during handoff or alarm review.

The clock is therefore acceptable only as a contextual display, not as a
clinical timing control.

## security and privacy impact

Expected security and privacy impact is low.

- No new patient data element is introduced.
- No new authentication, authorization, or network boundary is required if the
  feature reads the local system clock only.
- No new persistence should be introduced for this MVP.

If future scope expands into network time synchronization or audit/event
timestamping, that would require a separate security and risk review because it
would change trust assumptions around time provenance.

## affected requirements or "none"

none in the current approved baseline.

Before implementation, design should add traceable requirements covering at
least:

- the clock's purpose as system-time orientation only
- the display label and non-ambiguous format
- refresh/update behavior
- non-interference with existing header controls and status indicators

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- give trained clinical staff a visible system-time reference on the main
  monitoring dashboard

User population:

- ward nurses, bedside clinicians, and other trained operators using the pilot
  on a shared Windows workstation

Operating environment:

- a desktop monitoring station in a clinical or simulated clinical setting,
  including handoff and review use where the dashboard is already open

Foreseeable misuse:

- reading the clock as elapsed session time or alarm duration
- reading the clock as a patient-specific event timestamp
- assuming the displayed time is centrally synchronized when it is only local
  workstation time
- misreading an ambiguous numeric date format
- relying on a stale display after resume, login/logout, or other UI state
  changes

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity:

- low to moderate workflow impact. The credible harm is delayed or incorrect
  review correlation, not direct alteration of patient-monitoring logic.

Probability:

- low if the display is clearly labeled, read-only, and formatted
  unambiguously
- higher if the display is unlabeled, ambiguous, clipped, or stale

Initial risk:

- low to medium before controls because the display could be misinterpreted in
  a time-sensitive workflow even though it does not change measurements

Risk controls:

- label the field explicitly as `SYSTEM TIME` or equivalent
- use an unambiguous date/time format, preferably 24-hour time and a month-name
  or ISO-like date representation
- keep the clock visually distinct from patient identifiers, alarm/status text,
  and any future timer concepts
- update via the existing dashboard refresh path so it does not become stale
  during normal operation
- refresh correctly across login, logout, admit/refresh, clear-session, pause,
  resume, and window resize flows
- keep the MVP read-only with no dashboard control for setting time
- preserve readability and avoid overlap with user badges, simulation state, or
  header buttons at the minimum supported window width
- if future design needs synchronized or auditable clinical timestamps, split
  that into a separate issue and risk review

Verification method:

- requirements review confirming scope stays presentation-only
- visual GUI verification that the label, placement, contrast, and format stay
  readable and non-overlapping
- manual behavior checks during login/logout, admit/discharge-style refresh,
  pause/resume, and minimum-width resize
- manual system-clock change check to confirm the displayed value follows the
  local system clock and updates after redraw
- regression check that patient records, alerts, and exports do not start using
  the header clock as a clinical timestamp

Residual risk:

- low after controls because the feature remains contextual only and does not
  drive clinical decision logic

Residual-risk acceptability rationale:

- the operator-orientation benefit is reasonable for the pilot if the UI makes
  the display clearly system time and avoids ambiguous timing semantics

## hazards and failure modes

- incorrect workstation time is displayed and trusted during handoff or review
- ambiguous date format causes day/month confusion
- operator mistakes the clock for a patient-event, charting, or alarm timer
- clock stops updating and gives a false impression of current time
- new text overlaps the logged-in user, role badge, simulation indicator, or
  header buttons
- later design scope expands into editable or synchronized time behavior
  without new requirements and review

## existing controls

- The issue already constrains the change to a display-only clock and excludes
  time synchronization behavior.
- `paint_header()` provides a single bounded rendering surface for header
  content.
- The dashboard already redraws on a periodic timer, reducing pressure to add a
  second time-update mechanism.
- Existing patient, alarm, and measurement logic is structurally separate from
  the header paint path.

## required design controls

- Add explicit requirements before coding; do not implement from issue prose
  alone.
- Treat the clock as system context only, not part of the patient record model.
- Use wording and styling that distinguish the clock from timers and alarm
  status.
- Choose a format approved by the product owner that avoids ambiguous
  day/month interpretation.
- Keep interaction out of scope for the pilot; no click-to-edit or time menu
  from the dashboard header.
- Confirm the new element does not crowd out existing header controls at the
  supported minimum window size.

## validation expectations

- Manual GUI smoke confirming the clock is visible after login and remains
  visible through logout/login and patient refresh flows
- Manual check that the displayed value changes over time and does not freeze
  when simulation is paused
- Manual resize check at the minimum supported width
- Manual check that the clock does not imply patient-specific timing in labels
  or surrounding copy
- Design review confirming new requirements and traceability are created before
  implementation

## residual risk for this pilot

Low, provided the pilot keeps the feature read-only, clearly labeled as system
time, and free from synchronization or clinical-event claims. The feature is
appropriate for pilot design under those constraints.

## human owner decision needed, if any

- Approve the exact label and date/time format policy for the pilot.
- Decide whether local workstation time is acceptable for this pilot or whether
  future synchronized time behavior should be separately specified.
- Confirm that product messaging will describe the feature as orientation aid
  only, not as a clinical timestamping capability.
