# Design Spec: Make clinician idle timeout configurable

Issue: `#60`
Branch: `feature/60-make-clinician-idle-timeout-configurable`
Spec path: `docs/history/specs/60-make-clinician-idle-timeout-configurable.md`

## Problem

Issue #60 asks for an administrator-configurable idle timeout for clinician
session auto-lock. The current repository does not expose a configurable idle
timeout and, more importantly, does not show an existing traced idle-timeout
requirement or visible idle-timeout implementation in `src/**`,
`include/**`, or `requirements/**`.

Current code supports:

- explicit login and logout
- role-based settings access
- local `monitor.cfg` persistence for a small set of GUI settings

Current code does not support:

- idle detection tied to authenticated sessions
- lock-versus-logout semantics for idle expiry
- bounded timeout persistence and validation
- a settings control for session timeout policy

Without a narrow design, implementation could easily pick the wrong secure
state on expiry, expose the setting to clinical users, or add config parsing
that accepts invalid values and weakens access control.

## Goal

Add a narrow, traceable session-idle control that lets an administrator
configure how long an authenticated application session may remain inactive
before the UI enters a secure locked state.

The intended implementation outcome is:

- one local timeout value, in minutes, stored in `monitor.cfg`
- admin-only editing of that value
- enforced minimum, maximum, and default values
- idle expiry that locks the interactive UI and requires re-authentication
- no change to alarms, vital-sign calculations, patient-record semantics, or
  monitoring cadence

For this design, the recommended bounds are:

- minimum: `1` minute
- maximum: `30` minutes
- default/fallback: `5` minutes

## Non-goals

- No new authentication factor, SSO, badge login, or remote identity flow.
- No per-user or per-role timeout values in the MVP.
- No remote policy sync, fleet management, audit export, or analytics.
- No "never timeout" or `0 = disabled` option in the MVP.
- No reuse of the existing explicit logout path as the idle-expiry behavior.
- No change to alarm limits, NEWS2, vital classification, patient storage,
  live-monitoring cadence, or hardware-abstraction behavior.
- No production-code changes in this architect role; only the design spec is
  added on this branch.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact:

- The feature extends access-control behavior for authenticated sessions on the
  local workstation.
- It does not change clinical interpretation or monitoring behavior.

User population:

- ward administrators who configure the timeout
- authenticated bedside clinicians and administrators whose sessions are
  subject to the configured timeout

Operating environment:

- shared local Windows workstation or monitor terminal in a ward or bedside
  setting

Foreseeable misuse:

- an administrator sets the timeout too low and causes avoidable workflow
  interruption
- an administrator sets the timeout too high and weakens unattended-session
  protection
- a malformed `monitor.cfg` value effectively disables or destabilizes the
  feature if validation is weak
- passive dashboard updates are incorrectly treated as user activity, so the
  session never locks in simulation mode
- implementation silently clears patient context on timeout by reusing logout
  instead of an explicit lock flow

## Current behavior

- `src/gui_main.c` implements explicit login and logout only. Logout destroys
  the dashboard window, clears session fields, clears patient state, and
  returns to the login window.
- `src/gui_main.c` already enforces role-based settings navigation. Admin users
  receive the full settings surface; clinical users receive a reduced tab set.
- `src/app_config.c` and `include/app_config.h` persist `sim_enabled` and
  `language` in `monitor.cfg`, using dedicated load/save helpers rather than a
  broad config object.
- `tests/unit/test_config.cpp` already covers basic `monitor.cfg` persistence
  and malformed-file fallback behavior for existing keys.
- Repository search and the issue-60 risk note both indicate that idle-timeout
  behavior is not currently implemented or traced as an approved requirement.

## Proposed change

Implement issue #60 as a new local session-control feature with the following
design decisions.

1. Treat idle timeout as a new authenticated-session behavior, not as exposure
   of an existing hidden constant.

2. Apply the timeout to all authenticated dashboard sessions, including admin
   and clinical users. Editing the value remains admin-only.

3. Use a dedicated locked-session state on idle expiry, not the existing
   logout flow. The locked state shall:
   - hide or fully obscure patient-identifying and patient-monitoring content
   - block dashboard and settings interaction until re-authentication succeeds
   - preserve the current in-memory patient session and monitoring state
   - leave explicit user-initiated Logout behavior unchanged

4. Preserve monitoring and alarm behavior while locked. Idle expiry must not:
   - stop simulation/device timers
   - pause alarm evaluation
   - clear the patient record
   - change the current patient context

5. Add a separate session-idle timer path in `src/gui_main.c` that runs
   regardless of simulation mode. Do not reuse `TIMER_SIM` as the only expiry
   clock because idle locking must still work when simulation is disabled.

6. Track app-local user activity, not passive redraws. Only keyboard, mouse,
   and direct authenticated-window interaction should reset the inactivity
   timestamp. Automatic `WM_TIMER` updates, repaint traffic, and simulated
   vital refreshes must not count as activity.

7. Add a small pure helper module for policy and expiry logic, for example:
   - `include/session_timeout.h`
   - `src/session_timeout.c`

   That helper should own:
   - approved constants (`min`, `max`, `default`)
   - validation/clamp-or-default behavior for persisted values
   - a pure "expired or not" decision based on elapsed ticks

   This keeps bounds and expiry semantics unit-testable instead of burying them
   entirely inside Win32 message handling.

8. Extend config persistence using the current module pattern rather than a
   large refactor. Recommended approach:
   - add `idle_timeout_minutes=<N>` to `monitor.cfg`
   - add dedicated `app_config_load_idle_timeout_minutes()` and
     `app_config_save_idle_timeout_minutes()` helpers
   - preserve existing keys (`sim_enabled`, `language`) on save
   - reject missing, zero, negative, non-numeric, and out-of-range values by
     falling back to the approved default

9. Add an admin-only session/security settings surface in `src/gui_main.c`.
   Recommended MVP shape:
   - a new admin-only Settings tab such as `Session` or `Security`
   - a constrained numeric input or drop-down in minutes
   - inline text showing the allowed range and that the value applies to future
     idle detection immediately after save

   Clinical users should not be able to edit the value and should not gain a
   new navigation path into admin controls. For the MVP, do not add a
   clinician-visible editor.

10. Introduce a dedicated lock UI path in `src/gui_main.c`, such as a
    `PVM_Lock` modal window or lock overlay, instead of overloading the
    existing login window. Successful unlock should:
    - re-authenticate the current user with username/password
    - restore the dashboard/settings context without clearing patient data
    - reset the inactivity timestamp

11. Keep the implementation local and static-memory-compatible. No heap-backed
    session objects, remote callbacks, or persistence outside `monitor.cfg`
    should be introduced for this issue.

## Files expected to change

Expected implementation files:

- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`
- `include/app_config.h`
- `src/app_config.c`
- `include/session_timeout.h`
- `src/session_timeout.c`
- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`
- `tests/unit/test_config.cpp`
- `tests/unit/test_session_timeout.cpp`

Expected files to inspect and likely leave unchanged:

- `src/gui_auth.c`
- `include/gui_auth.h`
- `src/gui_users.c`
- `tests/unit/test_auth.cpp`
- `docs/history/risk/60-make-clinician-idle-timeout-configurable.md`

Files that should not change:

- `src/vitals.c`
- `src/alerts.c`
- `src/patient.c`
- `src/news2.c`
- `src/alarm_limits.c`
- `tests/integration/**`
- CI/release workflow files unless implementation discovers a build-target
  update is required for the new helper module

## Requirements and traceability impact

This issue should not rely on vague extension of current auth wording. It needs
explicit requirement updates.

Recommended traceability shape:

- extend `UNS-013` to cover unattended-session protection after authentication
- extend `UNS-016` to cover admin-only configuration of access-control policy
- add `SYS-020` for configurable idle session locking with bounded timeout,
  secure locked state, and local persistence requirements
- add `SWR-GUI-013` for the admin session-timeout settings UI and persistence
- add `SWR-SEC-005` for inactivity tracking, expiry enforcement, locked-state
  behavior, and re-authentication semantics

Recommended implementation/verification mapping:

- `SWR-GUI-013`
  - implementation: `src/gui_main.c`, `src/app_config.c`, `src/localization.c`
  - verification: `tests/unit/test_config.cpp`, manual admin-settings GUI
    verification

- `SWR-SEC-005`
  - implementation: `src/session_timeout.c`, `src/gui_main.c`
  - verification: `tests/unit/test_session_timeout.cpp`, manual idle-expiry and
    unlock walkthroughs

`requirements/TRACEABILITY.md` will need new forward and backward rows for the
new SWR(s), plus updated coverage counts if the implementation adds a new unit
test suite.

## Medical-safety, security, and privacy impact

Medical-safety impact is indirect but real. A timeout that is too short can
interrupt review and delay access to patient information. A timeout that is too
long can leave patient data exposed on an unattended shared device. The design
mitigates that by using:

- bounded values
- an admin-only editor
- explicit lock semantics that preserve monitoring behavior
- strict fallback on invalid config input

Security impact is positive if implemented as designed. The feature improves
local unattended-session protection, but only if:

- invalid config values cannot disable the control
- clinicians cannot edit the timeout
- idle expiry leads to a true locked state that requires re-authentication

Privacy impact is meaningful because patient identity and current monitoring
context may be visible on shared devices. The lock UI must obscure patient data
until unlock succeeds.

Because this issue touches authentication and persistence, the implementation
review must explicitly confirm that idle expiry does not silently broaden data
access, change logout semantics, or alter patient-monitoring behavior.

## AI/ML impact assessment

This change does not add, change, remove, or depend on any AI-enabled device
software function.

- No model is introduced.
- No model input/output path changes.
- No human-in-the-loop AI decision changes.
- No dataset, bias, monitoring, or PCCP implications apply.

## Validation plan

Build and automated verification:

```powershell
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
build/tests/test_unit.exe
```

Targeted automated checks expected after implementation:

```powershell
build/tests/test_unit.exe --gtest_filter=ConfigTest.*:SessionTimeout*.*
```

Targeted text/config inspection:

```powershell
rg -n "idle_timeout|session timeout|locked" requirements/SYS.md requirements/SWR.md requirements/TRACEABILITY.md src/gui_main.c src/app_config.c include/session_timeout.h src/session_timeout.c tests/unit/test_config.cpp tests/unit/test_session_timeout.cpp
git diff --stat
```

Manual verification:

1. Log in as admin and confirm the session-timeout setting is visible only in
   the admin settings surface.
2. Save boundary values `1` and `30` minutes and verify they persist across
   application restart.
3. Attempt invalid values (`0`, negative, non-numeric, above max) and verify
   the app stores or loads the approved default instead of accepting them.
4. Leave the authenticated UI idle until expiry and verify:
   - the UI locks
   - patient-identifying content is obscured
   - monitoring continues in the background
   - alarms and current patient context are unchanged
5. Unlock with valid credentials and verify the same patient session resumes.
6. Verify an incorrect unlock password does not expose which field was wrong.
7. Verify clinical users cannot edit the setting or reach the admin-only
   session configuration path.

Regression focus:

- explicit logout still clears session and returns to login
- simulation mode behavior is unchanged apart from no longer preventing idle
  locking through passive timer traffic
- language, alarm-limit, and other settings persistence remain intact

## Rollback or failure handling

If implementation cannot preserve patient context behind a locked state without
an unexpectedly broad refactor, stop and split the work rather than silently
falling back to full logout on idle expiry.

If the product owner rejects the recommended `1..30` minute range or `5` minute
default, update the requirements/spec before implementation rather than
hard-coding unapproved values.

If bounded validation cannot be added without destabilizing the current
`monitor.cfg` helpers, revert the partial timeout changes and keep the branch in
documentation/design state until the config approach is corrected.

If implementation discovers a need for role-specific or per-user timeout
policies, do not expand this issue in place. Treat that as a follow-on feature.
