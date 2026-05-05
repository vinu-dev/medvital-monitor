# Risk Note: Issue #32

Date: 2026-05-05
Issue: `#32` - Traceability: restore defensible SYS mappings for RR and NEWS2 requirements

## proposed change

Add or approve defensible SYS-level parent requirement coverage for
`SWR-VIT-008` and `SWR-NEW-001`, then align `requirements/SWR.md` and
`requirements/TRACEABILITY.md` to that approved system-level intent.

This issue should remain documentation-only. It should not change runtime
behavior, clinical thresholds, NEWS2 scoring logic, alert generation, or user
workflow.

## medical-safety impact

No direct patient-facing behavior should change if the work stays within the
documentation-only scope.

The safety significance is indirect but material because respiration-rate
classification and NEWS2 scoring both sit on the deterioration-alerting path.
Incorrect or missing SYS traceability can weaken change-impact analysis,
requirements review, and future verification for clinically relevant logic.

If the design effort concludes that new clinical intent must be introduced, that
is no longer a documentation cleanup. It becomes a human-approved requirements
change that must be handled explicitly before implementation.

## security and privacy impact

No security or privacy change is expected from the proposed documentation-only
update.

No authentication, authorization, patient-data handling, persistence, or
logging behavior should change.

## affected requirements or none

- `UNS-005` Automatic Abnormal Value Alerting
- `UNS-006` Alert Severity Differentiation
- `UNS-010` Consolidated Status Summary
- `SWR-VIT-008` Respiration Rate Check
- `SWR-NEW-001` NEWS2 Aggregate Clinical Risk Score
- Current mismatched or incomplete system traceability in `requirements/SYS.md`,
  `requirements/SWR.md`, and `requirements/TRACEABILITY.md`
- Existing documented evidence in issue `#12` and PR `#24`

## hazards and failure modes

- Future reviewers may miss the full impact of RR or NEWS2 changes because the
  software behavior does not trace to a defensible SYS requirement.
- A documentation repair may incorrectly reuse unrelated SYS entries such as
  alert-record generation or aggregate alert level, masking the defect instead
  of controlling it.
- A later change could alter RR thresholds or NEWS2 behavior under the false
  assumption that the work is documentation-only.
- Audit or release review may incorrectly conclude that the clinical alerting
  path is fully justified when the system-level rationale is still incomplete.

## existing controls

- `requirements/SWR.md` already defines the intended RR threshold behavior and
  NEWS2 scoring behavior at the software-requirement level.
- Automated verification already exists for both impacted SWRs in
  `tests/unit/test_vitals.cpp` and `tests/unit/test_news2.cpp`.
- Implemented behavior already exists in `src/vitals.c` and `src/news2.c`.
- Issue `#12` and PR `#24` already record that remapping these SWRs to
  unrelated SYS entries is not a defensible resolution.
- Issue `#32` explicitly constrains scope to no runtime or threshold change.

## required design controls

- Add or revise approved SYS requirement text so it explicitly covers
  respiration-rate classification and NEWS2 aggregate scoring before any final
  SWR or RTM remap is merged.
- Keep this item documentation-only. Split any code, threshold, scoring, or
  workflow change into a separate human-approved issue.
- Use one consistent approved mapping across `SYS.md`, `SWR.md`,
  `TRACEABILITY.md`, and any summary documentation.
- Require independent review of the resulting system-level wording for clinical
  coherence and traceability completeness.
- Confirm that no other SWRs depend on the same stale or nonexistent SYS link.

## validation expectations

- Run targeted text checks for `SWR-VIT-008`, `SWR-NEW-001`, `SYS-018`,
  `SYS-005`, and `SYS-006` across requirements documents and `README.md`.
- Manually confirm that the chosen SYS entry or entries actually specify RR and
  NEWS2 behavior, rather than adjacent alerting behavior only.
- Confirm the change set stays documentation-only and does not modify
  `src/**`, `include/**`, `tests/**`, or release artifacts.
- If any executable behavior changes are proposed during design, stop and obtain
  explicit human approval before implementation proceeds.

## residual risk for this pilot

Residual risk is low if the team performs an approved documentation and
traceability repair only, with independent review of the SYS-level wording.

Residual risk becomes medium if the team again treats an unrelated SYS remap as
an acceptable closure, because that would preserve a clinically relevant
traceability defect in the alerting path.
