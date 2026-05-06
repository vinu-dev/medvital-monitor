# Risk Note: Issue #91 - Export Session Alarm Event Log to CSV

Date: 2026-05-06
Issue: #91 "Feature: export the session alarm event log to CSV"
Branch: `feature/91-export-session-alarm-event-log-to-csv`

## Proposed change

Add a local `Export Alarm Log` action to the session review surface so the
currently visible session alarm event rows can be written to a deterministic
UTF-8 CSV file in their visible order. The intended MVP is output-only,
local-only, and session-scoped. It must not change acquisition, alarm
thresholds, NEWS2 behavior, active-alert semantics, or background sync.

The issue description constrains the initial CSV shape to fields already visible
in review context: timestamp, severity/priority, source, and summary text.

## Product hypothesis and intended user benefit

Hypothesis: reviewers get more value from the existing session alarm event log
when they can hand off or archive the exact list they inspected instead of
retyping it or capturing screenshots.

Expected user benefit:

- preserves a review artifact for handoff and pilot evidence
- reduces manual transcription errors from the session review list
- keeps the workflow tied to the existing bounded alarm-review surface rather
  than inventing a new monitoring mode

## Source evidence quality

Evidence quality is adequate for a narrow product-discovery decision and not
adequate for broader clinical, enterprise, or retention claims.

- Mindray's BeneVision Alarm Data Server product material supports the product
  hypothesis that alarm-review data is sometimes exported as CSV and that alarm
  fields such as priority and timing are useful for analysis. This is vendor
  product evidence, not independent safety evidence.
- Mindray's BeneVision CMS operator-manual material supports the workflow idea
  that review pages and alarm-log export-to-path behavior exist in monitoring
  ecosystems. It is stronger than pure marketing for workflow realism, but it
  still does not justify copying central-station scope or feature breadth.
- Philips Clinical Insights IFU material supports that alarm-related patient
  data export is a real operator workflow, but it also reinforces that export
  scope can expand quickly into patient-info and multi-file behavior if the MVP
  is not kept narrow.

Conclusion: the sources are sufficient to justify a small, non-copying local
CSV export for the current session review list. They are not sufficient to
justify scheduled export, PDF output, multi-patient reporting, remote storage,
or inclusion of more patient-identifying data than the pilot strictly needs.

## Medical-safety impact

This change does not alter clinical classification logic, alarm limits, NEWS2,
or clinician-facing treatment guidance. The safety risk comes from context and
data integrity, not from new medical calculations.

Primary safety benefit:

- a reviewer can retain the exact reviewed alarm-history subset after the live
  state has changed

Primary safety risks:

- an exported CSV could be mistaken for a complete patient record or for the
  current active-alarm state
- an exported filtered subset could be treated as the full session history if
  filter state or session boundaries are not disclosed
- stale, wrong-patient, or simulation-mode rows could be handed off as if they
  were current real-device evidence
- malformed ordering, quoting, or partial writes could distort what the user
  believes they reviewed

Overall medical-safety impact: low-to-moderate if the feature remains a
read-only adjunct to the existing session review surface and makes its scope
plain in the file content and UX.

## Security and privacy impact

This is the main risk area. Unlike the existing in-app review log, a CSV export
creates a portable artifact that can persist outside the authenticated session
boundary and outside the application's normal clear/reset behavior.

- Exported files may contain patient-linked review data and can be copied,
  emailed, synced, or opened in third-party spreadsheet tools.
- The app already has a precedent for restricted local file creation:
  `users.dat` is written with owner-read/write-only permissions. Export should
  use the same best-effort restriction model where the platform allows it.
- The export should be user-initiated only. No scheduled, automatic, networked,
  or background export path belongs in this MVP.
- CSV fields must be quoted and escaped deterministically. Any text field that
  could begin with spreadsheet formula characters should be neutralized before
  writeout.
- The default export payload should exclude unnecessary identifiers. If patient
  name, ID, or other direct identifiers are added, that choice needs explicit
  requirements and product-owner acceptance.
- Write failures must surface clearly. The feature must not silently leave a
  truncated or stale file that appears complete.

## Affected requirements or "none"

Existing requirements likely affected:

- `UNS-017` Session Alarm Event Review
- `UNS-011` Data Integrity
- `UNS-013` User Authentication
- `SYS-020` Session Alarm Event Review Storage
- `SYS-021` Session Alarm Event Review Presentation
- `SYS-012` Static Memory Allocation
- `SWR-PAT-007` and `SWR-PAT-008` because export scope must stay aligned with
  the stored session alarm event model
- `SWR-GUI-013` because the exported data must match the visible review list

New derived requirements are likely needed for:

- a local export action and its allowed operating states
- deterministic CSV schema, ordering, encoding, and escaping rules
- disclosure of active filter state, session scope, and simulation/device mode
- overwrite, cancel, and write-failure behavior
- best-effort protected file creation and explicit exclusion of unsupported
  network/background export paths

No alert-threshold, NEWS2, or alarm-limit requirements should change for this
feature.

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained reviewers export the current session alarm review list as a local
  artifact for handoff or archive

User population:

- bedside clinicians, internal testers, and pilot reviewers using the Windows
  workstation application

Operating environment:

- a local authenticated desktop session with one active patient/session and the
  existing bounded session alarm event review surface

Foreseeable misuse:

- treating the CSV as a complete legal or clinical record
- treating a filtered export as the full session history
- exporting simulation-mode data and later presenting it as real clinical data
- modifying the CSV outside the app and treating the edited copy as
  authoritative system output
- saving the file to a shared or synced location without recognizing the
  privacy impact

## MVP boundary that avoids copying proprietary UX

The MVP should remain intentionally narrow:

- export the current visible session review rows only
- use only fields the pilot already owns and displays
- produce one deterministic local UTF-8 CSV artifact
- keep the action manual and local to the workstation session

The MVP should explicitly exclude:

- PDF reports, analytics dashboards, or scheduled exports
- multi-patient export, enterprise reporting, or central-station workflows
- cloud sync, email, EMR integration, or background archival
- alarm acknowledgment, lock status, clinician response tracking, or other
  workflow copied from competitor central-monitoring products

## Clinical-safety boundary and claims that must not be made

The exported CSV may support retrospective review. It must not claim to:

- replace the active alarm list or live monitoring display
- provide a complete longitudinal medical record
- prove that no events occurred outside the retained session/filter scope
- confirm clinician acknowledgment, escalation timing, or treatment action
- represent simulation-mode output as real-device clinical evidence

## Whether the candidate is safe to send to Architect

Yes, with constraints. The issue is specific enough to assess, does not change
clinical algorithms, and has adequate workflow evidence for a narrow export MVP.
Architect should keep the design focused on local file generation, explicit
scope metadata, minimal payload, and privacy controls.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a misleading or leaked export could cause handoff mistakes or privacy harm, even though the feature does not alter active alert generation. |
| Probability | Possible without controls because file export introduces a new persistence path and manual redistribution risk. |
| Initial risk | Medium |
| Key risk controls | Export only existing stored session event rows; preserve visible order; disclose filter state/session scope/simulation mode; use deterministic quoting and schema; require manual local save; apply best-effort restricted file creation; surface overwrite and write failures clearly. |
| Verification method | Unit tests for schema/order/escaping, integration checks for parity with the visible review list and reset boundaries, and GUI/manual review of cancel/error/overwrite paths plus simulation labeling. |
| Residual risk | Low-to-medium after controls because a user can still manually redistribute a valid exported file outside the app. |
| Residual-risk acceptability rationale | Acceptable for this pilot only if export remains a deliberate local action, the payload stays minimal, and the file itself states that it is a session-scoped review artifact rather than a complete medical record. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Export is mistaken for complete current truth | File omits filter state, session boundary, or active-vs-historical context | Reviewer underestimates recent or current risk and makes a poor handoff decision |
| Wrong context is exported | Previous patient rows, cleared-session rows, or simulation-mode rows are written without disclosure | Patient mix-up or false reliance on non-clinical evidence |
| Portable artifact leaks patient data | File is saved to a shared path, synced folder, or insecure location | Privacy breach and unauthorized disclosure |
| CSV content is corrupted or partially written | Ordering, quoting, or write completion is wrong | Reviewer archives an inaccurate record of the inspected events |
| Spreadsheet interpretation changes data | CSV text is treated as a formula or transformed on open | Security exposure or misleading displayed content |

## Existing controls

- The app already requires authentication before patient data is accessible.
- The existing session alarm event log is bounded, session-scoped, and derived
  from deterministic validated alert semantics.
- The current architecture is local-workstation oriented and does not require
  network or cloud transport for this feature.
- `users.dat` already demonstrates a protected local file creation pattern with
  owner-read/write-only permissions.
- The existing simulation mode is visible in the UI, which can be reused to
  prevent exported demo data from being mistaken for real monitoring evidence.

## Required design controls

- Use the existing stored session alarm event records as the only export source.
  Do not recalculate or reinterpret alert semantics during export.
- Export exactly the currently visible rows in their visible order. If filters
  are active, the export must disclose that the file is a subset.
- Include explicit metadata in the file or accompanying UX for export time,
  session scope, and simulation/device mode.
- Keep the default payload minimal. Do not add extra identifiers or patient
  location data unless explicitly approved in requirements.
- Use deterministic UTF-8 CSV formatting with fixed column order, quoting, and
  escaping rules.
- Neutralize spreadsheet formula execution risk for any text cell that could
  begin with `=`, `+`, `-`, or `@`.
- Make export manual, local, and cancelable. No background, scheduled, network,
  or cloud destinations in this MVP.
- Confirm overwrites and fail safely on write error. Prefer complete-file write
  semantics over partial output that looks successful.
- Apply best-effort protected file permissions similar to the existing
  `users.dat` handling where feasible on the target platform.

## Validation expectations

- unit tests for CSV column order, encoding, quoting, and formula-neutralization
- integration tests proving the exported rows match the visible session review
  list for both unfiltered and filtered states
- verification that patient/session reset boundaries and simulation/device mode
  are reflected correctly in export behavior
- GUI/manual tests for cancel, overwrite, unwritable path, and success/failure
  messages
- review evidence that no test artifacts containing real patient data are added
  to the repository or CI outputs

## Residual risk for this pilot

Residual risk remains that a human can deliberately or accidentally redistribute
an otherwise correct CSV outside the app. For this pilot, that residual risk is
acceptable only if export is explicit, local, minimally scoped, and clearly
labeled as review evidence rather than as the authoritative medical record.

## Human owner decision needed, if any

Product owner and architect should explicitly decide:

- whether the export includes any patient identifiers at all, and if so which
  minimum fields are justified
- whether simulation-mode exports need a conspicuous marker in the filename or
  file header
- how filtered-view context is disclosed so a subset export is not mistaken for
  the full session history
