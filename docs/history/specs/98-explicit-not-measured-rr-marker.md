# Design Spec: Issue 98

Issue: `#98`  
Branch: `feature/98-explicit-not-measured-rr-marker`  
Spec path: `docs/history/specs/98-explicit-not-measured-rr-marker.md`

## Problem

The repository already treats `VitalSigns.respiration_rate == 0` as a pilot
"not measured" sentinel for clinical logic:

- `src/vitals.c` skips RR escalation in `overall_alert_level()`
- `src/alerts.c` does not emit a missing-RR alert
- `src/news2.c` gives RR a zero NEWS2 sub-score when it is not measured

The presentation layer is inconsistent:

- `src/gui_main.c` already shows `--` on the live RR tile when RR is missing
- `src/gui_main.c` omits RR entirely from dashboard reading-history rows when
  RR is missing
- `src/patient.c` `patient_print_summary()` does not print a respiration-rate
  line at all, so the summary cannot distinguish "not measured" from
  "not shown"

That inconsistency creates avoidable ambiguity during bedside review, test
evidence capture, and handoff because a hidden RR field can look like complete
patient state rather than incomplete measurement state.

## Goal

Add an explicit, traceable missing-state presentation for respiration rate on
the surfaces that currently hide it, while preserving the existing approved
clinical semantics for RR missing values.

The intended outcome is:

- dashboard surfaces never silently omit RR for a stored patient reading
- the patient summary always includes RR when a latest reading exists
- missing RR is shown as a non-numeric state rather than a numeric zero
- alerting, NEWS2, thresholds, and device-state behavior remain unchanged

## Non-goals

- No change to RR thresholds, alert generation, aggregate alert escalation, or
  NEWS2 scoring.
- No change to the meaning of `respiration_rate == 0` for this pilot issue.
- No new sensor-fault inference, disconnect detection, or transport contract.
- No change to simulation-off, device-mode, or no-patient placeholders that
  currently use `N/A` or equivalent non-patient messaging.
- No redesign of unrelated vital-sign tiles, alarm lists, or patient-summary
  formatting outside the RR-specific gap.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

Intended use impact:

- The change improves visibility of missing RR data on existing review
  surfaces.
- It does not change how clinicians interpret numeric RR values or active
  alerts.

User population:

- Trained clinical users, internal testers, and reviewers using the current
  Windows desktop application.

Operating environment:

- The current single-patient local workstation flow, with static in-process
  storage and either simulator-fed or manually entered readings.

Foreseeable misuse:

- Interpreting `--` as "clinically normal" instead of "not measured"
- Assuming missing RR implies sensor disconnection or device fault
- Confusing RR missing state with simulation-off or no-patient state
- Carrying the `RR == 0` sentinel into future device integration without
  revalidating that contract

## Current Behavior

- The live dashboard RR tile already renders `--` when `respiration_rate == 0`.
- Dashboard reading-history rows omit the entire RR segment when RR is missing.
- `patient_print_summary()` prints HR, BP, Temperature, and SpO2, but no RR
  line, even when a latest reading exists.
- The no-patient and simulation-off states already use distinct placeholders
  such as `N/A` or patient/session messages and should remain distinct from RR
  missing-state presentation.
- Existing automated coverage already proves that missing RR does not trigger
  spurious alert escalation or NEWS2 inflation.

## Proposed Change

Implement the issue as a presentation-only consistency fix with these
decisions:

1. Keep `respiration_rate == 0` as the approved pilot sentinel for "not
   measured" in this issue only. Do not reinterpret it as a device fault or a
   physiological claim.
2. Treat the existing live RR tile behavior as the UI baseline: the RR value
   token for missing data remains `--`.
3. Update dashboard reading-history rows in `src/gui_main.c` so every stored
   reading includes an RR segment:
   - numeric RR: `RR <value> br/min`
   - missing RR: `RR -- br/min`
4. Update `patient_print_summary()` so the latest-vitals section always prints
   an RR line whenever a latest reading exists:
   - numeric RR: show the measured value, unit, and the current RR
     classification from `check_respiration_rate()`
   - missing RR: show `--` as the value token and an explicit textual marker
     such as `NOT MEASURED` rather than calling `check_respiration_rate(0)`
5. Keep the distinction between:
   - RR missing for a patient reading
   - no patient admitted
   - simulation/device mode without live data
   - a real numeric RR reading
6. Do not change `overall_alert_level()`, `generate_alerts()`,
   `news2_calculate()`, or RR threshold rules as part of this issue.
7. If implementation needs helper formatting logic to make summary output or
   history rows testable, that refactor is allowed only inside the scoped
   presentation files and must not introduce new RR semantics.
8. If implementation reveals that the team cannot express dashboard
   reading-history behavior under current approved GUI requirements, add the
   minimal requirements wording needed for traceability rather than broadening
   the feature.

## Files Expected To Change

Expected requirements and traceability files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected implementation files:

- `src/gui_main.c`
- `src/patient.c`

Expected verification files:

- `tests/unit/test_patient.cpp`
- GUI or DVT/manual verification evidence for dashboard reading-history output

Files expected not to change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/sim_vitals.c`
- authentication, persistence, and alarm-limit modules

## Requirements And Traceability Impact

Existing requirements directly impacted:

- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard
- `SYS-018` Respiration Rate Monitoring and Classification
- `SWR-PAT-006` Patient Summary Display

Likely requirements clarification needed:

- `SWR-PAT-006` should explicitly require RR to be shown in the summary both
  when present and when not measured.
- Existing GUI wording does not clearly govern dashboard reading-history
  missing-state presentation outside the live RR tile. The team should either:
  - amend `SWR-GUI-003` with scoped wording for RR missing-state display on
    dashboard review surfaces, or
  - add a narrow GUI requirement that covers reading-history row rendering for
    missing RR.
- `requirements/TRACEABILITY.md` should be updated so the selected GUI and
  summary evidence maps to the revised requirement wording.

Traceability boundaries:

- This issue refines presentation of an already-approved missing-data semantic.
- It does not justify new RR clinical behavior, new alarm semantics, or a new
  device/HAL contract.

## Medical-Safety, Security, And Privacy Impact

Medical safety:

- This is acceptable only as a display-only change.
- Primary safety benefit: missing RR becomes visible instead of silent.
- Primary presentation risk: users may misread `--` as stable unless the
  summary and dashboard context make the missing state explicit.
- The design must preserve a clear distinction between RR missing and broader
  system-state placeholders such as `N/A`.

Security:

- No new authentication, authorization, network, or file-persistence behavior
  is introduced.

Privacy:

- No new patient-data storage, export, or transmission path is introduced.
- The feature only changes how existing local-session data is rendered.

## AI/ML Impact Assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: not applicable
- Input data: existing deterministic vital-sign readings only
- Output: deterministic text rendering of RR presence or absence
- Human-in-the-loop limits: unchanged
- Transparency needs: standard UI clarity only; no AI explainability concern
- Dataset and bias considerations: not applicable
- Monitoring expectations: standard software verification and GUI review only
- PCCP impact: none

## Validation Plan

Implementation should add targeted automated coverage plus bounded GUI review
evidence.

Automated validation scope:

- Extend `tests/unit/test_patient.cpp` to capture `patient_print_summary()`
  output and assert:
  - a present RR value prints numerically with unit
  - a missing RR value prints explicitly and is not silently omitted
- Re-run existing RR regression coverage to confirm no alert or NEWS2 behavior
  changed for `respiration_rate == 0`.

Recommended validation commands:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Recommended focused review evidence:

- manual or DVT-backed verification that dashboard reading-history rows show
  `RR -- br/min` when RR is missing
- manual confirmation that simulation-off and no-patient states still show
  their existing non-patient placeholders instead of the RR missing marker

Expected validation outcome:

- RR missing remains non-escalatory in alerts and NEWS2
- the summary and dashboard history stop hiding RR when it is not measured
- no unrelated dashboard or clinical behavior changes are introduced

## Rollback Or Failure Handling

- If implementation requires changing RR thresholds, alert semantics, NEWS2,
  or device/HAL behavior, stop and split that work into a follow-on issue.
- If the team cannot keep RR missing distinct from `N/A`, no-patient, or
  device-mode states, do not ship a confusing hybrid marker; revert to the
  pre-change behavior and reopen design clarification.
- If requirements wording cannot capture dashboard reading-history scope
  cleanly, narrow the implementation to the explicitly approved surfaces rather
  than broadening undocumented GUI behavior.
- Rollback is straightforward because the intended runtime change is limited to
  RR-specific presentation formatting in existing summary and dashboard code.
