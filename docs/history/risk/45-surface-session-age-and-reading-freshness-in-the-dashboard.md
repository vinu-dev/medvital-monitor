# Risk Note: Issue #45

Date: 2026-05-06
Issue: `#45` - Feature: surface session age and reading freshness in the dashboard

## proposed change

Add a read-only dashboard indicator for:

- session age since the current patient session began
- freshness of the most recent reading, preferably as relative elapsed time

The change should stay inside the presentation/session layer and must not alter
acquisition cadence, threshold logic, NEWS2 scoring, alert generation, patient
record semantics, persistence, or export behavior.

Current repo evidence supports the gap:

- `src/gui_main.c:292` `paint_header()` shows user and simulation state only.
- `src/gui_main.c:333` `paint_patient_bar()` shows demographics, BMI, and
  reading count, but no freshness or timestamp context.
- `src/gui_main.c:680` `update_dashboard()` renders ordinal reading history, not
  time-based recency context.

## product hypothesis and intended user benefit

The product hypothesis is credible: operators reviewing the live dashboard will
more quickly detect whether the screen is current, paused, or stale when the UI
shows session age and last-reading freshness explicitly.

Expected user benefit for this pilot:

- safer handoff and demo review because stale data are easier to spot
- more trustworthy DVT screenshots and walkthroughs
- reduced ambiguity when simulation is paused, resumed, or waiting for the
  first reading

## source evidence quality

Source quality is sufficient for product-discovery context, but not for
clinical-effectiveness claims.

- The Mindray ePM 10M/ePM 12M operator manual states that changing date/time
  affects trend and event storage, and that time may be sourced from a central
  monitoring system. That supports time-context importance, but also shows that
  clock provenance is safety-relevant.
  Source:
  https://www.mindray.com/content/dam/xpace/en_us/resource-library/patient-monitoring/technical-document/continuous-patient-monitoring/H-046-019798-00-ePM-10M-ePM-12M-ops-manual-FDA-9.0.pdf
- The Philips MR Patient Care Portal 5000 instructions for use describe a
  patient bar and a date/time area on the main screen. That supports freshness
  and time context as a common monitoring-display convention.
  Source:
  https://www.documents.philips.com/assets/Instruction%20for%20Use/20250611/8761d1e3741948a48e86b2f90143275d.pdf?feed=ifu_docs_feed
- Both sources are vendor manuals for existing products. They are useful for
  general HMI benchmarking only. They do not justify copying layout details,
  and they do not prove clinical outcome improvement for this product.

## medical-safety impact

This is a display-only feature, but it affects how a human interprets whether
the displayed physiology is current. The primary hazard is false reassurance:
stale or paused data may be mistaken for a fresh live state if the feature is
implemented ambiguously.

Medical-safety impact is therefore indirect but real. The feature is acceptable
for design only if it makes recency clearer without implying stronger claims
than the system can currently support.

## security and privacy impact

No direct security-control change is needed if the feature remains local and
session-scoped.

Privacy impact is low, but not zero:

- if timestamps appear in screenshots, they become additional patient-context
  metadata
- if absolute wall-clock time is introduced, designers may be tempted to add
  persistence, synchronization, or audit expectations that are not in current
  scope

The feature should not add storage, network sync, export, or logging of
freshness metadata in this issue.

## affected requirements or none

Related current requirements:

- `UNS-010` consolidated status summary
- `UNS-014` graphical dashboard
- `UNS-015` live monitoring feed
- `SYS-011` patient status summary display
- `SYS-014` graphical vital signs dashboard
- `SYS-015` hardware abstraction layer
- `SWR-GUI-003` color-coded vital signs display
- `SWR-PAT-006` patient summary display
- `SWR-GUI-010` sim toggle and persistence
- `SWR-GUI-011` rolling simulation status banner

Gap to address during design:

- no current requirement explicitly defines session-age or reading-freshness
  semantics
- no current requirement defines pause behavior, stale-state wording, or time
  source/provenance for an absolute timestamp

Design should add explicit requirement text rather than stretching existing
GUI-summary requirements beyond their current approved meaning.

## intended use, user population, operating environment, and foreseeable misuse

Intended use:

- help a trained operator understand how old the current session is and how
  recent the latest displayed reading is

Intended user population:

- clinical or demo operators using the Win32 dashboard during supervised pilot
  monitoring, demos, DVT, and review workflows

Operating environment:

- local Windows workstation running the current single-process monitor GUI in
  simulation mode today, with possible future real-HAL back-end reuse

Foreseeable misuse:

- treating a stale but normal-looking screen as current
- assuming an absolute timestamp is clinically authoritative or synchronized
- assuming session age proves sensor connectivity or data validity
- using freshness text as a substitute for alarm review or clinical judgment

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: medium. Misread recency could delay recognition that the display is
  paused or stale, but the feature does not change physiologic computation or
  alarm logic.
- Probability: medium without controls, because the UI currently lacks explicit
  freshness context and operators may over-trust a static display.
- Initial risk: medium.
- Risk controls:
  - Prefer relative elapsed age for MVP, derived from a monotonic session timer,
    not configurable wall-clock time.
  - Freeze or explicitly relabel freshness when simulation is paused or no new
    reading has arrived.
  - Reset session-age and freshness state on admit, logout, and new session.
  - Distinguish `no reading yet`, `live`, `paused`, and `stale/no update`
    states clearly.
  - Keep the feature read-only and separate from alert thresholds, NEWS2,
    trends, exports, and stored patient records.
  - If absolute timestamps are later required, define clock source, drift,
    timezone/locale behavior, and effects of system time changes explicitly.
- Verification method:
  - targeted GUI verification for first reading, normal live updates, paused
    simulation, resumed simulation, no-patient state, and new-session reset
  - targeted review confirming no change to alert generation, NEWS2, or patient
    record storage semantics
  - requirement review confirming explicit wording for freshness semantics and
    pause behavior
- Residual risk: low for the pilot if the controls above are implemented.
- Residual-risk acceptability rationale: acceptable because the feature is
  contextual and read-only, existing alert behavior remains authoritative, and
  the recommended MVP avoids clock-provenance claims that the current product
  cannot yet defend.

## hazards and failure modes

- Freshness text keeps advancing while the simulation is paused, falsely
  implying new data.
- A wall-clock timestamp jumps backward or forward after a workstation time
  change and makes old data appear recent.
- Session age or last-reading age persists across logout or new patient admit.
- The UI shows a green or neutral freshness indicator even when no reading has
  arrived yet.
- Freshness text crowds or visually competes with alert status and reduces
  noticeability of clinically meaningful warning colors.
- Designers store timestamps inside the clinical record path, widening
  traceability and validation scope unintentionally.

## existing controls

- `paint_header()` already exposes simulation live/paused state.
- `paint_patient_bar()` already centralizes high-level patient/session context.
- `update_dashboard()` already refreshes dashboard lists after new readings.
- `SWR-GUI-002` requires logout to clear all session data.
- `UNS-015` / `SYS-015` constrain acquisition behind the HAL, limiting this
  issue to presentation concerns if the change is scoped correctly.
- Existing alert and NEWS2 logic already remain outside this proposed feature.

## required design controls

- Keep freshness/session-age metadata out of the clinical core unless a
  separate requirements change explicitly expands scope.
- Prefer a relative `last reading age` label over an absolute timestamp for the
  MVP.
- If both session age and reading age are shown, label them distinctly so users
  do not confuse session duration with data recency.
- Show explicit wording for paused/no-data states rather than continuing the
  normal freshness presentation.
- Preserve current alert banner prominence and tile readability.
- Add explicit verification evidence for freshness-state behavior; do not rely
  on adjacent GUI demo evidence alone.
- Treat any request for persistence, synchronization to external clocks, or
  trend/event timestamp semantics as a follow-on issue with fresh review.

## validation expectations

- Review the final design against `UNS-010`, `UNS-014`, `UNS-015`, `SYS-011`,
  and `SYS-014` to ensure the new display remains contextual rather than
  clinically interpretive.
- Demonstrate correct behavior for:
  - no patient admitted
  - patient admitted before first reading
  - fresh reading arrival
  - paused simulation
  - resumed simulation
  - logout/new session reset
- Confirm the implementation does not modify:
  - threshold logic
  - NEWS2 logic
  - stored reading payload semantics
  - network or persistence behavior
- If absolute timestamps are proposed, add tests or review evidence for clock
  adjustment behavior and explicitly document limitations.

## residual risk for this pilot

Residual pilot risk is low if the team ships a relative-age, display-only cue
with explicit paused/no-data behavior and no new persistence or synchronization
claims.

Residual pilot risk becomes medium if the design uses workstation wall-clock
time as if it were authoritative clinical provenance, or if the feature is
allowed to imply that displayed values are validated as current beyond the last
local update cycle.

## clinical-safety boundary and claims that must not be made

The feature must not claim or imply:

- clinical time synchronization with external systems
- audit-grade event chronology
- confirmation that sensors are connected and healthy
- confirmation that data are suitable for diagnosis or treatment by freshness
  alone
- any change in alarm behavior, thresholding, or physiologic interpretation

## MVP boundary that avoids copying proprietary UX

Acceptable MVP boundary:

- one compact session-age label
- one compact last-reading-age label
- local wording and layout chosen for this product's existing header/patient bar
- relative-age presentation aligned to the current simulation workflow

Out of scope for this issue:

- cloning vendor patient-bar layout, terminology, iconography, or screen
  composition
- adding trend review, event review, historical timestamps, or CMS/HIS time
  synchronization
- repackaging the feature as a clinical-record or audit-log capability

## whether the candidate is safe to send to Architect

Yes, with constraints.

It is safe to send to Architect if the candidate is framed as a read-only,
session-scoped freshness cue and the preferred MVP uses relative elapsed time
instead of authoritative wall-clock timestamps. The architect should treat
absolute timestamp behavior, time synchronization, persistence, or changes to
clinical record structure as out of scope unless reopened under a new approved
requirements change.

## human owner decision needed, if any

No blocking human decision is needed if the MVP is limited to relative session
age and relative last-reading age.

A human product/quality decision is needed before design only if the team wants
to:

- display authoritative wall-clock timestamps
- claim synchronization with external systems
- persist freshness metadata outside the active session
- reuse this feature as evidence for clinical chronology, auditability, or
  treatment workflow decisions
