# Risk Note: Issue #82 - Copy Current Patient Review Summary to Clipboard

Date: 2026-05-06
Issue: #82 "Feature: copy the current patient review summary to the clipboard"
Branch: `feature/82-copy-current-patient-review-summary-to-clipboard`

## Proposed change

Add a user-invoked clipboard action in the local desktop application that copies
the current patient review summary as deterministic plain text for handoff,
notes, or ticketing. The proposed MVP is intentionally narrow:

- manual user action only
- plain text only
- current patient session only
- reuse of existing summary content rather than new clinical calculations
- no file export, network sync, or background transmission

Repo observations supporting the change:

- `src/patient.c` `patient_print_summary()` already prints patient name, ID,
  age, BMI, reading count, latest vitals, current active alerts, and session
  alarm events in a deterministic format.
- `requirements/SWR.md` `SWR-PAT-006` already defines the required summary
  content and ties it to `SYS-011` and `SYS-021`.
- `docs/history/specs/37-session-alarm-event-review-log.md` explicitly kept
  summary evidence local, session-scoped, and free of new file/network
  persistence.
- `README.md` and `docs/ARCHITECTURE.md` show the product is a local Windows
  workstation application using static-memory production code, which fits a
  bounded clipboard workflow better than a new export subsystem.

## Product hypothesis and intended user benefit

Hypothesis: a one-step copy action reduces retyping and speeds local handoff or
note-taking because the operator can reuse the existing review summary instead
of manually transcribing current vitals, alerts, and session alarm history.

Expected user benefit:

- faster transfer of the current session summary into a local note or ticket
- lower manual transcription burden
- less formatting drift than hand-written free-text summaries

## Source evidence quality

Evidence quality is adequate for a narrow product-discovery decision and weak
for any clinical-effectiveness or workflow-safety claim.

- Philips Horizon Trends describes trend displays that support quick review of
  patient changes at a glance. This supports the general workflow value of
  concise review surfaces, not clipboard-specific behavior.
  Source: https://www.usa.philips.com/healthcare/product/HCNOCTN172/horizontrendstrendmeasurementdisplay
- Philips Device Management Dashboard IFU describes exporting monitor
  information to CSV. This shows that local export/handoff workflows exist in
  adjacent products, but it is a broader device-management context than this
  single-patient MVP.
  Source: https://www.documents.philips.com/assets/Instruction%20for%20Use/20251222/34228399dff04850bbdeb3bb01817112.pdf?feed=ifu_docs_feed
- Philips monitor configuration guidance describes report printing and stored
  print jobs. This supports the general hypothesis that monitor workflows often
  need human-readable output beyond the live screen, but it does not justify
  copying proprietary report UX or expanding into durable reporting here.
  Source: https://www.documents.philips.com/assets/Instruction%20for%20Use/20220223/51414b09bd404c5ba256ae4501365f69.pdf?feed=ifu_docs_feed

Conclusion: the sources are sufficient to justify a narrow, non-copying MVP for
manual text handoff. They are not sufficient to justify claims about clinical
benefit, enterprise documentation, or durable record export. The strongest
feasibility evidence is internal: the repo already has a deterministic summary
surface to reuse.

## Medical-safety impact

This change does not alter thresholds, NEWS2 scoring, alert generation, alarm
limits, or treatment guidance. Risk comes from context transfer and human use of
the copied text, not from new clinical logic.

Primary safety benefit:

- reduces manual retyping of the current summary, which may reduce omission or
  transcription error during local handoff or note capture

Primary safety risks:

- copied text may be pasted into the wrong patient context or the wrong system
- users may treat a copied snapshot as current live state after the patient has
  changed
- session alarm event text may be mistaken for current active alarms if the
  copied payload does not preserve the distinction clearly
- if copy behavior silently truncates, fails, or captures the wrong patient
  session, the user may trust inaccurate review text

Overall medical-safety impact: low-to-moderate if the feature remains a manual,
read-only adjunct to the existing live dashboard and active alert surfaces. It
is not acceptable if it is framed as a medical-record export or as a substitute
for current in-app review.

## Security and privacy impact

Privacy impact is the dominant risk.

- The existing summary includes direct identifiers and patient-linked
  observations. Copying that text moves it out of the authenticated application
  boundary and into the operating-system clipboard.
- Once placed on the clipboard, retention and onward exposure may depend on OS
  features such as clipboard history, remote desktop redirection, or clipboard
  sync settings. The app cannot assume the payload remains local-only after
  copy.
- The proposed change should not add any app-managed file persistence, network
  transmission, background sync, or hidden telemetry.
- The feature should remain manual and visible to the user. Silent automatic
  copying would create unnecessary privacy and workflow risk.
- The copied text must not imply that it is an official export, durable record,
  or complete longitudinal history.

## Affected requirements or "none"

No existing approved requirement explicitly authorizes clipboard export. If this
candidate proceeds, derive new requirements adjacent to:

- `UNS-010` Consolidated Status Summary
- `UNS-017` Session Alarm Event Review
- `SYS-011` Patient Status Summary Display
- `SYS-021` Session Alarm Event Review Presentation
- `SYS-012` Static Memory Allocation
- `SYS-013` User Authentication Enforcement
- `SWR-PAT-006` Patient Summary Display

Likely new derived requirements are needed for:

- explicit user invocation of the copy action
- exact payload content and whether direct identifiers are included
- clipboard failure handling and user feedback
- privacy disclosure for clipboard behavior outside the app session boundary
- verification that copied text remains aligned with the validated summary
  content

## Intended use, user population, operating environment, and foreseeable misuse

Intended use:

- trained clinicians, testers, or operators manually copy the current patient
  session summary into a local note, ticket, or handoff message

User population:

- authenticated users of the pilot desktop application on managed Windows
  workstations

Operating environment:

- local Windows GUI workflow, single current patient session, static in-process
  data, no app-managed cloud or file export in this issue

Foreseeable misuse:

- pasting the summary into the wrong patient note or ticket
- treating the copied text as a live view rather than a moment-in-time snapshot
- using the feature to move identifiable patient data into consumer messaging or
  unmanaged tools
- assuming the copied text is a complete medical record rather than a bounded
  pilot review artifact

## MVP boundary that avoids copying proprietary UX

The MVP should stay narrow and implementation-led:

- one explicit copy command from the existing patient review surface
- plain-text payload only, derived from the app's own summary data
- no CSV, PDF, printer workflow, report templates, or branded export layout
- no batch export, no multi-patient clipboard bundles, no background copy
- no EMR, HL7, email, chat, or network-delivery behavior
- no promise that the app manages downstream clipboard retention once the user
  copies the text

## Clinical-safety boundary and claims that must not be made

The feature may support manual handoff and note-taking. It must not claim to:

- diagnose, predict, or recommend treatment
- replace the live dashboard, active alerts, or clinician judgment
- provide a complete legal medical record
- guarantee privacy once copied text leaves the app boundary
- prove clinician acknowledgment, escalation, or documentation completion

## Whether the candidate is safe to send to Architect

Yes, with constraints. The candidate is safe to send to Architect because it
reuses existing review content and does not change clinical algorithms. The
design must treat the clipboard as an external privacy boundary, keep the action
explicitly user-initiated, and avoid implying durable or official export
semantics.

## Severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

| Item | Assessment |
| --- | --- |
| Severity | Moderate: a wrong-patient paste, stale snapshot, or privacy leak could contribute to handoff error or unauthorized disclosure, even though the feature does not alter monitoring logic. |
| Probability | Possible without controls because copy/paste workflows are easy to misuse and clipboard retention is partly outside the application. |
| Initial risk | Medium |
| Key risk controls | User-initiated action only; payload derived from the existing validated summary data; clear distinction between active alerts and session-history text; visible success/failure feedback; no app-managed persistence or transmission; explicit privacy disclosure that clipboard handling may outlive the app session. |
| Verification method | Unit or helper tests for deterministic summary payload generation if a new builder is added; GUI/manual tests for correct patient/session content and clipboard failure handling; regression checks that no thresholds or alerts changed; validation that copied text preserves active-alert vs session-history distinction. |
| Residual risk | Low-to-medium if controls are implemented and the deployment context manages clipboard exposure appropriately. |
| Residual-risk acceptability rationale | Acceptable for this pilot only if the copy action remains a manual adjunct, identifiable-payload policy is explicit, and operators are not led to treat the clipboard text as a durable or authoritative clinical record. |

## Hazards and failure modes

| Hazard | Failure mode | Potential harm |
| --- | --- | --- |
| Wrong-patient communication | User copies one patient's summary and pastes it into another patient's note or ticket | Patient mix-up, delayed or incorrect follow-up |
| Stale snapshot is treated as live state | Copied text is reused after additional readings or patient/session reset | Reviewer acts on outdated information |
| Historical events are mistaken for active alarms | Clipboard payload blurs the line between `Active Alerts` and `Session Alarm Events` | User misprioritizes current care response |
| Unauthorized disclosure of patient data | Clipboard contents persist in history, sync, or unmanaged tools | Privacy incident or policy breach |
| Incomplete or incorrect payload | Copy routine truncates, omits fields, or captures the wrong patient/session | Handoff or note contains misleading information |

## Existing controls

- Existing authentication and login gating already limit in-app access to
  patient data before copy.
- Existing summary content is deterministic and already defined by
  `SWR-PAT-006`.
- Existing session review semantics are bounded by current patient-session
  behavior and reset paths added for session alarm event review.
- Existing production code avoids heap allocation, which supports a bounded
  plain-text implementation.
- Existing live dashboard and active-alert surfaces remain available as the
  primary current-state view.

These controls reduce implementation risk, but they do not by themselves manage
clipboard privacy or wrong-destination paste risk.

## Required design controls

- Make copy an explicit user action only. No automatic copy on timer, discharge,
  alert change, or logout.
- Keep the payload deterministic plain text derived from existing summary data.
  Do not introduce a second clinical-summary code path with different logic.
- Preserve explicit section labels such as current status, active alerts, and
  session alarm events so copied text cannot easily be read as a single
  undifferentiated alarm state.
- Provide clear success/failure feedback when clipboard access succeeds or is
  unavailable.
- Do not add app-managed file export, print workflow, network delivery, or
  background caching in this issue.
- Define whether patient name and ID are included by default, redacted by
  default, or controlled by role/policy. This decision should be explicit
  before implementation.
- Document that clipboard handling outside the app may persist beyond logout or
  patient clear, and do not imply that the app can revoke pasted or synced
  copies.

## Validation expectations

- manual GUI smoke check that the copied text pastes into a plain-text target
  and matches the intended patient/session summary
- regression check that copied content preserves the distinction between active
  alerts and session alarm events
- tests for any helper used to build clipboard text so formatting stays
  deterministic and aligned with `patient_print_summary()`
- negative-path validation for clipboard access failure so the app does not
  report success when copy did not occur
- confirmation that no new heap allocation, file persistence, or network path
  was added for this MVP

## Residual risk for this pilot

Residual risk remains that authenticated users can intentionally or accidentally
move identifiable patient text into destinations the application does not
control. For this pilot, that risk is acceptable only if the workflow stays
manual, narrow, and explicit about clipboard limitations.

## Human owner decision needed, if any

Product owner, architect, and deployment owner should explicitly decide:

- whether copied text may include direct identifiers by default
- whether pilot workstations must disable clipboard history or clipboard sync
  before identifiable payloads are allowed
- whether the MVP needs a redacted copy option now or can defer it
- whether the copied summary should be positioned as note-assist only rather
  than as an official export artifact
