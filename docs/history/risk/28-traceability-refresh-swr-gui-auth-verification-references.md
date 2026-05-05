## Risk Note: Issue 28

Issue: `#28`  
Branch: `feature/28-traceability-refresh-swr-gui-auth-verification-references`

## proposed change

Update the verification text in `requirements/SWR.md` for the GUI
authentication and GUI display requirements so it matches the current approved
traceability evidence. The intended scope is documentation and evidence
reconciliation only:

- `SWR-GUI-001` should cite the current `tests/unit/test_auth.cpp` evidence
  names `UsersTest.REQ_GUI_001_*` instead of stale `AuthValidation.*`.
- `SWR-GUI-002` should cite `UsersTest.REQ_GUI_002_*` instead of stale
  `AuthDisplayName.*`.
- `SWR-GUI-003` should stop claiming structural verification by
  `AuthValidation.*` and align with the RTM's GUI-demonstration evidence for
  colour-tile and banner painting behavior.

No production code, test logic, credentials, session flow, or GUI rendering
behavior should change.

## medical-safety impact

No direct runtime, clinical-threshold, alerting, NEWS2, patient-record, or
device-integration behavior change is proposed. The safety impact is indirect
but real: stale or overstated verification references can cause reviewers to
misjudge which authentication and GUI behaviors are actually covered by
automated evidence versus manual GUI evidence. That weakens change-impact review
and audit confidence, but it does not itself change bedside behavior. Overall
medical-safety impact is low if implementation remains documentation-only.

## security and privacy impact

No security-control behavior change is intended. Authentication, credential
storage, role enforcement, and logout behavior remain in scope only as evidence
references, not as code changes. The main security concern is integrity of the
verification record: inaccurate auth evidence could mislead reviewers about what
login and session behavior is actually tested. Privacy impact is none expected
because no patient-data collection, persistence, export, or access path changes
are requested.

## affected requirements or "none"

- `SWR-GUI-001`
- `SWR-GUI-002`
- `SWR-GUI-003`

These are affected only in their verification-text references. The underlying
`SYS-013` and `SYS-014` behavioral intent should remain unchanged.

## hazards and failure modes

- Reviewers follow stale suite names that no longer represent the approved auth
  evidence and conclude the repository lacks traceable verification.
- Documentation overstates automated evidence by implying auth unit tests
  structurally verify GUI tile and banner painting behavior.
- An implementer expands a documentation fix into auth, session-management, or
  GUI-rendering code changes to make stale references appear true.
- SWR text and RTM rows diverge again, leaving conflicting evidence claims for
  the same requirements.

## existing controls

- The issue body explicitly states there is no runtime or clinical behavior
  change.
- `requirements/TRACEABILITY.md` already maps `SWR-GUI-001` to
  `UsersTest.REQ_GUI_001_*`, `SWR-GUI-002` to `UsersTest.REQ_GUI_002_*`, and
  `SWR-GUI-003` to GUI-demo evidence.
- `tests/unit/test_auth.cpp` contains the actual approved GUI-auth evidence IDs
  and shows that `AuthValidation.*` / `AuthDisplayName.*` are stale names.
- The architecture separates GUI auth and presentation concerns from the
  clinically relevant domain logic, limiting patient-facing impact if scope is
  contained.

## required design controls

- Restrict implementation to requirements and traceability documentation; do not
  modify `src/**`, `include/**`, `tests/**`, or release artifacts for this
  issue.
- Treat `requirements/TRACEABILITY.md` and `tests/unit/test_auth.cpp` as the
  canonical evidence sources when updating `requirements/SWR.md`.
- Preserve `SWR-GUI-003` as GUI-demonstration evidence unless a human owner
  explicitly approves new automated GUI verification.
- Review the changed file list and diff to confirm the implementation stays
  documentation-only and does not alter authentication semantics or GUI logic.
- Keep implementation notes explicit that this is evidence reconciliation for
  existing approved behavior, not a redefinition of the auth or GUI
  requirements themselves.

## validation expectations

- Run the issue's targeted text searches and confirm `requirements/SWR.md`,
  `requirements/TRACEABILITY.md`, and `tests/unit/test_auth.cpp` agree on the
  current evidence names for `SWR-GUI-001` and `SWR-GUI-002`.
- Confirm `SWR-GUI-003` no longer claims `AuthValidation.*` as structural
  evidence and remains aligned to GUI-demonstration verification.
- Inspect the changed file list and diff during implementation to verify
  documentation-only scope.
- No product test rerun is required for this risk conclusion because executable
  behavior is unchanged; the relevant validation is evidence consistency across
  approved documentation and the existing test suite.

## residual risk for this pilot

Low. Residual risk is limited to human documentation transcription error or a
missed stale reference, not to patient-facing behavior. With the controls
above, this issue is appropriate to move forward as a low-risk
traceability/evidence-correction item.
