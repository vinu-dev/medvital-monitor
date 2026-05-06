# Risk Note: Issue #59

Date: 2026-05-06
Issue: `#59` - Feature: add clinician session auto-lock after inactivity
Branch: `feature/59-clinician-session-auto-lock-after-inactivity`
Note path: `docs/history/risk/59-clinician-session-auto-lock-after-inactivity.md`

## proposed change

Add an inactivity-based clinician session lock for shared bedside workstations.
After a conservative period with no qualifying user interaction, the GUI should
hide patient data and require re-authentication before the dashboard can be used
again.

This should be treated as an access-control feature, not as a clinical-monitor
feature. The design must not change vital-sign acquisition, alert thresholds,
NEWS2 logic, alarm generation, or patient-record integrity.

## product hypothesis and intended user benefit

The product hypothesis is credible: shared bedside terminals can remain logged
in after the prior operator walks away, creating an avoidable privacy and
unauthorised-use risk. A bounded inactivity lock should reduce that exposure
without forcing clinicians to change how they monitor vitals during active use.

The intended user benefit is operational and security-focused:

- reduce unattended access to patient data
- reduce the chance that the next user acts under the previous user's session
- preserve the existing authentication and RBAC model rather than introducing a
  new identity system

## source evidence quality

Source evidence quality is medium for product-discovery purposes and low for
clinical-safety proof.

- The issue cites public manufacturer-hosted manuals/IFUs from Drager and
  Mindray, which is better provenance than a blog post or marketing page.
- Those sources support the general product hypothesis that session login/logout
  controls are first-class workstation concerns in bedside-monitoring products.
- They do not, by themselves, justify a specific idle-timeout value, countdown
  policy, lock-screen behavior, or residual-risk acceptability.
- Retrieval of the linked PDFs was not reliable from this environment during
  this run, so this note treats the issue-cited links as unverified but
  plausibly authoritative product-context sources, not as independent clinical
  evidence.

## MVP boundary that avoids copying proprietary UX

The MVP should stay narrow:

- fixed conservative timeout only in this issue
- reset on explicit local user interaction
- obscure patient data and require re-authentication before revealing the
  dashboard again
- no SSO, badge tap, biometric unlock, network identity, or password-policy
  change
- no user-configurable timeout in this issue; configurability is a separate
  product decision and should not be folded into the MVP
- no copying of vendor-specific timeout values, wording, layouts, or warning
  flows from competitor products

## clinical-safety boundary and claims that must not be made

This candidate is not a clinical-feature change and must not be marketed or
documented as improving diagnosis, treatment, or patient outcomes.

The design must not claim that the inactivity lock:

- improves physiologic monitoring accuracy
- verifies staff presence at the bedside
- replaces manual logout discipline
- by itself establishes HIPAA, IEC 62304, or security-compliance sufficiency

The design must also not suppress alarms, alter patient data, or change the
clinical interpretation path while the screen is locked.

## whether the candidate is safe to send to Architect

Yes, with constraints.

It is safe to send to Architect if the design is bounded to access control and
the architecture explicitly separates inactivity lock from the current explicit
logout path. In the current code, explicit logout clears `g_app.patient`,
resets session state, and destroys the dashboard window. An inactivity lock
must not inherit that data-destruction behavior unless a human owner later
approves a materially different workflow.

## medical-safety impact

The direct intended effect is beneficial from a privacy and misuse perspective,
but there is a real indirect patient-safety risk if the timeout locks an
actively observed station simply because the clinician is reading rather than
typing or clicking.

The principal medical-safety concern is workflow interruption:

- a lock that obscures patient data too aggressively can delay situation
  awareness or response
- a design that reuses the current logout behavior could clear session context
  or interrupt monitoring workflow
- a lock that hides active-alert context without preserving alarm continuity can
  create confusion during handoff or escalation

This keeps the feature in a medium-risk design area even though it does not
change clinical algorithms.

## security and privacy impact

Security and privacy impact is positive if implemented correctly.

- It reduces shoulder-surfing and unattended-session exposure.
- It reduces the chance of the wrong clinician acting under a stale session.
- It depends on the existing local authentication path, hashed credentials, and
  role enforcement, so it does not widen the auth surface by itself.

Security risk appears if unlock flow shortcuts re-authentication, leaves stale
user identity visible, or restores access without a full credential check.

## affected requirements or none

- `UNS-013` User Authentication
- `UNS-016` Role-Based Access and Multi-User Support
- `SYS-013` User Authentication Enforcement
- `SYS-017` Role-Based Access Control
- `SWR-GUI-002` Session Management (Login / Logout)
- `SWR-SEC-001` Multi-User Authentication with Role Detection
- `SWR-GUI-008` Role-Based UI Differentiation
- likely new SYS/SWR text for inactivity lock behavior, because current
  requirements describe explicit login/logout but not automatic locking after
  inactivity

## intended use, user population, operating environment, and foreseeable misuse

Intended use remains unchanged: trained clinical staff use the application on a
Windows workstation to observe monitored patient data and manage a local
authenticated session.

Relevant operating context:

- shared bedside or ward workstations
- intermittent direct interaction with periods of passive observation
- local credential-based login with persistent user accounts

Foreseeable misuse and edge conditions:

- clinician walks away without logging out
- second user approaches an unattended unlocked terminal
- clinician is still present and observing the screen but provides no input long
  enough to trigger the timeout
- users assume auto-lock is a substitute for explicit logout or for broader
  organisational access-control policy

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

- Severity: moderate for workflow interruption and privacy breach; potentially
  serious if lock behavior obscures active patient status during a time-critical
  review.
- Probability: occasional without careful timeout selection and interaction
  design, because passive observation without keyboard or mouse activity is
  foreseeable in this environment.
- Initial risk: medium.
- Risk controls:
  - separate inactivity lock from explicit logout and do not clear patient data
    or session-owned monitoring context on timeout
  - require full re-authentication before patient data is shown again
  - reset the idle timer on all approved interaction paths
  - use a conservative default timeout approved by a human owner
  - provide a clear visual warning or grace period before lock if the final UX
    would otherwise interrupt passive observation too abruptly
  - preserve alarm continuity and make the locked-state behavior explicit in the
    design
  - keep timeout configurability out of this issue unless requirements are
    expanded and re-reviewed separately
- Verification method:
  - targeted unit tests for timer reset and timeout transition logic
  - GUI/manual verification that active interaction prevents lock
  - GUI/manual verification that inactivity triggers lock and requires
    re-authentication
  - regression verification that explicit manual logout still clears session
    state while inactivity lock does not
  - verification that patient context, alerts, and monitoring continuity behave
    per approved design during and after relock
- Residual risk: low to medium if the above controls are implemented and the
  timeout default is human-approved.
- Residual-risk acceptability rationale: acceptable for this pilot only if the
  feature remains access-control scoped, does not alter clinical logic, and
  independently verifies that locked-state behavior cannot silently destroy or
  suppress clinically relevant context.

## hazards and failure modes

- Unattended unlocked workstation exposes patient data to unauthorised viewers.
- Unattended unlocked workstation allows actions under the wrong user identity.
- Idle timeout fires while the clinician is still actively observing without
  interacting, obscuring patient status at the wrong time.
- Timeout implementation reuses the current logout path and clears patient
  context or monitoring session data.
- Unlock flow restores the dashboard without re-authentication.
- Timer does not reset on all relevant interaction paths, causing false locks.
- Timer resets too broadly and fails to lock when the workstation is truly
  unattended.

## existing controls

- The application already requires authentication before dashboard access under
  `SYS-013`.
- Explicit logout already returns the user to the login screen and clears
  session data.
- User accounts, role assignment, and password hashing already exist in
  `gui_users.c`, `gui_auth.c`, and `pw_hash.c`.
- The codebase already has a lightweight GUI-policy persistence surface in
  `app_config.c`, though this issue does not require timeout configurability.
- The issue body already constrains scope away from alert-threshold, NEWS2, and
  patient-data changes.

## required design controls

- Define inactivity lock as a distinct state transition, not a synonym for
  manual logout.
- Explicitly document whether patient context remains in memory, whether data
  acquisition continues, and how alerts behave while the screen is locked.
- Add or revise requirements so timeout behavior, re-authentication, and
  locked-state safety boundaries are testable.
- Keep the MVP to a fixed conservative timeout and avoid configuration creep.
- Ensure the lock screen does not reveal patient-identifiable data.
- Require independent review of the chosen default timeout and warning/grace
  behavior before implementation is approved.

## validation expectations

- Requirements update adding explicit inactivity-lock acceptance criteria.
- Unit coverage for timer reset, timeout expiry, and relock transition paths.
- Manual GUI scenario:
  - log in
  - leave the station idle past the timeout
  - confirm patient data is hidden and re-authentication is required
  - confirm the approved monitoring/session context behavior is preserved
- Negative GUI scenario:
  - maintain intermittent activity and confirm the session does not false-lock
- Regression verification that clinical calculations, alert thresholds, and
  explicit logout behavior remain unchanged.

## residual risk for this pilot

Residual pilot risk is acceptable if the final design preserves monitoring
continuity, avoids patient-data loss, and uses a human-approved conservative
timeout.

Residual pilot risk is not acceptable if inactivity timeout is implemented by
calling the current manual logout behavior, because that would blur an
access-control control into a workflow-disrupting session-destructive action.

## human owner decision needed, if any

Yes.

The product owner or clinical owner should explicitly approve:

- the default inactivity timeout value
- whether a pre-lock warning or grace period is required
- whether lock is permitted during passive observation with no input
- the approved behavior for active alerts and patient-context continuity while
  the screen is locked
