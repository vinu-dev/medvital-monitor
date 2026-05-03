# Design Spec: Traceability reconcile v2.7.0 SWR counts and missing requirement links

Issue: #12
Branch: `feature/12-traceability-reconcile-v2-7-0-swr-counts`
Spec path: `docs/history/specs/12-traceability-reconcile-v2-7-0-swr-counts.md`

## Problem

Issue #12 identifies internal inconsistencies in the approved v2.7.0
requirements and traceability documentation. The repository currently gives
different answers for how many SWRs exist, which SYS entries newer SWRs trace to,
and whether `SWR-GUI-011` is included in SWR coverage:

- `requirements/TRACEABILITY.md` revision G says `36/36 SWR, 287 tests`, while
  Section 4 says `35 / 35 SWRs implemented and tested`.
- `requirements/TRACEABILITY.md` Section 1 includes `SWR-GUI-011`, but Section 4
  omits it from the SWR coverage summary.
- `requirements/SWR.md` maps `SWR-NEW-001` to `SYS-018`, but
  `requirements/SYS.md` currently defines only `SYS-001` through `SYS-017`.
- `requirements/SWR.md` maps `SWR-VIT-008` to `SYS-003`, while
  `requirements/TRACEABILITY.md` maps it to `SYS-001`.
- `README.md` still reports `34 SWRs covered` and an RTM tree entry of
  `34/34 SWR, 287 tests`.

These are documentation and traceability defects. They should be corrected
without changing runtime behavior, tests, clinical thresholds, or production
code.

## Goal

Provide a narrow implementation plan that makes the v2.7.0 documentation give a
single, internally consistent account of:

- the current SWR count,
- the current automated and demonstration verification evidence,
- the intended SYS mappings for `SWR-VIT-008`, `SWR-NEW-001`, and
  `SWR-GUI-011`,
- the README summary counts and RTM description.

The implementation should preserve the issue's documentation-only intent and
produce a small, reviewable diff.

## Non-goals

- No production C source, headers, build scripts, CI workflows, release scripts,
  or generated Doxygen artifacts should change.
- No change to NEWS2 scoring, respiration-rate thresholds, alarm-limit defaults,
  alert severity, authentication, persistence, simulation behavior, or GUI
  runtime behavior.
- No new clinical requirement should be invented without explicit human-owner
  acceptance. If implementation determines that `SYS-018` must remain, a human
  owner must approve the new SYS entry before adding it to approved
  requirements.
- No broad restructuring of the requirements set or traceability matrix.

## Current behavior

The architecture keeps clinically relevant logic in the domain service layer
with presentation-layer behavior separated in `src/gui_main.c`. The impacted
issue is not a domain or GUI behavior defect; it is an evidence consistency
defect in the documentation layer.

Current requirement evidence:

- `requirements/SYS.md` defines `SYS-001` through `SYS-017`; there is no
  `SYS-018`.
- `requirements/SWR.md` defines 36 SWRs when counting:
  `SWR-VIT-001..008`, `SWR-NEW-001`, `SWR-ALT-001..004`, `SWR-PAT-001..006`,
  `SWR-GUI-001..011`, `SWR-SEC-001..004`, `SWR-ALM-001`, and `SWR-TRD-001`.
- `requirements/TRACEABILITY.md` Section 1 includes `SWR-GUI-011`, but Section 4
  lists only 35 SWRs and omits `SWR-GUI-011`.
- `requirements/TRACEABILITY.md` Section 6 totals 287 tests and states
  `33 SWRs (automated); 2 SWRs by GUI demo`, which does not account for all GUI
  demonstration-only requirements if the total SWR count is 36.
- `README.md` top-level and test sections already state 287 total tests, but
  its SWR coverage count remains stale at 34.

## Proposed change

Implement a documentation-only reconciliation with these decisions:

1. Treat 36 SWRs as the intended v2.7.0 total because `requirements/SWR.md`
   contains 36 software requirement entries and RTM revision G already records
   the v2.7.0 addition of `SWR-GUI-011`.
2. Add `SWR-GUI-011` to `requirements/TRACEABILITY.md` Section 4 with
   implementation `gui_main.c` and verification `GUI demo`, matching Section 1.
3. Update RTM Section 4 result text from `35 / 35` to `36 / 36`.
4. Update RTM Section 6 SWR evidence summary so the automated-vs-GUI-demo count
   accounts for all 36 SWRs. The implementation should count SWRs from the
   current rows rather than preserve stale prose.
5. Reconcile `SWR-NEW-001` by removing the nonexistent `SYS-018` reference unless
   the human owner explicitly approves adding a new `SYS-018`. The conservative
   default is to align `SWR-NEW-001` to existing alerting/aggregate risk
   requirements already used by RTM, `SYS-005` and `SYS-006`.
6. Reconcile `SWR-VIT-008` mapping by choosing one approved existing SYS mapping
   consistently across `requirements/SWR.md` and `requirements/TRACEABILITY.md`.
   Because respiration rate is a vital-sign classification and alert input, the
   implementation should avoid changing clinical thresholds and should document
   the rationale for the selected existing SYS mapping in the revision or nearby
   traceability text. If the human owner intends respiration rate to require a
   new system requirement, stop and request that acceptance before changing SYS.
7. Update `README.md` SWR coverage summaries and repository tree to match the
   reconciled RTM counts.
8. Keep the revision history concise and factual. If requirements documents are
   revised, add revision-history entries that describe traceability/count
   reconciliation only.

## Files expected to change

Expected implementation files:

- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `README.md`

Possible implementation file only if explicitly approved by the human owner:

- `requirements/SYS.md` if, and only if, the chosen resolution is to add an
  approved `SYS-018` instead of remapping `SWR-NEW-001` to existing SYS entries.

Files that should not change:

- `src/**`
- `include/**`
- `tests/**`
- `.github/workflows/**`
- generated documentation under `docs/html/**` or `docs/xml/**`

## Requirements and traceability impact

This change touches requirements and traceability documentation directly. It
must be handled as documentation reconciliation, not behavior expansion.

Impacted SWRs:

- `SWR-VIT-008`
- `SWR-NEW-001`
- `SWR-GUI-011`

Impacted documentation totals:

- RTM SWR coverage total should become consistent at 36/36.
- README SWR coverage totals should match the RTM.
- RTM test total should remain 287 unless implementation discovers actual test
  additions or removals, which are out of scope for this issue.

No source-level requirement tags or test names should change.

## Medical-safety, security, and privacy impact

Medical-safety impact is indirect but important. The issue touches vital
classification, NEWS2, alarm-limit-related traceability, and SWR coverage
evidence. The implementation must not alter:

- respiration-rate thresholds,
- NEWS2 scoring tables or risk categories,
- configurable alarm-limit defaults or behavior,
- alert generation or aggregate severity behavior.

The intended safety improvement is audit clarity: reviewers should not have to
infer whether NEWS2, respiration-rate display/classification, alarm limits,
trends, and simulation-mode messaging are covered by approved requirements and
verification evidence.

Security impact is limited to documentation consistency. Authentication,
password hashing, RBAC, file permissions, and persistence behavior are not in
scope and should not be changed.

Privacy impact is none expected. No patient data fields, storage behavior, logs,
exports, or identifiers should change.

## Validation plan

Run targeted text checks:

```powershell
rg -n "36/36|35 / 35|34 SWRs|34/34 SWR|SYS-018|SWR-GUI-011|SWR-VIT-008|SWR-NEW-001" requirements README.md
```

Review the resulting matches manually to confirm:

- no stale `34/34 SWR`, `34 SWRs covered`, or `35 / 35 SWRs implemented and
  tested` statements remain in current v2.7.0 summaries,
- `SWR-GUI-011` appears in both RTM Section 1 and Section 4,
- `SWR-NEW-001` no longer references a nonexistent SYS entry unless an approved
  `SYS-018` has been added,
- `SWR-VIT-008` uses the same approved SYS mapping in SWR and RTM.

Run the normal documentation-adjacent verification commands from the issue:

```powershell
run_tests.bat
python dvt/run_dvt.py --no-build --build-dir build
```

If local build artifacts are unavailable, document the limitation in the
implementation handoff and still complete the targeted `rg` validation.

## Rollback or failure handling

Rollback is documentation-only: revert the implementation commit if reviewers
reject the selected SYS mapping or coverage-count reconciliation.

If implementation cannot determine the correct approved SYS mapping for
`SWR-VIT-008` or `SWR-NEW-001`, do not guess by changing clinical intent. Comment
on the issue with the unresolved mapping choices and leave the issue in
`ready-for-design` until the human owner provides acceptance criteria.
