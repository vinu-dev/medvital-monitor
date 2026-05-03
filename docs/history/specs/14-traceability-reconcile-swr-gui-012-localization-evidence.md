# Design Spec: Issue 14

## Problem

Issue `#14` exposes a traceability gap around the already-implemented
localization feature. The repository contains active `SWR-GUI-012` references in
GUI code, configuration persistence APIs, DVT automation, and workflow
documentation, but the approved requirements set stops at `SWR-GUI-011` and the
RTM has no `SWR-GUI-012` row. The repo also claims automated localization test
coverage that does not currently exist in `tests/unit/`.

This leaves the implementation, requirement, and verification records out of
sync, which is unacceptable for IEC 62304-style change traceability.

## Goal

Formally reconcile localization traceability by documenting the existing
behavior as `SWR-GUI-012`, wiring that requirement into the RTM, and ensuring
the documented automated evidence matches the real test suite.

## Non-goals

- Adding new languages beyond the existing English, Spanish, French, and German
  support.
- Changing any localized clinical wording beyond what is required to align
  requirement text with current behavior.
- Reworking the localization architecture or replacing the Win32 GUI approach.
- Changing vital-sign classification, NEWS2, alarm limits, authentication, or
  any other clinical or security behavior unrelated to localization evidence.
- Removing the localization feature from production code.

## Current Behavior

- `src/localization.c` and `include/localization.h` implement a static
  four-language string table and language-selection API.
- `src/gui_main.c` contains a Settings language tab, applies the selected
  language, refreshes dashboard controls, and restores the saved language at
  startup. These flows are annotated with `@req SWR-GUI-012`.
- `include/app_config.h` and `src/app_config.c` provide
  `app_config_load_language()` and `app_config_save_language()` for persisting
  `language=N` in `monitor.cfg`, also annotated with `@req SWR-GUI-012`.
- `dvt/automation/run_dvt.py` records `DVT-GUI-16` against `SWR-GUI-012` by
  verifying that the language combo box exposes four options.
- `docs/DEVOPS_WORKFLOWS.md` claims a `LocalizationTest` suite with 8 tests for
  `SWR-GUI-012`.
- `requirements/SWR.md` has no `SWR-GUI-012` entry, and
  `requirements/TRACEABILITY.md` has no forward trace, backward trace, coverage
  summary, or test-count entry for that requirement.
- `tests/CMakeLists.txt` and `tests/unit/` do not define a localization test
  file today, so the documented unit-test evidence is inaccurate.

## Proposed Change

Adopt the implemented localization behavior as an approved requirement rather
than removing the feature. Implementation work should:

1. Add `SWR-GUI-012` to `requirements/SWR.md` as a narrow GUI requirement that
   covers:
   - Four supported languages.
   - Language selection from the Settings dialog.
   - Runtime dashboard/control text refresh after apply.
   - Persistence and restore of the selected language through `monitor.cfg`.
   - Static-storage/no-heap expectations for the localization layer.
2. Update `requirements/TRACEABILITY.md` so `SWR-GUI-012` appears in:
   - Forward traceability.
   - Backward traceability.
   - SWR coverage summary.
   - Test count summary.
3. Add real automated unit coverage for localization rather than leaving the
   workflow docs aspirational. The expected path is a new
   `tests/unit/test_localization.cpp` plus any minimal `tests/CMakeLists.txt`
   target update needed to compile it into `test_unit`.
4. Keep language-persistence tests aligned with the requirement boundary:
   - Pure string-table and API behavior belongs in `test_localization.cpp`.
   - `monitor.cfg` read/write behavior may stay in `test_config.cpp` if those
     cases are explicitly tied to `SWR-GUI-012`, or may move into the new test
     file if that yields cleaner ownership.
5. Correct `docs/DEVOPS_WORKFLOWS.md` so suite names and counts reflect the real
   test target output after the new evidence is added.
6. Leave production behavior unchanged unless testing exposes a genuine defect in
   the existing localization implementation. Any such defect should be handled
   in a follow-on implementation issue because this design is traceability-first.

## Files Expected To Change

- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `docs/DEVOPS_WORKFLOWS.md`
- `tests/CMakeLists.txt`
- `tests/unit/test_localization.cpp`
- `tests/unit/test_config.cpp` if language-persistence coverage is retained or
  expanded there

Production-source files are not expected to change for this issue:

- `src/gui_main.c`
- `src/app_config.c`
- `src/localization.c`
- `include/app_config.h`
- `include/localization.h`
- `dvt/automation/run_dvt.py` unless wording alignment is needed for consistency
  with the final requirement text

## Requirements And Traceability Impact

- Adds the missing approved requirement definition for `SWR-GUI-012`.
- Repairs the RTM so the requirement is traceable from implementation to
  verification evidence.
- Forces the documented automated evidence to match actual repository contents.
- Does not change SYS-level clinical thresholds or require new system
  requirements unless the human owner explicitly wants localization promoted to
  SYS scope. Default assumption: this remains a software/UI requirement only.

## Medical-Safety, Security, And Privacy Impact

- Medical safety: indirect only. This change improves auditability and reduces
  ambiguity around what UI text/localization behavior is approved, but it does
  not alter clinical logic, thresholds, alarms, NEWS2 scoring, or treatment
  guidance.
- Security: no intended change. Authentication and authorization paths are out
  of scope.
- Privacy: no intended change. No patient-data flows or persistence locations
  change.

## Validation Plan

- `rg -n "SWR-GUI-012|LocalizationTest|localization|language" requirements docs src include tests dvt`
- `rg --files tests | rg "localization|config"`
- Configure and build tests successfully after the new unit-test file is added.
- Run the unit-test target and confirm the localization cases execute and pass.
- Verify the RTM entries, SWR coverage summary, and test-count summary match the
  actual test suite names/counts.
- If DVT wording is adjusted, verify `DVT-GUI-16` still maps cleanly to the
  approved requirement text.

## Rollback Or Failure Handling

- If the team cannot define a narrow, stable `SWR-GUI-012` that matches the
  existing implementation, stop and escalate to the human owner before editing
  traceability artifacts further.
- If automated evidence proves too broad for one issue, prefer a minimal first
  pass that adds the requirement and corrects inaccurate workflow/RTM claims,
  then open a follow-up issue for expanded localization unit coverage.
- Do not "fix" the mismatch by silently deleting code annotations or DVT evidence
  unless the product owner explicitly decides to de-scope localization as a
  supported feature.
