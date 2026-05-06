# Design Spec: Show current date/time in the dashboard header

Issue: #77
Branch: `feature/77-show-current-date-time-in-dashboard-header`
Spec path: `docs/history/specs/77-show-current-date-time-in-dashboard-header.md`

## Problem

Issue #77 requests an always-visible current date/time display in the dashboard
header so the operator can orient the view without relying on the Windows title
bar or other OS chrome.

That need is reasonable for a shared monitoring workstation, but the current
dashboard does not expose any explicit system-time context in the application
surface. The header in `src/gui_main.c` currently renders the product title,
logged-in user, role badge, simulation status, and header buttons only.

The existing implementation also creates a specific design risk that the issue
text only partially captured: the current dashboard refresh loop is tied to the
simulation timer. `apply_sim_mode()` kills the timer when device mode is off,
and `WM_TIMER` only updates the dashboard when `sim_enabled && !sim_paused`.
If implementation naively "piggybacks on the existing UI tick", the displayed
clock would stop advancing in device mode and while simulation is paused. The
design therefore needs an explicit clock-refresh path that stays correct across
all dashboard states.

## Goal

Add a compact, read-only system date/time display to the existing dashboard
header so trained operators can see current workstation time while using the
main monitor view.

The intended outcome is:

- the dashboard shows a clearly labelled system-time display after login
- the displayed value remains available in simulation mode, paused simulation,
  and device mode
- the display is explicitly contextual only and cannot reasonably be confused
  with patient-event timing, alarm timing, or synchronized enterprise time
- the new header content fits within the current dashboard minimum width
  contract without overlapping existing buttons or role/session indicators
- the implementation remains presentation-only and does not alter patient,
  alert, NEWS2, authentication, or persistence behavior

This design assumes that local workstation time is acceptable for the pilot.
If the product owner instead requires synchronized, auditable, or
patient-record-linked time behavior, that is out of scope for issue #77 and
must be split into a new design and risk review.

## Non-goals

- No change to clinical thresholds, alarms, NEWS2 scoring, patient records, or
  alert-generation logic.
- No addition of patient-event timestamps, charting timestamps, alarm timers,
  session timers, or treatment timers.
- No ability to edit the system time from the dashboard.
- No new persistence, configuration, or synchronization feature for time
  source, timezone, NTP, EMR integration, or bedside-device clock alignment.
- No claim that the displayed value is anything other than the local
  workstation system time.
- No broad header redesign beyond the minimum reflow needed to make the new
  display readable and non-overlapping at the supported minimum width.

## Intended use impact, user population, operating environment, and foreseeable misuse

Intended use impact:

- The feature adds operator-orientation context to the existing dashboard.
- It does not change the core intended use of the product as a Windows-based
  patient vital signs monitor.

User population:

- Trained clinical staff, ward nurses, and other authorized operators using the
  monitor on a shared Windows workstation.

Operating environment:

- Clinical or simulated-clinical desktop workstations where the application may
  remain open during review, handoff, or alarm correlation activities.

Foreseeable misuse:

- An operator interprets the header value as a patient-event timestamp rather
  than general system time.
- An operator assumes the time is centrally synchronized with other hospital
  systems when it is only local workstation time.
- An operator misreads an ambiguous numeric date format.
- A clipped or stale display is trusted during handoff because it appears to be
  current.

## Current behavior

Current repository behavior relevant to this issue:

- `src/gui_main.c` renders header content in `paint_header()`, with the right
  side already shared by user/session information and three header buttons.
- `src/gui_main.c` enforces a dashboard minimum width of `760` pixels in
  `WM_GETMINMAXINFO`, so the new element must work at that width and above.
- `reposition_dash_controls()` anchors `Logout`, `Pause`, and `Settings` to the
  right edge, which limits the safe placement area for any new header text.
- `apply_sim_mode()` starts `TIMER_SIM` only when simulation mode is enabled
  and kills it when device mode is active.
- `WM_TIMER` advances dashboard state only for `TIMER_SIM` while
  `g_app.sim_enabled && !g_app.sim_paused`, so paused simulation currently does
  not trigger dashboard repaints.
- The approved requirements baseline contains no explicit UNS, SYS, or SWR item
  covering a system-time orientation display in the GUI.
- The repository already supports four static UI languages, so any new header
  label should route through the existing localization tables rather than be
  hard-coded in English.

## Proposed change

Implement issue #77 as a narrow presentation-layer enhancement with these
design decisions:

1. Add a dedicated read-only clock element to the dashboard header in
   `src/gui_main.c`. The feature stays entirely in the presentation layer and
   must not add fields to patient data structures, alerts, history entries, or
   persistence files.

2. Treat the displayed value as local workstation system time only. Retrieve
   the current value at render/update time from the operating system clock using
   standard Win32 time APIs. Do not cache it in patient state and do not reuse
   it as a clinical timestamp in any other surface.

3. Label the display explicitly as system time. The preferred pilot policy is:
   localized label text plus a fixed ISO-like numeric value format of
   `YYYY-MM-DD HH:MM`. This keeps the date unambiguous across English, Spanish,
   French, and German without requiring locale-specific month-name logic.

4. Add a clock-refresh path that is independent of `TIMER_SIM`. The preferred
   implementation is a lightweight dashboard timer dedicated to clock refresh
   that invalidates only the header region while the dashboard window exists.
   Do not rely solely on live-simulation updates, because that would freeze the
   value in paused simulation and device mode.

5. Keep the clock visible after login across these states:
   simulation enabled, simulation paused, device mode, patient admitted,
   patient cleared, and after logout/login recreation of the dashboard.

6. Reflow the header text zones only as needed to guarantee no overlap at the
   current `760px` minimum dashboard width. Existing action buttons, role badge,
   and logged-in user information have higher priority than decorative spacing.
   If the clock cannot be made readable at the supported minimum width without a
   broader header redesign, stop and reopen design rather than silently
   clipping text.

7. Keep the feature intentionally non-interactive. There is no click target,
   no settings affordance, and no dashboard control for changing time source,
   timezone, or refresh policy under this issue.

## Files expected to change

Expected files to modify during implementation:

- `src/gui_main.c`
- `include/localization.h`
- `src/localization.c`
- `requirements/UNS.md`
- `requirements/SYS.md`
- `requirements/SWR.md`
- `requirements/TRACEABILITY.md`

Expected files that may need modification if the team records a named manual
verification artifact for the new GUI requirement:

- `dvt/DVT_Protocol.md`
- `tests/unit/test_localization.cpp`

Expected files to inspect but not modify for this issue:

- `docs/history/risk/77-show-current-date-time-in-dashboard-header.md`
- `src/gui_main.c` timer and header-layout call sites
- existing localization strings and language-switch flows

Files that should not change:

- `src/vitals.c`
- `src/alerts.c`
- `src/news2.c`
- `src/patient.c`
- `src/gui_users.c`
- `src/app_config.c`
- persisted runtime data formats such as `monitor.cfg`, `users.dat`, or patient
  history representations

## Requirements and traceability impact

The current approved baseline does not explicitly cover a system-time
orientation display. To keep this change traceable and defensible, the feature
should not be implemented from issue prose alone.

Preferred traceability additions:

- Add `UNS-017` for operator-visible system-time orientation on the main
  dashboard.
- Add `SYS-020` describing a clearly labelled, read-only local system date/time
  display on the dashboard header, with no implication of patient-event timing
  or enterprise synchronization.
- Add `SWR-GUI-013` defining the GUI behavior in implementation terms, including
  label policy, display format, refresh behavior independent of simulation
  state, and the non-overlap requirement at the minimum supported width.

Expected traceability rules for the new requirement set:

- The new requirement chain should trace to GUI implementation only.
- It should not retroactively claim linkage to clinical alerting, NEWS2,
  patient-history, or authentication logic.
- Verification evidence should be explicit. A documented GUI verification case
  or DVT/manual protocol entry is preferred if no practical automated GUI test
  harness exists.
- If implementation adds a helper for localization coverage, keep that test
  focused on string availability or formatting policy, not on clinical logic.

No existing clinical requirements should be renumbered or reinterpreted to hide
this gap. The safer path is to add one narrow new requirement chain for the
clock rather than stretching `UNS-014` beyond its approved wording.

## Medical-safety, security, and privacy impact

Medical-safety impact is low and indirect. The feature does not alter measured
values, alarm thresholds, NEWS2 logic, patient identity, or session access.
The credible hazard is misinterpretation: an operator could mistake the header
clock for a patient-event timestamp or trust a stale or clipped value during
handoff.

Required safety controls for implementation:

- The display must be explicitly labelled as system time.
- The displayed date/time format must be unambiguous.
- The feature must remain read-only and presentation-only.
- The refresh path must work outside live simulation so the value does not
  freeze in supported dashboard states.
- The final layout must remain visually distinct from patient identifiers,
  alert banners, and any future timer concepts.

Security impact is low:

- no new authentication or authorization boundary
- no new network traffic
- no new trust relationship beyond the local workstation system clock

Privacy impact is none expected:

- no new patient data element
- no new storage of patient or user information
- no export or logging change is required for this MVP

## AI/ML impact assessment

This issue does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond the local workstation system clock
- Output: none
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged
- Dataset and bias considerations: not applicable
- Monitoring expectations: unchanged
- PCCP impact: none

## Validation plan

Implementation validation should prove both scope control and the specific GUI
behavior.

Baseline build and regression commands:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "GUI|Patient"
```

Required manual GUI verification:

- Log in and confirm the dashboard header shows the new system-time display.
- Leave the dashboard open long enough to confirm the displayed value advances
  and is not frozen.
- Pause simulation and confirm the clock remains visible and continues to
  advance.
- Disable simulation or enter device mode and confirm the clock remains visible
  and continues to advance.
- Admit a patient, clear the session, and confirm the clock remains readable
  and unchanged in meaning.
- Log out and log back in, then confirm the recreated dashboard still shows the
  clock correctly.
- Resize the dashboard to the supported minimum width and confirm there is no
  overlap with the title, logged-in user, role badge, sim-status indicator, or
  header buttons.
- Change the local system clock in a controlled test environment, or otherwise
  wait for the next visible time change, and confirm the display follows local
  workstation time after the expected refresh interval.
- Confirm the header wording does not imply patient-event, alarm, or session
  timing semantics.

Localization verification:

- Confirm the new header label exists in all supported UI language tables.
- If the implementation keeps the numeric ISO-like date/time value fixed across
  languages, verify that only the label text changes while the numeric format
  remains consistent and unambiguous.

Traceability verification:

- Confirm the new UNS/SYS/SWR entries are added before merging implementation.
- Confirm `requirements/TRACEABILITY.md` includes the new mapping and named
  verification evidence.

## Rollback or failure handling

If implementation cannot fit the new display into the existing header at the
supported minimum width without overlap or clipping, stop and raise a follow-on
design issue for header-layout refactoring rather than forcing a degraded clock
into the current surface.

If the product owner rejects local workstation time, the fixed ISO-like format,
or the "system time only" semantics, pause implementation and return the issue
to design with the revised requirement.

If implementation pressure expands the feature into synchronized clocks,
editable time controls, patient-event timestamping, or persistent time-source
configuration, stop and split that work into a new issue with its own risk and
requirements review.
