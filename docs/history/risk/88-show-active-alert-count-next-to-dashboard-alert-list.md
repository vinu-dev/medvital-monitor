# Risk Note: Issue #88 - Show Active Alert Count Next To Dashboard Alert List

Date: 2026-05-06
Issue: #88 "Feature: show active alert count next to the dashboard alert list"
Branch: `feature/88-show-active-alert-count-next-to-dashboard-alert-list`

## Proposed change

Add a compact current-state badge next to the dashboard's active-alert list so
the operator can see how many alerts are active without scanning every row. The
badge should remain display-only and should be derived from the same active
alert set already shown in the list.

Repo observations supporting the change:

- `src/gui_main.c` `update_dashboard()` clears `IDC_LIST_ALERTS`, calls
  `patient_latest_reading()`, then derives the current active-alert rows from
  `generate_alerts(latest, alerts, MAX_ALERTS)`.
- When no active alerts exist, `update_dashboard()` already shows the explicit
  placeholder `No active alerts - all parameters within normal range.`
- The dashboard already has a separate `Session Alarm Events` list, so the
  proposed count must remain tied to current active alerts only and must not
  count historical event rows.
- Patient/session reset boundaries already exist in code:
  `patient_init()` on admit/sim startup, `do_clear()` on Clear Session, logout
  zeroing of patient state, and automatic simulation rollover when the reading
  buffer is full.

## Product hypothesis and intended user benefit

Hypothesis: a small active-alert count improves at-a-glance workload awareness
for trained users because it exposes current alarm burden before the user reads
each alert row.

Expected user benefit:

- faster recognition that more than one abnormal condition is active
- less visual scanning during handoff, review, or rapid bedside checks
- a narrow GUI-only workflow aid that does not expand clinical scope

## Source evidence quality

Evidence quality is adequate for product-discovery scope and weak for any
clinical-effectiveness claim.

- Philips IntelliVue Patient Information Center iX IFU link resolves to a live
  PDF and supports the general existence of alarm-count/summary review
  workflows in patient-monitoring products. It is useful as workflow context,
  not as validation of safety benefit or as a copying target.
  Source: https://www.documents.philips.com/assets/Instruction%20for%20Use/20250321/d80393498eb1407e8912b2a7015f378e.pdf?feed=ifu_docs_feed
- GE HealthCare's alarm-management page resolves live and frames alarm
  management as a workflow and alarm-fatigue problem. This supports the product
  hypothesis that concise burden cues are useful, but it is marketing material
  rather than clinical evidence.
  Source: https://www.gehealthcare.com/en-us/products/patient-monitoring/alarm-management

Conclusion: the sources are sufficient to justify a narrow, non-copying MVP
count badge. They are not sufficient to claim reduced adverse events, reduced
alarm fatigue, or improved clinical outcomes.

## Medical-safety impact

This change does not alter alert thresholds, NEWS2 logic, alarm limits, alert
generation rules, or patient-care workflow. Risk arises from presentation and
user interpretation, not from new clinical computation.

Primary safety benefit:

- makes multi-alert burden visible sooner when the active-alert list contains
  several current abnormalities

Primary safety risks:

- the badge undercounts, overcounts, or lags the active-alert list
- the badge survives a patient/session reset and shows stale burden
- the user treats the count as a severity signal, for example assuming one
  critical alert is less urgent than two warning alerts
- the badge visually competes with or obscures the active-alert text rows

Overall medical-safety impact: low-to-moderate if the count remains a passive
adjunct to the current active-alert list and existing severity banner. It is
not acceptable if the badge becomes an independent alarm-status source.

## Security and privacy impact

Security and privacy impact is minimal because the badge exposes only a count
of data already visible inside the authenticated dashboard.

- No new PHI category is introduced.
- No new network, export, or persistence path should be introduced.
- The count must clear under the same logout and session-reset boundaries as
  the existing alert list.
- Device mode, no-patient state, and session rollover states must not leave a
  stale count visible.

## Affected requirements or "none"

Likely affected existing requirements:

- `UNS-010` Consolidated Status Summary
- `UNS-014` Graphical User Interface
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-GUI-003` Colour-Coded Vital Signs Display

Existing alert semantics should be reused, not changed:

- `SWR-ALT-001` through `SWR-ALT-004`

New derived requirement(s) are likely needed for:

- active-alert count derivation from the current alert list source of truth
- zero/none/current-patient state wording
- reset and clearing behavior across admit, clear, logout, device mode, and
  automatic session rollover
- layout behavior so the count does not reduce the readability of active alerts

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained users see how many current active alerts exist for the active patient
  in the local dashboard session

User population:

- bedside clinical staff and internal testers using the Windows workstation
  application

Operating environment:

- single-patient local Win32 dashboard in the current pilot scope, using
  simulator-fed or manually entered readings

Foreseeable misuse:

- treating the count as a severity ranking instead of a quantity cue
- assuming `0` means no recent instability, rather than no current active
  alerts
- assuming the count includes historical `Session Alarm Events`
- relying on the badge instead of reading the actual alert rows and severity
  banner

## MVP boundary that avoids copying proprietary UX

The MVP should remain narrow:

- one text badge or compact label adjacent to the existing active-alert list
- current active-alert quantity only
- no waveform summaries, no historical counters, no central-station style alarm
  dashboards, no trend analytics, and no per-severity matrix copied from
  competitors
- no new acknowledge, silence, escalate, export, or audit workflow

## Clinical-safety boundary and claims that must not be made

The feature may claim to summarize current active-alert count. It must not
claim to:

- replace the active-alert list or severity banner
- prioritize, triage, diagnose, or recommend treatment
- prove the patient is stable when the count is `0`
- summarize historical alert burden unless that is separately designed and
  validated
- reduce alarm fatigue or improve outcomes without independent human evidence

## Whether the candidate is safe to send to Architect

Yes, with constraints. The candidate is safe to send to Architect because it is
display-only, localized to the GUI, and does not alter clinical algorithms.
Design must keep the count tied to the same current alert calculation already
used for `IDC_LIST_ALERTS` and must clear the count on every existing
patient/session reset path.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a misleading count could slow recognition of current alert burden or support a wrong impression that no alerts are active, but the feature does not create or suppress the underlying alerts. |
| Probability | Possible without controls because a new GUI element can drift from the list logic, survive reset incorrectly, or be misread as a priority cue. |
| Initial risk | Medium |
| Key risk controls | Derive the badge from the same `generate_alerts()` result already used for the current list; update the list and badge atomically in `update_dashboard()`; distinguish `no patient`, `0 active alerts`, and `N active alerts`; clear on admit, Clear Session, logout, device-mode transition, and simulation rollover; keep severity communication on the existing banner and rows. |
| Verification method | GUI smoke checks for 0, 1, and multiple active alerts; reset-path checks for Admit, Clear Session, Logout, device mode, and auto-rollover; review that count equals current row count and excludes `Session Alarm Events`; traceability update for the new GUI behavior. |
| Residual risk | Low if controls are implemented and verified. |
| Residual-risk acceptability rationale | Acceptable for this pilot because the feature stays display-only, reuses the validated alert engine, and leaves the existing active-alert list and severity banner as the primary safety surfaces. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Current alert burden is understated | Badge shows fewer alerts than the active-alert list or fails to refresh with the latest reading | User underestimates current instability and delays review |
| Wrong-patient or stale alert burden is shown | Badge persists after Admit, Clear Session, Logout, device-mode switch, or session rollover | User forms a false impression of the active patient's status |
| Quantity is mistaken for priority | User treats `2 active` as more urgent than one critical alert | Severe condition may be deprioritized |
| Historical and current states are conflated | Badge counts `Session Alarm Events` or other historical records instead of only current active alerts | User responds to the wrong risk picture |
| Alert readability is reduced | Badge placement or resizing hides, clips, or de-emphasizes active alert rows | User misses the specific alert text needed for action |

## Existing controls

- Current active alerts are already derived centrally via `generate_alerts()`.
- `update_dashboard()` already rebuilds the live alert list from the latest
  reading rather than from cached GUI state.
- The dashboard already distinguishes current active alerts from historical
  `Session Alarm Events`.
- Existing session reset paths already clear or reinitialize patient state.
- The current severity banner and row text remain the primary detailed alert
  surfaces.

These controls reduce risk, but they do not by themselves guarantee that a new
count badge will stay synchronized or be interpreted safely.

## Required design controls

- Compute the badge from the same current-reading alert count already used to
  populate `IDC_LIST_ALERTS`; do not keep an independent cached count.
- Update badge text and active-alert rows in the same `update_dashboard()`
  execution path.
- Distinguish clearly between:
  - no patient admitted
  - no current active alerts
  - one or more current active alerts
- Exclude `Session Alarm Events` and any historical review data from the badge.
- Clear or recompute the badge on all existing reset boundaries, including
  `patient_init()`, `do_clear()`, logout, device-mode transition, and automatic
  session rollover.
- Keep severity communication on the existing banner and alert rows; do not add
  count-based severity colors or novel urgency logic.
- Verify the layout at the current minimum window width and under supported
  localization settings so the badge does not reduce list readability.

## Validation expectations

- GUI smoke check with normal, warning, and critical scenarios confirming badge
  values of 0, 1, and multiple active alerts
- verification that a single critical alert still reads as urgent even when the
  count is `1`
- reset-path checks for Admit, Clear Session, Logout, device mode, and
  simulation rollover
- review that the badge value matches the number of current active-alert rows
  and does not track session-event history
- traceability updates for any new or refined GUI requirement introduced by the
  design

## Residual risk for this pilot

Residual risk remains that users may over-trust a compact count and not read
the detailed active-alert text. For this pilot that is acceptable only if the
badge is visually secondary to the existing severity banner and active-alert
rows, and only if `0` is clearly understood to mean no current active alerts,
not no recent instability.

## Human owner decision needed, if any

Product owner / architect should explicitly decide:

- the exact wording and visual treatment for `0`, `1`, and many active alerts
- whether the badge remains GUI-only or also affects summary/requirements text
- whether minimum-width layout changes are acceptable if the current alert list
  becomes crowded
- whether supported localization variants need shortened badge text to avoid
  clipping
