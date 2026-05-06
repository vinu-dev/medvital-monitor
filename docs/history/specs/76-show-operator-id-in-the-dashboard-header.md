# Design Spec: Issue 76

Issue: `#76`  
Branch: `feature/76-show-operator-id-in-the-dashboard-header`  
Spec path: `docs/history/specs/76-show-operator-id-in-the-dashboard-header.md`

## Problem

The dashboard header currently shows the signed-in user's display name and role
badge, but it does not show the authenticated operator ID for the active
session. In a shared-workstation workflow, display names alone are weaker for
traceability than the actual account identifier used to sign in.

The implementation already captures the authenticated username in
`g_app.logged_username` during `attempt_login()` and clears it on logout, but
`paint_header()` only renders `g_app.logged_user` (display name) plus the role
badge. That leaves the unique operator identifier unavailable at the point of
care even though the session state already has it.

## Goal

Add a read-only operator ID indicator to the dashboard header immediately next
to the existing role badge, using the authenticated username already stored in
session state. The change should improve operator traceability without changing
authentication rules, role permissions, account persistence, or any clinical
behavior.

## Non-goals

- Changing authentication logic, credential storage, or role assignment.
- Adding audit logging, activity history, or operator data export.
- Changing patient data, alarms, NEWS2, vital-sign classification, or other
  clinical workflow behavior.
- Changing the Settings or My Account flows beyond what is strictly required to
  support header display.
- Introducing a new account identifier source distinct from the existing
  authenticated username.

## Intended Use Impact, User Population, Operating Environment, And Foreseeable Misuse

### Intended use impact

This is a display-only traceability improvement for the existing Win32
dashboard. It does not change what the monitor measures, how it classifies
vitals, how alerts are generated, or how users authenticate. Its intended value
is faster confirmation of which authenticated operator account is currently
active on a shared station.

### User population

The change affects the same two authenticated user populations that already use
the dashboard:

- `ROLE_ADMIN` users
- `ROLE_CLINICAL` users

Both groups may log into a workstation that is shared across shifts, handoffs,
or supervised demo/training use.

### Operating environment

The affected surface is the existing Win32 desktop dashboard in both simulation
mode and device mode. The header must remain legible in the current resizable
window layout and must not overlap the pause/settings/logout controls or the
simulation status indicator.

### Foreseeable misuse

- An implementer displays the user's display name again and labels it as an
  operator ID, leaving the unique authenticated account still hidden.
- A long username causes the new indicator to overlap existing header buttons
  or obscure the simulation status indicator.
- The header continues to show a stale operator ID after logout or after a new
  user signs in.
- Reviewers interpret the operator ID chip as a permissions change rather than
  a display of the already-authenticated account.

Design controls for these misuse cases:

- Use `g_app.logged_username` as the display source for operator ID, not
  `g_app.logged_user`.
- Preserve existing login and logout session clearing behavior.
- Use a bounded layout with truncation or ellipsis for long usernames.
- Keep the operator ID visually secondary to, and separate from, the existing
  role badge so the UI does not imply new authority.

## Current Behavior

- `src/gui_main.c` stores the authenticated role in `g_app.logged_role` and the
  authenticated username in `g_app.logged_username` inside `attempt_login()`.
- The same login path resolves a display name via `auth_display_name()` and
  stores it in `g_app.logged_user`.
- `paint_header()` renders:
  - the application title
  - the display name from `g_app.logged_user`
  - the role badge derived from `g_app.logged_role`
  - the simulation status indicator when simulation mode is enabled
- The logout handler clears both `g_app.logged_user` and
  `g_app.logged_username`.
- The My Account tab uses the display name plus role text, but the dashboard
  header does not expose the authenticated username.

## Proposed Change

1. Update `src/gui_main.c` so `paint_header()` renders a dedicated operator ID
   element adjacent to the existing role badge, sourced from
   `g_app.logged_username`.
2. Keep the existing display name text and role badge. The operator ID should
   be rendered as a distinct chip or bounded text block between the display
   name and the role badge, rather than replacing either element.
3. Prefer a username-only operator ID chip, for example `clinical`, instead of
   adding a new localized prefix such as "Operator ID:". This keeps the change
   narrow and avoids unnecessary localization churn.
4. Preserve the existing session lifecycle:
   - set `g_app.logged_username` on successful login
   - clear it on logout
   - never reload operator identity from storage during paint
5. Adjust header spacing so the operator ID does not overlap:
   - `IDC_BTN_SETTINGS`, `IDC_BTN_PAUSE`, `IDC_BTN_LOGOUT`
   - the existing role badge
   - the simulation status indicator
6. When the username is too long for the available width, truncate or ellipsize
   the rendered value. The fallback priority should be:
   - preserve buttons
   - preserve role badge
   - preserve a visible operator ID token
   - shrink or ellipsize the display-name text before dropping the operator ID
7. Keep the change inside the existing presentation/session boundary. No new
   persistence fields, authentication APIs, or user-record schema changes
   should be introduced.
8. Update requirements and traceability documents as part of implementation.
   Recommended traceability approach:
   - add `SYS-020` for authenticated operator identification in the dashboard
     header, traced to existing `UNS-016`
   - add `SWR-GUI-013` for the concrete header behavior in `src/gui_main.c`
   - update `requirements/TRACEABILITY.md` to map the new GUI requirement to
     implementation and verification evidence

## Files Expected To Change

Expected implementation files:

- `src/gui_main.c`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Optional file changes only if the implementer chooses to extract pure,
testable formatting logic:

- `tests/unit/test_auth.cpp`
- or a new focused unit test file for header-formatting behavior

Files expected not to change:

- `src/gui_users.c`
- `src/gui_auth.c`
- `include/gui_users.h`
- `include/gui_auth.h`
- `src/localization.c`
- `include/localization.h`
- Any clinical domain module such as `vitals.c`, `alerts.c`, `patient.c`, or
  `news2.c`
- CI, release, and installer files

## Requirements And Traceability Impact

- This feature is best traced to existing `UNS-016` because it refines the
  multi-user authenticated-session experience rather than adding new clinical
  behavior.
- Recommended system-level addition:
  - `SYS-020`: the dashboard header shall display the authenticated operator ID
    for the active session alongside the role indicator.
- Recommended software-level addition:
  - `SWR-GUI-013`: after successful authentication, the dashboard header shall
    render the authenticated username from the active session adjacent to the
    role badge, update it on each login, and clear it on logout; long values
    shall remain bounded so header controls stay usable.
- Recommended implementation references for the new SWR:
  - `src/gui_main.c` - `attempt_login()`
  - `src/gui_main.c` - logout handling in `dash_proc()`
  - `src/gui_main.c` - `paint_header()`
- Recommended verification evidence:
  - primary: manual GUI demonstration, consistent with existing GUI-header
    verification practice
  - optional secondary: unit coverage for any extracted pure formatting helper
- No change is intended to existing clinical requirements, alarm logic,
  authentication success criteria, or RBAC semantics.

## Medical-Safety, Security, And Privacy Impact

- Medical safety: low, display-only. The change does not alter patient
  monitoring, classification thresholds, alerts, NEWS2 scoring, persistence of
  vital records, or treatment workflow. It can improve accountability during
  workstation handoff by making the active authenticated operator easier to
  confirm.
- Security: low impact if scoped correctly. The implementation must not change
  credential validation, hashing, or role enforcement. The operator ID display
  must use the already-authenticated in-memory username only and must not expose
  password material, hash values, or inactive accounts.
- Privacy: limited but real on-screen exposure increase. The authenticated
  username becomes more explicit in the shared header. This is acceptable only
  because the issue explicitly asks for operator identification and the value is
  already part of the active authenticated session. No new storage, export, or
  transmission of operator data should be added.

## AI/ML Impact Assessment

This change does not add, modify, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing authenticated session strings
- Output: none beyond deterministic UI text rendering
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged
- Dataset and bias considerations: none
- Monitoring expectations: unchanged
- PCCP impact: none

## Validation Plan

Implementation validation should stay targeted to the affected GUI/session
surface and the associated traceability updates.

Build and regression checks:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R GUI|Auth|Patient
```

Manual GUI validation:

1. Log in as the built-in admin account and confirm the header shows:
   - existing display name
   - operator ID derived from the authenticated username
   - existing admin role badge
2. Log out, then log in as the built-in clinical account and confirm the
   operator ID updates to the clinical username with no stale admin value left
   behind.
3. Verify the header in both simulation-enabled and simulation-disabled modes
   so the operator ID does not collide with the simulation status indicator.
4. Resize the window to a narrower but still supported width and confirm the
   operator ID remains bounded and does not overlap the right-side buttons.
5. If practical during implementation, create or use a long username near the
   configured limit to confirm truncation or ellipsis behavior remains safe.

Traceability validation:

```powershell
rg -n "SYS-020|SWR-GUI-013|UNS-016|operator ID|authenticated username" requirements
git diff --name-only
git diff -- src/gui_main.c requirements/SYS.md requirements/SWR.md requirements/TRACEABILITY.md
```

## Rollback Or Failure Handling

- If header layout testing shows the new operator ID element cannot fit without
  interfering with header controls, do not ship an overlapping layout. Reduce
  or ellipsize the display-name region first and keep the operator ID bounded.
- If the product owner or reviewer rejects use of the authenticated username as
  the operator ID because deployment policy requires a different staff
  identifier, stop implementation and obtain an approved identifier source
  before changing account or persistence design.
- If implementation unexpectedly requires changes to `gui_users.c`,
  `gui_auth.c`, account-file persistence, or role semantics, pause and split the
  work; this issue is intentionally scoped as a presentation-layer enhancement.
- Rollback is straightforward: revert the header rendering change and the
  associated requirements/traceability entries, restoring the prior
  display-name-plus-role-badge behavior.
