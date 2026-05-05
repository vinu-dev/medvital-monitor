# Design Spec: Restore defensible SYS mappings for RR and NEWS2 requirements

Issue: #32
Branch: `feature/32-traceability-restore-defensible-sys-mappings`
Spec path: `docs/history/specs/32-traceability-restore-defensible-sys-mappings.md`

## Problem

The approved requirements set does not currently provide a defensible
system-level parent for two already-implemented, clinically relevant software
requirements:

- `SWR-VIT-008` in `requirements/SWR.md` still traces to `SYS-003`, which is the
  body-temperature classification requirement and does not describe respiration
  rate behavior.
- `SWR-NEW-001` in `requirements/SWR.md` still traces to `SYS-018`, but
  `requirements/SYS.md` currently defines only `SYS-001` through `SYS-017`.
- `requirements/TRACEABILITY.md` attempts to compensate by tracing both SWRs to
  `SYS-005` and `SYS-006`, but those SYS entries cover alert-record generation
  and aggregate alert level only. They do not specify respiration-rate
  thresholds, "not measured" RR handling, AVPU input, or NEWS2 scoring.

The code and tests already implement these behaviors:

- `src/vitals.c` classifies respiration rate with dedicated thresholds and
  excludes `respiration_rate == 0` from aggregate alert escalation.
- `src/alerts.c` skips RR alerts when `respiration_rate == 0`.
- `src/news2.c` computes NEWS2 sub-scores, includes AVPU, and treats
  `respiration_rate == 0` as a zero RR sub-score so the absence of a sensor does
  not inflate the total.

This leaves the repository with a documentation-only traceability defect in a
medical-safety path. Prior discussion on issue `#12` and PR `#24` also shows
that a simple remap to `SYS-005` / `SYS-006` is not a durable resolution.

## Goal

Define a narrow design for restoring a valid requirements chain from approved
user needs, through explicit system requirements, down to `SWR-VIT-008` and
`SWR-NEW-001`, without changing implemented clinical behavior, thresholds, or
tests.

## Non-goals

- No production C source, headers, tests, CI workflows, or release artifacts
  change in this issue.
- No change to respiration-rate thresholds, NEWS2 scoring tables, AVPU handling,
  alert severity behavior, or dashboard runtime behavior.
- No broad rewrite of unrelated SYS, SWR, or RTM content.
- No decomposition of `SWR-VIT-008` or `SWR-NEW-001` into multiple new SWRs in
  this issue unless implementation discovers an unavoidable documentation
  blocker.
- No new user-need document entry is required unless the repo owner explicitly
  decides the current `UNS-005`, `UNS-006`, `UNS-010`, `UNS-014`, and
  `UNS-015` coverage is insufficient.

## Current behavior

Current implemented behavior is already specific and verified:

- `src/vitals.c` implements `check_respiration_rate()` with the threshold bands
  documented in `SWR-VIT-008`: `<= 8` or `>= 25` is critical, `9-11` or `21-24`
  is warning, and `12-20` is normal.
- `src/vitals.c` implements `overall_alert_level()` so `respiration_rate == 0`
  means "not measured" and is treated as `ALERT_NORMAL` for aggregate
  escalation.
- `src/alerts.c` generates an RR alert only when respiration rate is present and
  abnormal.
- `src/news2.c` implements NEWS2 sub-scoring for RR, SpO2, systolic BP, heart
  rate, temperature, and AVPU, then classifies overall NEWS2 risk from the
  total and any single-score `3`.
- `src/news2.c` also treats `respiration_rate == 0` as "not measured" and sets
  the RR sub-score to `0`.
- `tests/unit/test_vitals.cpp` and `tests/unit/test_news2.cpp` already verify
  these behaviors, so the gap is not implementation or verification coverage; it
  is missing and inconsistent SYS-level documentation.

Current requirements behavior is inconsistent:

- `requirements/SWR.md` uses one invalid or mismatched SYS parent for each SWR.
- `requirements/TRACEABILITY.md` uses a different pair of SYS parents that do
  not actually specify the governed behavior.
- `requirements/README.md` still shows `UNS-005 -> SYS-005 -> SWR-VIT-008` as
  an example trace, which reinforces the current non-defensible mapping.

## Proposed change

Implement the repair by adding explicit SYS requirements and then realigning the
downstream traces to those new entries. Do not repeat the prior
`SYS-005` / `SYS-006` remap.

### 1. Add a new SYS requirement for respiration rate behavior

Add `SYS-018` to `requirements/SYS.md` with language equivalent to:

`The system shall support respiration rate as a monitored vital sign. It shall
classify adult respiration-rate readings as NORMAL for 12-20 br/min, WARNING
for 9-11 or 21-24 br/min, and CRITICAL for <= 8 or >= 25 br/min. When a
respiration-rate reading is not available for a cycle, the system shall preserve
that "not measured" state and shall not raise a spurious RR-driven alert or
aggregate escalation from the missing value.`

Expected user-need trace anchors:

- `UNS-005` automatic abnormal value alerting
- `UNS-006` alert severity differentiation
- `UNS-014` graphical dashboard presentation
- `UNS-015` live monitoring feed

### 2. Add a new SYS requirement for NEWS2 clinical risk scoring

Add `SYS-019` to `requirements/SYS.md` with language equivalent to:

`The system shall compute and display a NEWS2 aggregate clinical risk score from
respiration rate, SpO2, systolic blood pressure, heart rate, temperature, and
AVPU using the approved Royal College of Physicians NEWS2 tables. The system
shall classify the resulting score into the implemented low / low-medium /
medium / high response bands and shall not inflate the score when respiration
rate is not measured for a cycle.`

Expected user-need trace anchors:

- `UNS-005` automatic abnormal value alerting
- `UNS-006` alert severity differentiation
- `UNS-010` consolidated status summary
- `UNS-014` graphical dashboard presentation

### 3. Repoint the affected SWRs to the new SYS entries

Update `requirements/SWR.md` so:

- `SWR-VIT-008` traces to `SYS-018`
- `SWR-NEW-001` traces to `SYS-019`

Keep the requirement bodies aligned with the current implemented behavior. This
issue does not authorize threshold or scoring edits.

### 4. Realign the RTM to the new SYS entries

Update `requirements/TRACEABILITY.md` so both the forward and backward
traceability rows for these SWRs reference the new SYS parents:

- `SWR-VIT-008 -> SYS-018`
- `SWR-NEW-001 -> SYS-019`

Also update the UNS coverage summary so the relevant user needs reflect the new
SYS entries without changing the overall coverage result.

### 5. Repair auxiliary requirements guidance

Update `requirements/README.md` so its example trace and maintenance guidance do
not continue to teach the stale RR mapping through `SYS-005`.

### 6. Keep revision history factual and narrow

Revision history entries in the touched requirements docs should state that the
change restores defensible SYS-level traceability for existing RR and NEWS2
requirements. They should explicitly note that no runtime clinical behavior was
changed.

## Files expected to change

Expected implementation files:

- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `requirements/README.md`

Files that should not change:

- `src/**`
- `include/**`
- `tests/**`
- `.github/workflows/**`
- release artifacts or generated documentation outputs

## Requirements and traceability impact

This issue directly changes approved requirements and the RTM in a
safety-sensitive monitoring path.

Impacted SWRs:

- `SWR-VIT-008`
- `SWR-NEW-001`

Impacted SYS layer:

- new `SYS-018` for respiration-rate monitoring and classification
- new `SYS-019` for NEWS2 aggregate clinical risk scoring

Impacted UNS coverage:

- `UNS-005`
- `UNS-006`
- `UNS-010`
- `UNS-014`
- `UNS-015`

No source `@req` tags, function names, or test IDs should change in this issue.
The purpose is to make the existing code/test evidence trace to accurate SYS
statements.

## Medical-safety, security, and privacy impact

Medical-safety impact is high from a documentation-governance perspective even
though runtime behavior does not change. The affected requirements govern:

- respiration-rate classification,
- alarm escalation behavior when RR is absent,
- NEWS2 scoring,
- AVPU contribution to early-warning risk,
- clinician-facing status presentation.

The implementation must preserve the current approved clinical behavior exactly.
If the repo owner wants different thresholds, different AVPU wording, or a
different NEWS2 response mapping, that is a separate requirements-change issue
and must not be folded into this traceability repair.

Security impact is none expected. Authentication, RBAC, password hashing, and
file-permission behavior are out of scope.

Privacy impact is none expected. No patient-data fields, storage paths, or
exports should change.

## Validation plan

Run targeted consistency checks after the documentation update:

```powershell
rg -n "SWR-VIT-008|SWR-NEW-001|SYS-003|SYS-005, SYS-006|SYS-018|SYS-019|NEWS2|respiration" requirements
```

Confirm manually that:

- `requirements/SYS.md` defines both `SYS-018` and `SYS-019`
- `requirements/SWR.md` no longer maps `SWR-VIT-008` to `SYS-003`
- `requirements/SWR.md` no longer maps `SWR-NEW-001` to a nonexistent SYS entry
- `requirements/TRACEABILITY.md` forward and backward rows for the two SWRs use
  the same new SYS IDs as `requirements/SWR.md`
- `requirements/README.md` no longer teaches `SYS-005` as the RR parent trace

No code test rerun is required for this issue because the approved design scope
is documentation-only and does not alter production or test logic. If the
implementation accidentally broadens beyond documentation, stop and return the
issue to design review instead of silently running with a larger scope.

## Rollback or failure handling

Rollback is documentation-only: revert the implementation commit if reviewers
reject the new SYS wording or the selected UNS trace anchors.

If implementation discovers that current approved UNS statements are considered
insufficient to support `SYS-018` or `SYS-019`, stop before editing the
requirements set further, comment on the issue with the gap, and move the item
back to `ready-for-design` rather than guessing.
