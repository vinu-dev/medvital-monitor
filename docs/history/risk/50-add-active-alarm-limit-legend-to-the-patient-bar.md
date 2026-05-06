# Risk Note: Issue #50

Date: 2026-05-06
Issue: `#50` - Feature: add active alarm-limit legend to the patient bar

## proposed change

Issue `#50` proposes a dashboard-only UI change: show the currently configured
alarm limits in the patient bar so operators can confirm the cutoffs without
opening Settings.

The intended benefit is lower context switching during monitoring and handoff.
However, the issue currently describes those values as the "active" limits
while the current repository implementation does not use `g_app.alarm_limits`
to drive the live tile colors, aggregate status, or alert-generation path.
Today, the dashboard status path still comes from the fixed `check_*()`
classification functions in `src/gui_main.c` / `src/vitals.c`.

That makes the core risk question a source-of-truth problem, not just a layout
problem.

## medical-safety impact

This is nominally display-only work, but it sits on the clinician's primary
single-screen summary and therefore has meaningful indirect safety impact.

If the UI shows configured limits as if they are the live alert cutoffs when
the runtime alerting path still uses fixed thresholds, a clinician could:

- trust the wrong threshold during handoff or rapid reassessment
- misread a red/amber tile as inconsistent with the displayed legend
- delay escalation because the displayed "limit" appears wider than the actual
  alert threshold
- escalate unnecessarily because the displayed "limit" appears narrower than
  the actual alert threshold

For this pilot, the candidate is not safe to approve as written because the
word "active" is not yet defensible against the implemented monitoring logic.

## security and privacy impact

No material security or privacy change is expected if the feature remains
read-only.

The candidate does not add authentication, authorization, storage, export, or
network behavior. It would only expose already-configured alarm-limit values in
an additional on-screen location.

## affected requirements or none

- `UNS-010` Consolidated Status Summary
- `UNS-014` Graphical User Interface
- `SYS-011` Patient Status Summary Display
- `SYS-014` Graphical Vital Signs Dashboard
- `SWR-ALM-001` is related because it governs alarm-limit configuration and
  persistence, but it does not currently define a patient-bar legend or assert
  that those settings are the dashboard alerting source of truth
- A new or revised GUI-facing requirement would be needed before implementation
  so the display semantics are explicit and testable

## intended use, user population, operating environment, and foreseeable misuse

Intended use:
Show a quick summary of configured alarm-limit values on the live dashboard for
trained clinical users already monitoring a patient.

User population:
Ward clinicians and other trained staff using the Windows dashboard during
live monitoring, handoff, or review after a settings change.

Operating environment:
The existing desktop dashboard in simulation mode today, and potentially the
same patient-summary workflow in future hardware-connected deployments.

Foreseeable misuse:

- A user assumes the displayed legend is the threshold currently driving tile
  color, aggregate status, and alert escalation when it is not.
- A user treats the legend as a personalized "safe range" rather than a
  configured setting snapshot.
- A stale or partially updated legend is trusted during a settings change.
- The feature encourages users to stay on the dashboard instead of opening the
  authoritative settings view when confirmation is clinically important.

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Severity:
Moderate to serious, because misleading alarm-threshold presentation can
contribute to delayed recognition or incorrect prioritization even if it does
not directly change the underlying alert engine.

Probability:
Possible, because the issue explicitly frames the displayed values as "active"
cutoffs and the patient bar is a high-visibility summary surface.

Initial risk:
Medium-high.

Risk controls required before design approval:

- Obtain a human product/clinical decision on whether configurable alarm limits
  are intended to be authoritative for live alerting in this product.
- If the answer is "no", the UI copy must not call them "active" alarm
  cutoffs. It must present them as configured settings only.
- If the answer is "yes", this is no longer a display-only change. A separate
  approved behavior-change issue must align the alerting source of truth before
  any UI claim is made.
- Keep the patient-bar legend read-only. Do not allow in-place editing from the
  summary view.
- Render the displayed values from one authoritative source and define stale,
  unavailable, and device-mode behavior explicitly.
- Add explicit requirement text and verification evidence for the chosen
  semantics.

Verification method:

- Independent design review of the exact UI wording and source-of-truth claim
- Manual GUI smoke test after changing alarm limits in Settings and returning to
  the dashboard
- Negative review that checks for any contradiction between displayed legend
  text and the actual alert/color logic
- If future work makes limits authoritative, targeted unit/integration tests on
  the alerting path would be required under a separate issue

Residual risk:
Acceptable only if the feature is tightly bounded as a read-only configured
settings summary or if the product first changes the true alerting source of
truth under separate approval.

Residual-risk acceptability rationale:
Not acceptable as currently written, because the proposed "active limit" claim
can misrepresent current device behavior.

## hazards and failure modes

- The patient bar shows configured limits that do not match the thresholds that
  actually drive tile status and alert escalation.
- The legend remains stale after a settings change, patient reset, or device
  mode transition.
- The UI implies that configurable limits are patient-specific clinical targets
  rather than system settings.
- The summary bar becomes more authoritative-seeming than the actual settings
  screen without clear provenance.
- A design copies competitor phrasing or layout too closely and imports
  confusing alarm UX assumptions into this product.

## existing controls

- The current patient bar does not present alarm-limit values, which avoids this
  ambiguity today.
- The Settings window already contains a dedicated Alarm Limits tab for explicit
  editing and review.
- The current dashboard tile and aggregate-status path uses fixed vital-sign
  classification functions, which keeps one stable runtime alerting behavior.
- `alarm_limits_load()` / `alarm_limits_save()` already provide persisted values
  that can be shown if the product owner approves a read-only summary use.
- The cited Philips IFU material is public product-discovery evidence that alarm
  limits may be viewable in commercial monitoring systems, but it also
  reinforces that alarm settings are device-specific and safety-relevant.

## required design controls

- Human owner must decide whether the displayed values are:
  1. configured settings only, or
  2. the authoritative live alert thresholds
- The UI must use wording consistent with that decision. "Active" must not be
  used unless the runtime alert path truly uses those values.
- The legend must keep the existing app visual language and remain a simple text
  or chip summary; do not copy competitor patient-sector layout, iconography,
  alarm tones, or wording.
- The design must define behavior for no-patient, first-reading, simulation
  paused, and device-mode states.
- The design must define update timing after an Alarm Limits save so the
  dashboard cannot show stale values without an explicit stale indicator.
- The implementation issue must add or revise requirements and traceability
  before release evidence is claimed.

## validation expectations

- Review `src/gui_main.c` to confirm current patient-bar and tile logic before
  any wording is approved.
- Confirm the chosen legend text does not overstate what the values mean.
- Run the issue's proposed manual smoke test only after the source-of-truth
  semantics are approved:
  change a limit in Settings, return to dashboard, and confirm the legend
  updates correctly.
- If the future design keeps the feature display-only, verify that no alert
  engine, threshold, escalation, or NEWS2 behavior changes are included.
- If design later proposes that displayed limits are authoritative, stop and
  split that work into a separately approved behavior-change item.

## residual risk for this pilot

Residual risk is not acceptable for the current issue wording.

The candidate becomes acceptable only after the product owner narrows the claim:

- either the dashboard shows a clearly labeled configured-settings summary, or
- the organization explicitly approves a separate safety-reviewed change to make
  configurable limits the real alerting source of truth

## product hypothesis and intended user benefit

Hypothesis:
Showing alarm-limit values on the dashboard reduces context switching and makes
handoff confirmation faster.

Intended benefit:
Users can confirm settings without leaving the monitoring view.

Risk note:
That hypothesis is only safe if the displayed values are truthfully described.
The current issue does not yet establish that.

## source evidence quality

Source evidence quality is moderate for product-discovery purposes.

The cited Philips Instructions for Use are stronger than marketing pages because
they are manufacturer user documentation and they show that viewable alarm-limit
workflows exist in commercial monitors and information centers. They are still
competitor materials, not clinical evidence, and they do not justify copying a
proprietary layout or assuming the same source-of-truth model applies here.

They also reinforce that alarm settings are safety-relevant and device-specific,
which increases the need for precise wording in this issue.

## MVP boundary that avoids copying proprietary UX

- Keep the MVP to a compact, read-only summary rendered inside the repository's
  existing patient-bar style.
- Use generic parameter labels and units already present in this product.
- Do not copy competitor patient-sector arrangement, exact terminology,
  iconography, or alarm-message phrasing.
- Do not introduce printing, audit logs, alarm acknowledgement, or bedside-sync
  workflows in this issue.

## clinical-safety boundary and claims that must not be made

- Do not claim that the legend changes, governs, or validates live alarm
  classification unless the product actually does that.
- Do not present configured limits as personalized therapeutic targets or
  treatment advice.
- Do not imply that the feature reduces alarm fatigue or improves diagnosis
  without separate evidence.
- Do not let the feature become a substitute for reviewing the authoritative
  settings source when a human must confirm safety-critical parameters.

## whether the candidate is safe to send to Architect

No.

The issue should be blocked until a human owner clarifies whether configured
alarm limits are merely displayable settings or the authoritative live alert
thresholds. Architect should not be asked to design around an unsafe or
ambiguous "active limit" claim.

## human owner decision needed, if any

Yes.

The product owner or clinical owner must decide one of the following before the
issue returns to design:

- The dashboard will show a clearly labeled configured-settings summary only,
  with no claim that it drives live alert status, or
- The product intends configurable limits to become the actual live alerting
  source of truth, which requires a separate approved behavior-change issue and
  new risk/design review
