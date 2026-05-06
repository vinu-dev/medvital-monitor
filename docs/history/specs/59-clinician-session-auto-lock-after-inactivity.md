# Design Spec: Clinician session auto-lock after inactivity

Issue: #59
Branch: `feature/59-clinician-session-auto-lock-after-inactivity`
Spec path: `docs/history/specs/59-clinician-session-auto-lock-after-inactivity.md`

## Problem

Issue #59 identifies a session-hygiene gap on shared bedside workstations. The
application already requires login before dashboard access and supports manual
logout, but an authenticated workstation can remain open indefinitely if the
operator walks away without clicking Logout.

The current logout implementation is not a safe proxy for inactivity lock. In
`src/gui_main.c`, manual logout kills the simulation timer, saves config,
clears `g_app.patient`, clears the logged-in identity, recreates the login
window, and destroys the dashboard. Reusing that path for inactivity would turn
an access-control control into a session-destructive workflow change, which the
risk note explicitly forbids.

This issue therefore needs a distinct locked state that obscures patient data
and requires re-authentication, while preserving approved monitoring context
and avoiding changes to vital-sign classification, alert generation, NEWS2,
alarm limits, or patient-record integrity.

## Goal

Implement a narrow inactivity-based session lock for authenticated dashboard
sessions that:

- triggers after a fixed, human-approved idle timeout
- hides patient-identifiable and session-identifying dashboard content
- requires full credential re-entry before patient data is shown again
- preserves in-memory monitoring context across the lock
- leaves manual logout semantics unchanged

The intended outcome is that a clinician can leave an authenticated station
idle without exposing patient data or stale user identity, while the monitor
state itself is not silently reset.

## Non-goals

- No password-policy, SSO, badge, biometric, or network-auth changes.
- No timeout configurability, per-user timeout preference, or settings-tab
  control in this issue.
- No changes to vital thresholds, NEWS2 scoring, trend logic, alarm limits,
  alert text, or patient-history capacity.
- No changes to persistence of patient records across process restart.
- No requirement to preserve the previous authenticated user after lock; the
  unlock flow may establish a new authenticated user.
- No attempt to make inactivity lock a substitute for explicit manual logout or
  wider organizational security policy.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use remains unchanged: trained clinical staff use the Windows
workstation UI to observe monitored patient data and interact with a local
authenticated session.

Relevant user and environment assumptions:

- shared bedside or ward workstations are plausible
- periods of passive observation with no keyboard or mouse input are normal
- more than one authorized clinician may use the same station across a shift

Foreseeable misuse and edge conditions:

- a clinician walks away without logging out
- a second clinician approaches an unattended station
- the station is being passively observed when the idle timeout expires
- an alert state changes while the screen is locked
- auxiliary authenticated windows such as Settings or password dialogs are open
  when the lock fires

The design must reduce unattended exposure without causing session destruction
or requiring the previous user to be physically present to resume patient
observation.

## Current behavior

The current login and session flow is split across the login window and the
dashboard window:

- `attempt_login()` authenticates through `auth_validate_role()`, stores
  `logged_username`, `logged_user`, and `logged_role`, then creates the
  dashboard and destroys the login window.
- manual logout in `dash_proc()` kills `TIMER_SIM`, saves `sim_enabled`,
  zeroes the patient record, resets `has_patient`, clears the stored user
  identity, recreates the login window, and destroys the dashboard
- `update_dashboard()` and the paint helpers render patient name, ID, BMI,
  readings, alert list entries, and role identity directly from `g_app`
- the only existing periodic timer is `TIMER_SIM`, which is tied to simulation
  mode; there is no independent idle-session timer
- `app_config` persists `sim_enabled` and language only; there is currently no
  timeout policy persisted to disk

These facts drive two constraints:

- inactivity lock must not call the current manual logout path
- inactivity tracking must not depend on the simulation timer, because session
  lock still matters when simulation is paused, disabled, or later replaced by
  real-hardware mode

## Proposed change

Implement the feature as a distinct UI/session-state addition, not as a
clinical-feature change.

1. Add a dedicated authenticated-session lock state to the application UI
   state. The dashboard session should explicitly distinguish:
   - logged out
   - active authenticated session
   - locked authenticated monitor context

2. Keep manual logout behavior unchanged. Clicking Logout must continue to
   clear session-owned patient data and return to the login window exactly as
   it does today. Idle expiry must use a separate `lock_session()` path.

3. Introduce a small non-clinical helper module for idle policy and timeout
   arithmetic, for example `include/session_lock.h` and `src/session_lock.c`.
   This module should:
   - use only static or stack storage
   - own the fixed timeout constant and elapsed-time calculation
   - expose testable helpers for activity recording, timeout expiry, and lock
     state transition decisions
   - avoid any dependency on Win32 painting or patient-domain code

4. Add a dedicated session timer, for example `TIMER_SESSION_IDLE`, that runs
   whenever the dashboard session is authenticated. It should tick in both
   simulation and device mode. The existing `TIMER_SIM` remains responsible
   only for simulated vital updates.

5. Record user activity from the central message loop rather than from only a
   few controls. The main loop already sees messages for the dashboard and the
   authenticated auxiliary windows. That loop should treat relevant keyboard,
   mouse, focus, and command messages targeting the dashboard or its owned
   authenticated dialogs as activity and reset the idle deadline.

6. Treat Settings, password change, and add-user windows as authenticated
   session surfaces. Interaction with those windows must reset the idle timer.
   If the lock fires while any of them are open, close them before presenting
   the lock surface so no privileged UI remains exposed behind the lock.

7. On idle expiry, transition to a locked surface that obscures patient data
   and session identity. The lock surface should be implemented as a dedicated
   dashboard-owned overlay or equivalent full-client lock UI so that:
   - patient name, ID, BMI, vital values, reading history, alert messages,
     role badge, and prior display name are not visible
   - underlying dashboard controls cannot be used until unlock succeeds
   - monitoring context can remain in memory behind the lock surface

8. The lock surface must require full re-authentication using the existing
   credential path. It should ask for username and password again, call the
   existing auth facade, and keep generic failure messaging. It must not reveal
   whether the prior username is still active or whether the username or
   password field was incorrect.

9. A successful unlock establishes a fresh authenticated session over the same
   in-memory monitor context. That means:
   - the unlocking user may be the same user or a different authorized user
   - `logged_username`, `logged_user`, and `logged_role` are refreshed from the
     unlocking credential pair
   - patient context and accumulated readings survive the lock
   - role-specific controls after unlock reflect the unlocking account, not the
     previously locked identity

10. Locked-state monitoring behavior must preserve the risk-note boundary:
    - do not clear `g_app.patient`
    - do not reset reading history solely because the session locked
    - do not change alarm thresholds, alert generation, or NEWS2 logic
    - do not suppress ongoing data acquisition merely because the user is
      locked out of the screen

11. Keep the timeout fixed in code for this issue. Do not add a settings-tab
    control or config-file persistence for timeout duration, warning grace, or
    lock enable/disable. If configurable policy is desired later, it needs a
    separate issue and review.

12. Localize all new lock and unlock strings through the existing localization
    tables. New user-facing strings for lock title, lock prompt, unlock action,
    and generic locked-state messaging should not be embedded as ad hoc English
    literals in `gui_main.c`.

13. Require human-owner approval for three policy decisions before
    implementation is treated as complete:
    - the default inactivity timeout value
    - whether a pre-lock warning or grace countdown is required
    - whether the locked screen may show any non-PHI indication that monitoring
      is active or alerts are present

## Files expected to change

Expected to change:

- `docs/history/specs/59-clinician-session-auto-lock-after-inactivity.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `include/session_lock.h`
- `src/session_lock.c`
- `tests/unit/test_session_lock.cpp`

Expected to inspect and modify only if implementation chooses to route unlock
through the existing auth facade rather than calling the user store directly:

- `include/gui_auth.h`
- `src/gui_auth.c`

Expected to inspect but not modify for MVP:

- `src/gui_users.c`
- `src/app_config.c`
- `include/app_config.h`
- `tests/unit/test_auth.cpp`
- `tests/unit/test_config.cpp`
- `docs/history/risk/59-clinician-session-auto-lock-after-inactivity.md`

Files that should not change:

- `src/vitals.c`
- `src/alerts.c`
- `src/patient.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `src/trend.c`
- `tests/integration/**`

## Requirements and traceability impact

This issue should remain traceable as an access-control enhancement, not as a
clinical algorithm change.

Recommended requirements approach:

- keep `SYS-013` focused on launch authentication and manual logout semantics
- add a new system requirement for inactivity-based session lock rather than
  overloading logout text
- update `SYS-017` or its derived software text so role-specific controls after
  unlock reflect the newly authenticated user
- add a new GUI/software requirement for idle activity tracking, timeout
  expiry, and locked-screen privacy behavior

Recommended traceability changes:

- trace the new behavior back to `UNS-013` and `UNS-016`
- add new forward-trace rows in `requirements/TRACEABILITY.md` for the new
  inactivity-lock SWR(s)
- link the new SWR(s) to:
  - `src/session_lock.c` for timeout policy helpers
  - `src/gui_main.c` for state transition, lock surface, and unlock flow
  - `tests/unit/test_session_lock.cpp` for timeout arithmetic and state logic
  - manual GUI verification for end-to-end lock/unlock behavior and role
    restoration on unlock

The design should not claim any change to existing RR, NEWS2, alarm-limit, or
clinical-alert traceability rows.

## Medical-safety, security, and privacy impact

Medical-safety impact is indirect but real because the feature can interrupt
observation workflow if poorly tuned.

Safety constraints:

- idle lock must stay separate from manual logout so patient context is not
  silently destroyed
- the approved patient-context behavior while locked must be explicit and
  verifiable
- alert-generation logic and monitoring calculations must remain unchanged
- active monitoring state should continue in memory while locked unless a
  future reviewed requirement says otherwise

Security impact is the primary positive driver:

- reduces unattended-session exposure
- reduces the chance that a later clinician acts under stale credentials
- forces re-authentication before PHI is shown again
- allows role and identity to refresh to the actual unlocking user

Privacy impact is also positive if implemented correctly:

- lock surface must not display patient name, ID, history, or alert text
- the prior user identity should not remain visible while locked
- auxiliary authenticated dialogs must not remain exposed behind the lock

Residual risk remains if the timeout is too aggressive during passive
observation or if lock behavior during active alerts is underspecified.
Implementation should therefore treat the owner-approved timeout and warning
policy as required acceptance inputs, not as optional polish.

## AI/ML impact assessment

This change does not add, change, remove, or depend on an AI-enabled device
software function. No model is introduced. No AI input data, output,
human-in-the-loop boundary, transparency control, dataset concern, bias
analysis, monitoring expectation, or Predetermined Change Control Plan impact
applies. Existing NEWS2 and alerting logic remain deterministic and unchanged.

## Validation plan

Validation should combine new unit coverage for the non-clinical lock-policy
helper with manual GUI verification for the Win32 state transition.

Build and automated regression:

```powershell
cmake --build build
build/tests/test_unit.exe
build/tests/test_integration.exe
```

New unit-test focus:

- idle deadline expires only after the approved timeout
- qualifying user activity resets the deadline
- lock state does not auto-clear without successful unlock
- manual logout semantics are not reused by the lock helper

Manual GUI scenarios:

- log in and confirm normal dashboard behavior is unchanged before inactivity
- remain intermittently active for longer than the timeout and confirm the
  session does not false-lock
- leave the authenticated dashboard idle past the approved timeout and confirm
  the lock surface obscures patient and user identity
- attempt unlock with bad credentials and confirm generic failure handling plus
  continued lock state
- unlock with the same user and confirm patient context and reading history are
  still present
- lock again and unlock with a different authorized user and confirm the role
  badge and role-specific controls now match the unlocking account
- verify manual Logout still clears session data and returns to the login
  window, distinct from lock behavior
- verify settings, password, and add-user windows do not remain exposed when
  lock activates
- verify simulation mode continues to behave per approved design while locked
  and after unlock

Traceability review:

- confirm new SYS/SWR text exists for inactivity lock
- confirm the RTM includes the new implementation and verification evidence
- confirm no existing clinical trace rows were edited beyond the new lock
  linkage

## Rollback or failure handling

Rollback is straightforward because the change is UI-session scoped and does not
change the patient-data model on disk.

If implementation fails to meet the distinct-lock requirement, revert to the
pre-feature behavior rather than silently falling back to manual logout on idle.

If the owner does not approve the timeout value, warning/grace policy, or
locked-state behavior during active alerts, do not broaden the implementation
with guessed defaults. Keep the code path disabled or stop before merge until
those decisions are made.

If partial implementation introduces data loss, hidden dashboard exposure, or
role confusion after unlock, back out the lock path entirely and retain the
existing explicit-login and explicit-logout flow until the design is corrected.
