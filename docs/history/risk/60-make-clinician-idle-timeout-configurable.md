## Risk Note: Issue 60

Issue: `#60`  
Branch: `feature/60-make-clinician-idle-timeout-configurable`

## proposed change

Add an administrator-configurable idle timeout for clinician session auto-lock
or auto-logout behavior, using the existing local settings and config
persistence model. The proposed scope is limited to access-control behavior and
must not change alarm logic, vital-sign calculations, patient-data storage
format, or authentication factors.

Repository review on 2026-05-06 found existing login/logout, role-based
settings access, and `monitor.cfg` persistence, but did not find an existing
idle-timeout requirement or a visible idle-timeout implementation in
`src/**`, `include/**`, or `requirements/**`. Treat this issue as a new
session-control feature, not as a pure exposure of an already traced constant.

## product hypothesis and intended user benefit

The product hypothesis is credible: a fixed idle-timeout value can be too short
for slower review workflows or too permissive for shared ward workstations.
Allowing an administrator to configure the value can reduce repeated workflow
interruptions while still limiting unattended patient-data exposure. The
intended user benefit is local policy flexibility without changing any clinical
interpretation, thresholds, alarms, or monitoring math.

## source evidence quality

Source evidence quality is moderate for the narrow claim that clinician
authentication and session timeout/logout controls are normal product behavior
in bedside-monitoring ecosystems, and low for choosing a safe timeout range or
proving usability benefit in this repository.

The issue cites public vendor material from:

- Drager Vista IFU, which describes clinician auto logout when the configured
  auto-logout time is reached.
- Mindray public operator-manual resources showing clinician login/logout and
  session-control flows in monitoring-viewer products.

These sources are acceptable as untrusted product-discovery context only. They
are not independent clinical evidence, not a substitute for local usability
validation, and not a basis for copying vendor UX.

## MVP boundary that avoids copying proprietary UX

The MVP should use a simple local control in the existing settings surface,
such as a numeric minutes field or constrained drop-down, persisted through the
same local config mechanism already used for `monitor.cfg`. The design should
avoid copying competitor screen layout, terminology, icons, warning copy, or
workflow choreography. The MVP should exclude per-user policy, remote policy
sync, fleet management, analytics, and any claim of enterprise access-control
completeness.

## clinical-safety boundary and claims that must not be made

This change is an access-control feature, not a clinical feature. It must not
be described as improving diagnosis, triage, alarm detection, or treatment
quality. It must not alter alarm thresholds, suppress alarms, pause monitoring,
change vital-sign acquisition cadence, or silently change patient context. It
also must not be positioned as a complete HIPAA or security-compliance
solution without broader system controls and validation.

## whether the candidate is safe to send to Architect

Yes, with constraints. The issue is specific enough for architecture and design
work because it has a bounded user problem, an explicit non-clinical scope, and
clear repo touchpoints in authentication, settings, and config persistence.
However, the design spec must make the timeout semantics explicit instead of
assuming that a new setting can simply reuse any current behavior.

## medical-safety impact

Medical-safety impact is indirect but real. A timeout that is too short can
interrupt clinician review, force unnecessary re-authentication, and delay
access to patient information during a monitoring workflow. A timeout that is
too long can leave patient information visible on an unattended shared device.
Because the requested change does not alter clinical calculations or alarm
evaluation, the main harm mode is delayed access, confusion, or privacy
exposure rather than direct treatment error. The feature remains suitable for
design if the timeout range, default, privilege boundary, and expiry behavior
are explicitly constrained.

## security and privacy impact

The intended security effect is positive if the feature enforces a sensible
default and a bounded maximum duration. The new risks are:

- a mis-set or invalid timeout value weakens session protection
- a clinician-accessible setting weakens role-based access control
- weak load/save validation allows `monitor.cfg` values that disable or bypass
  the intended control

Privacy impact is meaningful because unattended sessions can expose patient
identifiers and current monitoring context on shared workstations. No new data
collection, export, or sharing path is justified by this issue.

## affected requirements or "none"

Likely affected existing intent:

- `UNS-013`
- `UNS-016`
- `SYS-013`
- `SYS-016`
- `SYS-017`
- `SWR-GUI-002`
- `SWR-GUI-008`
- `SWR-GUI-009`
- `SWR-GUI-010` if the new value is persisted in `monitor.cfg`

New or updated requirement text will likely be needed for idle-timeout
behavior, bounds, persistence, and RBAC. The issue should not rely only on
informal extension of existing auth/settings prose.

## intended use, user population, operating environment, and foreseeable misuse

Intended use: allow an authorized ward administrator to configure how long a
logged-in session may remain idle before the UI requires re-authentication.

User population: ward administrators who manage local policy, and bedside
clinicians who are subject to the configured timeout.

Operating environment: shared local workstation or monitor terminal in a ward
or bedside environment, where users may step away and multiple staff may share
the same device.

Foreseeable misuse:

- setting the timeout too low for normal workflow and leaving it in production
- setting the timeout to an effectively disabled value if bounds are weak
- assuming the feature protects an unattended device without other local
  physical and operational controls
- assuming the feature changes alarms or patient monitoring behavior

## severity, probability, initial risk, risk controls, verification method, residual risk, and residual-risk acceptability rationale

Primary access-interruption hazard: timeout expires during active but low-input
review, causing workflow delay.

- Severity: moderate
- Probability before controls: occasional
- Initial risk: medium
- Required controls: bounded range, safe default, clear units, explicit expiry
  behavior, and validation that inactivity detection matches the intended use
- Verification method: unit tests for bounds and persistence, GUI tests for
  admin-only editing, and manual idle-expiry walkthroughs
- Residual risk: low to medium-low

Primary privacy hazard: timeout is too long or invalid, leaving patient data
visible on an unattended device.

- Severity: moderate
- Probability before controls: occasional
- Initial risk: medium
- Required controls: enforced minimum/maximum duration, reject invalid config
  values on load, and no "disable timeout" MVP option without human approval
- Verification method: persistence tests, negative tests for invalid values,
  and manual unattended-session expiry checks
- Residual risk: low

Residual-risk acceptability rationale: acceptable for this pilot if the feature
remains local, admin-controlled, bounded, and clearly separated from alarm and
patient-data behavior. The risk is not acceptable if the implementation allows
clinical users to edit the setting, accepts invalid config values, or uses an
expiry path that silently changes patient context.

## hazards and failure modes

- An invalid or out-of-range persisted value creates an immediate logout loop
  or disables the timeout in practice.
- The timeout expires while the user is reviewing data with low input activity,
  causing avoidable delay.
- The expiry path behaves differently from the intended secure state and leaves
  user or patient context partially exposed.
- The feature is placed in a settings area reachable by clinical users and
  weakens role-based access control.
- The design reuses the current explicit logout path without deciding whether
  clearing session data on idle expiry is acceptable.
- Timeout handling interferes with monitoring timers, alarms, or future device
  mode transitions.

## existing controls

- `SYS-013` and `SWR-GUI-002` already define explicit login/logout behavior and
  session clearing on logout.
- `SYS-017`, `SWR-GUI-008`, and `SWR-GUI-009` already define role-based
  privilege boundaries and an admin-specific user-management surface.
- `src/gui_main.c` already builds role-aware settings tabs and privileged
  account-management actions.
- `src/app_config.c` and `include/app_config.h` already provide local
  `monitor.cfg` persistence patterns for settings values.
- The issue explicitly keeps clinical thresholds, alarms, patient data, remote
  sync, and new authentication factors out of scope.

## required design controls

- Define the idle-expiry behavior explicitly as either full logout or
  re-authentication lock, and do not leave that choice implicit.
- Restrict editing of the timeout value to `ROLE_ADMIN`; clinicians must not be
  able to change it through UI navigation or direct config-path misuse.
- Enforce the approved range on both UI save and config-file load. Reject zero,
  negative, non-numeric, and out-of-range values.
- Use an explicit unit in minutes and keep a human-approved default. Do not add
  a "never timeout" option in the MVP unless a human owner approves it.
- Keep the setting local to the device. No remote policy, account sync, or
  hidden per-user behavior should be added in this issue.
- Do not alter alarm logic, sampling cadence, patient-record semantics, or
  authentication-factor rules as part of this work.
- Update requirements and traceability explicitly so the timeout behavior is
  testable and auditable rather than implied.

## validation expectations

- Unit tests for timeout parsing, bounds enforcement, default fallback, and
  persistence/load behavior in `monitor.cfg`
- UI verification that only admin sessions can edit the setting
- Manual verification that the configured value persists across restart
- Manual verification that idle expiry leads to the specified secure state and
  that re-authentication behavior is consistent
- Regression verification that alarms, patient vitals logic, and unrelated
  settings behavior are unchanged
- Negative testing for malformed config values and minimum/maximum boundary
  inputs

## residual risk for this pilot

Residual pilot risk is low if the setting is bounded, admin-only, and kept
strictly separate from clinical monitoring behavior. Residual risk rises to
unacceptable if the implementation silently clears patient context on timeout
without an explicit product decision, or if invalid config values can bypass
the timeout policy.

## human owner decision needed, if any

Yes.

- Approve the allowed timeout range and default value for the pilot. The issue
  suggests 1-30 minutes, which is plausible but not yet approved by repo
  requirements.
- Decide whether idle expiry is a full logout that clears session data or a
  lock/re-auth flow that preserves the active patient context.
- Confirm whether clinicians may view the configured value read-only or whether
  the setting should be hidden entirely outside admin flows.
