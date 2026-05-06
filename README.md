# Patient Vital Signs Monitor

[![CI — Build & Test](https://github.com/vinu-dev/medvital-monitor/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vinu-dev/medvital-monitor/actions/workflows/ci.yml)

Medical device software for real-time patient vital sign monitoring and alert generation.
Built to **IEC 62304 Class B** and **FDA SW Validation Guidance** standards.
**Version 2.7.0** — Six vital signs (including respiration rate), NEWS2 early warning score, configurable alarm limits (IEC 60601-1-8), trend sparkline graphs, a session alarm event review log, role-based settings access, rolling status message in simulation mode, 293 unit + 14 integration tests (307 total).

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Modules](#modules)
4. [Alert Thresholds](#alert-thresholds)
5. [Installation](#installation)
6. [Build](#build)
7. [GUI Workflow](#gui-workflow)
8. [User Roles & Settings](#user-roles--settings)
9. [Testing](#testing)
10. [Code Coverage](#code-coverage)
11. [Documentation](#documentation)
12. [Standards Compliance](#standards-compliance)
13. [Repository Structure](#repository-structure)

---

## Overview

The Patient Vital Signs Monitor acquires and classifies six vital sign parameters
in real time, generates structured alert records, computes a NEWS2 early warning
score, and presents them through a colour-coded Windows desktop GUI with trend
sparkline graphs.  A console demonstration executable is also included.

**Supported vital signs:**

| Parameter          | Unit    | Source standard                       |
|--------------------|---------|---------------------------------------|
| Heart rate         | bpm     | AHA/ACC 2019                          |
| Systolic BP        | mmHg    | JNC-8 / ESC 2018                      |
| Diastolic BP       | mmHg    | JNC-8 / ESC 2018                      |
| Body temperature   | °C      | WHO Clinical References               |
| SpO2               | %       | British Thoracic Society              |
| Respiration rate   | br/min  | IEC 80601-2-49 / NEWS2 (RCP 2017)     |
| BMI (derived)      | kg/m²   | WHO BMI categories                    |
| NEWS2 score        | 0–20    | Royal College of Physicians 2017      |

**Design constraints (IEC 62304 Class B):**

- No heap allocation — all storage is stack or statically sized.
- Patient record holds up to `MAX_READINGS` (10) readings per monitoring cycle;
  the GUI resets and continues automatically when the buffer is full.
- Hardware acquisition isolated behind a HAL (`hw_vitals.h`) so the simulation
  driver (`sim_vitals.c`) can be replaced by a real hardware driver without
  modifying any other file.

---

## Architecture

```

## Agentry Medical Pilot

This repository is also used as the full medical-software pilot target for
Agentry. The active pilot flow uses Codex-only roles for research, risk
analysis, architecture, implementation, testing, code review, quality review,
cybersecurity review, regulatory impact review, traceability review, merge, and
release smoke checks.

See `docs/ai/agentry-medical-pilot.md` for the current role chain and operating
rules.
+-----------------------------------------------------------------------+
|                        Presentation Layer                             |
|                                                                       |
|  patient_monitor_gui.exe (gui_main.c)   patient_monitor.exe (main.c) |
|  Interactive Win32 dashboard            Console scenario demo         |
|  -- Login / logout                                                    |
|  -- Colour-coded vital tiles (NORMAL/WARNING/CRITICAL)                |
|  -- Live simulation feed  (Pause / Resume)                            |
+----------------------------+------------------------------------------+
                             |
              +--------------v--------------+
              |        HAL Interface         |
              |  hw_vitals.h                 |
              |  hw_init() / hw_get_next()   |
              |                              |
              |  sim_vitals.c  <-- current   |
              |  hw_driver.c   <-- swap in   |
              |                   for real HW|
              +--------------+---------------+
                             |
              +--------------v--------------+
              |          patient.c           |
              |   PatientRecord management   |
              |   - Init / add reading       |
              |   - Status query             |
              |   - Print summary            |
              +--------+----------+----------+
                       |          |
          +------------v--+  +----v-----------+
          |   vitals.c    |  |   alerts.c     |
          | Parameter     |  | Alert record   |
          | validation    |  | generation     |
          | BMI / strings |  | from VitalSigns|
          +---------------+  +----------------+
```

### Data Flow

```
hw_get_next_reading()  -->  VitalSigns
VitalSigns  -->  check_*()            -->  AlertLevel  (per parameter)
VitalSigns  -->  overall_alert_level  -->  AlertLevel  (aggregate)
VitalSigns  -->  generate_alerts()   -->  Alert[]      (display records)
Alert[]     -->  GUI tiles / status banner / alerts list
```

### Memory Model

No heap allocation is used anywhere in production code.

| Structure       | Storage                                       |
|-----------------|-----------------------------------------------|
| `VitalSigns`    | 6 fields — ~24 bytes                          |
| `Alert`         | level + message strings — ~132 bytes          |
| `PatientRecord` | PatientInfo + 10 × VitalSigns + int           |
| Simulation      | 20-entry static table in `sim_vitals.c`       |

---

## Modules

### `vitals.c` / `vitals.h` — UNIT-VIT

Core classification engine.  Validates each vital sign parameter against
AHA/ACC-derived clinical thresholds and returns an `AlertLevel`.

| Function                 | Description                                           |
|--------------------------|-------------------------------------------------------|
| `check_heart_rate()`     | Classifies bpm → NORMAL / WARNING / CRITICAL          |
| `check_blood_pressure()` | Classifies systolic + diastolic together              |
| `check_temperature()`    | Classifies °C → NORMAL / WARNING / CRITICAL           |
| `check_spo2()`           | Classifies SpO2 % → NORMAL / WARNING / CRITICAL       |
| `check_respiration_rate()`| Classifies br/min → NORMAL / WARNING / CRITICAL      |
| `overall_alert_level()`  | Returns the highest level across all six parameters   |
| `calculate_bmi()`        | BMI = weight / height²; returns -1.0 for invalid input|
| `bmi_category()`         | Maps BMI float → WHO category string                  |
| `alert_level_str()`      | Maps `AlertLevel` → "NORMAL" / "WARNING" / "CRITICAL" |

---

### `alerts.c` / `alerts.h` — UNIT-ALT

Translates a `VitalSigns` snapshot into a list of human-readable `Alert`
records, one per out-of-range parameter.

| Function            | Description                                                  |
|---------------------|--------------------------------------------------------------|
| `generate_alerts()` | Fills caller-supplied buffer; returns count of alerts written |

---

### `patient.c` / `patient.h` — UNIT-PAT

Top-level patient record management.  Stores demographics and a history of
up to `MAX_READINGS` (10) vital sign readings.

| Function                   | Description                                    |
|----------------------------|------------------------------------------------|
| `patient_init()`           | Zero-fills and populates a `PatientRecord`     |
| `patient_add_reading()`    | Appends a reading; returns 0 if buffer is full |
| `patient_latest_reading()` | Returns pointer to last reading, or NULL       |
| `patient_current_status()` | Overall `AlertLevel` from latest reading       |
| `patient_is_full()`        | True when `reading_count == MAX_READINGS`      |
| `patient_print_summary()`  | Formatted terminal output with alerts          |

---

### `gui_auth.c` / `gui_auth.h` — UNIT-GUI (auth)

Fixed-credential authentication for the desktop GUI.

| Function             | Description                                             |
|----------------------|---------------------------------------------------------|
| `auth_validate()`    | Returns 1 only when username + password both match      |
| `auth_display_name()`| Maps a username to a display name for the header bar    |

Default credential: **admin / Monitor@2026**

---

### `hw_vitals.h` — UNIT-HAL

Hardware Abstraction Layer interface.  The GUI calls only these two functions;
replacing `sim_vitals.c` with a hardware driver is the only change needed to
connect real sensors.

| Function                | Description                                          |
|-------------------------|------------------------------------------------------|
| `hw_init()`             | Initialise the acquisition source (called on login)  |
| `hw_get_next_reading()` | Return the next `VitalSigns` sample                  |

---

### `sim_vitals.c` — UNIT-SIM

Simulation back-end for the HAL.  Cycles through a 20-entry clinical scenario
table covering four phases:

| Phase         | Indices | Status   | Description                          |
|---------------|---------|----------|--------------------------------------|
| Stable        | 0–4     | NORMAL   | All parameters within clinical range |
| Deteriorating | 5–8     | WARNING  | Gradual rise towards threshold       |
| Critical      | 9–11    | CRITICAL | Life-threatening values              |
| Recovering    | 12–19   | WARNING→NORMAL | Progressive return to stable  |

---

### `news2.c` / `news2.h` — UNIT-NEW

NEWS2 Early Warning Score per Royal College of Physicians 2017.

| Function            | Description                                                  |
|---------------------|--------------------------------------------------------------|
| `news2_calculate()` | Computes aggregate NEWS2 score (0–20) from 5 vitals + AVPU   |

Risk classifications: LOW (0–4), LOW_MEDIUM (any single param = 3), MEDIUM (5–6), HIGH (≥ 7).

---

### `alarm_limits.c` / `alarm_limits.h` — UNIT-ALM

Configurable per-patient alarm limits per IEC 60601-1-8.

| Function                 | Description                                          |
|--------------------------|------------------------------------------------------|
| `alarm_limits_defaults()`| Load factory defaults                                |
| `alarm_limits_save()`    | Persist to `alarm_limits.cfg`                        |
| `alarm_limits_load()`    | Restore from file or apply defaults                  |
| `alarm_check_*()`        | Check each parameter against custom limits           |

---

### `trend.c` / `trend.h` — UNIT-TRD

Trend direction detection and sparkline data extraction.

| Function             | Description                                               |
|----------------------|-----------------------------------------------------------|
| `trend_direction()`  | Rising / Falling / Stable classification (5% hysteresis)  |
| `trend_extract_*()`  | Extract per-parameter arrays from VitalSigns history      |

---

## Alert Thresholds

| Parameter    | NORMAL           | WARNING                          | CRITICAL              |
|--------------|------------------|----------------------------------|-----------------------|
| Heart Rate   | 60–100 bpm       | 41–59 bpm / 101–150 bpm          | ≤ 40 / ≥ 151 bpm      |
| Systolic BP  | 90–140 mmHg      | 71–89 mmHg / 141–180 mmHg        | ≤ 70 / ≥ 181 mmHg     |
| Diastolic BP | 60–90 mmHg       | 41–59 mmHg / 91–120 mmHg         | ≤ 40 / ≥ 121 mmHg     |
| Temperature  | 36.1–37.2 °C     | 35.0–36.0 °C / 37.3–39.5 °C     | < 35.0 / > 39.5 °C   |
| SpO2         | 95–100 %         | 90–94 %                          | < 90 %                |
| Resp Rate    | 12–20 br/min     | 9–11 / 21–24 br/min             | ≤ 8 / ≥ 25 br/min     |

*Sources: AHA/ACC 2019, JNC-8, ESC 2018, WHO, British Thoracic Society, IEC 80601-2-49*

---

## Installation

### End-user install (no development tools needed)

Download **`PatientMonitorSetup-2.7.0.exe`** from the
[Releases](https://github.com/vinu-dev/medvital-monitor/releases) page and
double-click to run the setup wizard.

If you want the standalone executable instead, download
**`PatientMonitor-v2.7.0.exe`**. The portable bundle is
**`PatientMonitor-v2.7.0-portable.zip`**.

| Step | What happens |
|------|-------------|
| 1. Run setup wizard | Choose install folder (default: `C:\Program Files\Patient Vital Signs Monitor\`) |
| 2. Select shortcuts | Start Menu group created automatically; optional desktop shortcut |
| 3. Finish | App launches immediately after install |

**Uninstall:** Settings → Apps → Patient Vital Signs Monitor → Uninstall
*(or Control Panel → Programs → Uninstall a program)*

**System requirements:** Windows 10 or later (32-bit or 64-bit). No additional
runtimes or redistributables required — the executable depends only on standard
Windows system DLLs (`GDI32`, `KERNEL32`, `USER32`).

### Build your own installer (developers)

```bat
:: One-time: install Inno Setup 6
winget install --id JRSoftware.InnoSetup

:: Build app + compile installer  ->  dist\PatientMonitorSetup-2.7.0.exe
create_installer.bat
```

---

## Build

### Prerequisites

| Tool    | Minimum   | Install                                                      |
|---------|-----------|--------------------------------------------------------------|
| CMake   | 3.15      | `winget install Kitware.CMake`                               |
| MinGW GCC | 6.3+   | https://sourceforge.net/projects/mingw/ — select gcc-g++, mingw32-make |
| Git     | Any       | `winget install Git.Git`  (needed by CMake FetchContent)     |

> **Note:** MSVC is **not** required.  The project uses MinGW GCC exclusively.
> Google Test (release-1.10.0) is downloaded automatically on first build.

### Scripts

Five scripts cover the full workflow — double-click in File Explorer or run
from a terminal.

| Script                 | What it does                                                          |
|------------------------|-----------------------------------------------------------------------|
| `build.bat`            | Configure + build everything (first run or incremental). Launches GUI.|
| `run_tests.bat`        | Rebuild test targets and run all 307 tests. Exits non-zero on failure.|
| `run_coverage.bat`     | Build with `--coverage`, run tests, generate HTML + XML reports.      |
| `generate_docs.bat`    | Run Doxygen to produce HTML + XML design documentation.               |
| `create_installer.bat` | Build release exe + compile Windows installer (`dist\` folder).       |

### Typical workflow

```bat
:: First time (or after a clean)
build.bat

:: After code changes
build.bat

:: Run tests
run_tests.bat

:: Code coverage (requires pip install gcovr for HTML output)
run_coverage.bat

:: Doxygen docs
generate_docs.bat
```

### Manual build

```bat
cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_C_COMPILER=gcc ^
      -DCMAKE_CXX_COMPILER=g++ ^
      -DBUILD_TESTS=ON
cmake --build build
build\patient_monitor_gui.exe
```

---

## GUI Workflow

**Login**

- Credentials: `admin` / `Monitor@2026`
- Press Enter or click **SIGN IN**

**Dashboard (after login)**

- A demo patient (James Mitchell) is admitted automatically on startup.
- Vital sign tiles update every 2 seconds from the simulation feed.
- The header shows **\* SIM LIVE** (green) or **SIM PAUSED** (amber).
- Click **Pause Sim** / **Resume Sim** to freeze or resume the feed.
- Click **Logout** to return to the login screen.

**Manual data entry (optional)**

- Edit patient demographics and click **Admit / Refresh** to change the patient.
- Enter vital signs manually and click **Add Reading**.
- Use **Demo: Deterioration** or **Demo: Bradycardia** to load preset scenarios.
- **Clear Session** resets all data; the simulation feed resumes on the next tick.

**Connecting real hardware**

1. Write `src/hw_driver.c` implementing `hw_init()` and `hw_get_next_reading()`
   (see `include/hw_vitals.h` for the interface contract).
2. Replace `src/sim_vitals.c` with `src/hw_driver.c` in `CMakeLists.txt`.
3. Rebuild — no other source file changes are needed.

---

## User Roles & Settings

### Default credentials

| Username   | Password        | Role     | Access                                         |
|------------|-----------------|----------|------------------------------------------------|
| `admin`    | `Monitor@2026`  | Admin    | Full dashboard + Settings panel                |
| `clinical` | `Clinical@2026` | Clinical | Full dashboard + Settings (Sim/Alarm/Account)  |

Both accounts are created automatically on first launch.  Credentials are
persisted in `users.dat` in the same directory as the executable.

### Role-based access

| Feature                    | Admin | Clinical |
|----------------------------|-------|----------|
| Live vital signs dashboard | Yes   | Yes      |
| Admit patient / Add reading| Yes   | Yes      |
| Demo scenarios (sim only)  | Yes   | Yes      |
| Pause / Resume simulation  | Yes   | Yes      |
| Change own password        | Yes   | Yes      |
| Settings panel             | Yes   | Yes      |
| Simulation toggle          | Yes   | Yes      |
| Alarm limits config        | Yes   | Yes      |
| Add / Remove users         | Yes   | —        |
| Set any user's password    | Yes   | —        |

The header bar shows a **gold ADMIN** or **teal CLINICAL** pill badge next to
the logged-in user's name.

### Settings panel (all users)

Click **Settings** in the dashboard header to open the Settings window.
Tabs visible depend on role.

**Users tab** (Admin only)
- Lists all accounts with username, display name, and role.
- **Add User** — create a new account with username, display name, initial
  password (min 8 chars), and role selection.
- **Remove** — delete a selected account.  The last admin account and the
  currently logged-in user cannot be removed.
- **Set Password** — override any user's password without requiring the
  current password.

**Simulation tab** — toggle simulation mode on/off, persisted to `monitor.cfg`.

**Alarm Limits tab** — per-patient alarm thresholds per IEC 60601-1-8.
Edit limits for HR, SBP, DBP, Temp, SpO2, RR. Apply & Save or Reset Defaults.

**My Account tab** — change own password (current + new + confirm).

**About tab** — application version, standard (IEC 62304 Class B), and
requirements revision.

---

## Testing

**Framework:** Google Test release-1.10.0 (downloaded automatically via CMake FetchContent)

### Test counts

| File                                            | Tests  | Requirements                      |
|-------------------------------------------------|--------|-----------------------------------|
| `tests/unit/test_vitals.cpp`                    | 80     | SWR-VIT-001 – 008                 |
| `tests/unit/test_alerts.cpp`                    | 11     | SWR-ALT-001 – 004                 |
| `tests/unit/test_patient.cpp`                   | 29     | SWR-PAT-001 – 008                 |
| `tests/unit/test_auth.cpp`                      | 41     | SWR-GUI-001–002, SWR-SEC-001–004  |
| `tests/unit/test_news2.cpp`                     | 53     | SWR-NEW-001                       |
| `tests/unit/test_alarm_limits.cpp`              | 31     | SWR-ALM-001                       |
| `tests/unit/test_trend.cpp`                     | 18     | SWR-TRD-001                       |
| `tests/unit/test_hal.cpp`                       | 12     | Supporting HAL / simulator checks only |
| `tests/unit/test_config.cpp`                    | 10     | Supporting config persistence checks only |
| `tests/unit/test_localization.cpp`              | 8      | SWR-GUI-012                       |
| `tests/integration/test_patient_monitoring.cpp` | 7      | SWR-PAT-*, SWR-VIT-*, SWR-ALT-*   |
| `tests/integration/test_alert_escalation.cpp`   | 7      | SWR-VIT-*, SWR-ALT-*, SWR-PAT-007 |
| **Total**                                       | **307** | **40 SWRs covered across automated, architecture-review, and GUI-demo/manual evidence** |

### Test techniques applied

| Technique                   | Applied to                                      |
|-----------------------------|-------------------------------------------------|
| Equivalence Partitioning    | All six vital sign classification functions     |
| Boundary Value Analysis     | Every threshold boundary (±1 from each limit)  |
| Boundary Sweep Tables       | Heart rate and SpO2 full boundary sweep         |
| Escalation / Deescalation   | NORMAL → WARNING → CRITICAL and back            |
| Multi-parameter crisis      | All six parameters critical simultaneously      |
| Capacity enforcement        | `patient_add_reading()` beyond MAX_READINGS     |
| Independence verification   | Two patients with different statuses            |
| Authentication paths        | Valid, wrong user, wrong password, empty, NULL  |

### Run tests

```bat
run_tests.bat
```

Or manually with CTest:

```bat
ctest --test-dir build --output-on-failure
```

---

## Code Coverage

**Toolchain:** MinGW GCC `--coverage` flag + `gcov` + `gcovr` (optional, for HTML)

**Scope:** `vitals.c`, `alerts.c`, `patient.c`, `gui_auth.c`, `gui_users.c`, `news2.c`, `alarm_limits.c`, `trend.c` (production logic)

### Setup

```bat
pip install gcovr      :: once — for HTML + Cobertura XML output
run_coverage.bat
```

> Without `gcovr`, the script falls back to `gcov` text output.

### Reports generated

| File                                     | Format        | Purpose                    |
|------------------------------------------|---------------|----------------------------|
| `coverage_report\index.html`             | HTML          | Line-by-line review        |
| `coverage_report\coverage_cobertura.xml` | Cobertura XML | DHF / audit trail record   |
| `build_cov\results_unit.xml`             | JUnit XML     | Unit test execution record |
| `build_cov\results_integration.xml`      | JUnit XML     | Integration test record    |

> Coverage is built in a separate `build_cov\` folder so the normal `build\` is not affected.

### IEC 62304 coverage targets

| Coverage Type | Class B | Class C (safety-critical)                   |
|---------------|---------|---------------------------------------------|
| Statement     | 100%    | 100%                                        |
| Branch        | 100%    | 100%                                        |
| MC/DC         | N/A     | 100% — requires VectorCAST / BullseyeCoverage |

---

## Documentation

**Tool:** Doxygen 1.9+ with Graphviz (for call graphs and dependency diagrams)

### Install tools

```bat
winget install --id DimitriVanHeesch.Doxygen
winget install --id Graphviz.Graphviz
```

### Generate

```bat
generate_docs.bat
```

Opens `docs\html\index.html` automatically.

### What is generated

| Section                  | Description                                                       |
|--------------------------|-------------------------------------------------------------------|
| Module list              | File-level descriptions with IEC 62304 unit IDs                   |
| Data structure reference | `VitalSigns`, `Alert`, `PatientRecord`, `AlertLevel`              |
| Function reference       | Parameters, return values, pre/postconditions, requirement tags   |
| Requirement traceability | `SWR-*` Doxygen tags cross-referenced to test cases               |
| Call graphs              | Per-function call and caller diagrams (SVG)                       |
| Include dependency graph | Module dependency visualisation                                   |
| Source browser           | Annotated source with cross-references                            |
| Warnings log             | `docs\doxygen_warnings.log` — undocumented items                  |

---

## Standards Compliance

| Standard                      | Area                                              |
|-------------------------------|---------------------------------------------------|
| **IEC 62304:2006+AMD1:2015**  | SW development lifecycle, Class B                 |
| **IEC 60601-1-8**             | Configurable alarm limits, three-zone alerts      |
| **IEC 80601-2-49**            | Multifunction patient monitoring (resp rate)      |
| **NEWS2 (RCP 2017)**          | National Early Warning Score 2                    |
| **FDA SW Validation Guidance**| Test strategy, traceability, coverage             |
| **AHA/ACC 2019**              | Heart rate and blood pressure thresholds          |
| **JNC-8 / ESC 2018**         | Hypertension classification                       |
| **WHO Clinical References**   | Temperature ranges, BMI categories                |
| **British Thoracic Society**  | SpO2 supplemental oxygen thresholds               |

### Requirements traceability

| Requirement ID  | Module                    | Test File                        |
|-----------------|---------------------------|----------------------------------|
| SWR-VIT-001–008 | `vitals.c`                | `test_vitals.cpp`                |
| SWR-NEW-001     | `news2.c`                 | `test_news2.cpp`                 |
| SWR-ALM-001     | `alarm_limits.c`          | `test_alarm_limits.cpp`          |
| SWR-TRD-001     | `trend.c`                 | `test_trend.cpp`                 |
| SWR-ALT-001–004 | `alerts.c`                | `test_alerts.cpp`                |
| SWR-PAT-001–006 | `patient.c`               | `test_patient.cpp`               |
| SWR-GUI-001–002 | `gui_auth.c`              | `test_auth.cpp`                  |
| SWR-GUI-003–004 | `gui_main.c`              | GUI demonstration                |
| SWR-GUI-005–006 | `hw_vitals.h`/`sim_vitals.c` | `test_hal.cpp` + GUI demo     |
| SWR-SEC-001–004 | `gui_users.c`, `pw_hash.c`| `test_auth.cpp`                  |
| SWR-GUI-007     | `gui_users.c`/`gui_main.c`| `test_auth.cpp` — UserManagement |
| SWR-GUI-008-010 | `gui_main.c`, `app_config.c`| `test_config.cpp` + visual    |
| SWR-GUI-011     | `gui_main.c`              | GUI demonstration                |
| SWR-INT-MON     | All modules               | `test_patient_monitoring.cpp`    |
| SWR-INT-ESC     | All modules               | `test_alert_escalation.cpp`      |

Full traceability matrix: `requirements/TRACEABILITY.md`

---

## Repository Structure

```
medvital-monitor/
├── CMakeLists.txt                   # Root build configuration
├── Doxyfile                         # Doxygen documentation configuration
├── README.md                        # This file
│
├── include/
│   ├── vitals.h                     # Vital signs types and validation API
│   ├── alerts.h                     # Alert record structure and generation API
│   ├── patient.h                    # Patient record management API
│   ├── gui_auth.h                   # GUI authentication API
│   ├── gui_users.h                  # Multi-user account management API
│   ├── hw_vitals.h                  # Hardware Abstraction Layer interface
│   ├── news2.h                      # NEWS2 Early Warning Score API
│   ├── alarm_limits.h               # Configurable alarm limits API
│   ├── trend.h                      # Trend direction + sparkline API
│   └── app_config.h                 # Application configuration persistence
│
├── src/
│   ├── vitals.c                     # Vital sign validation + BMI  (UNIT-VIT)
│   ├── alerts.c                     # Alert record generation      (UNIT-ALT)
│   ├── patient.c                    # Patient record management     (UNIT-PAT)
│   ├── gui_auth.c                   # Auth delegation layer         (UNIT-GUI)
│   ├── gui_users.c                  # Multi-user account management (UNIT-USR)
│   ├── pw_hash.c                    # SHA-256 password hashing      (UNIT-SEC)
│   ├── sim_vitals.c                 # Simulation HAL back-end       (UNIT-SIM)
│   ├── news2.c                      # NEWS2 scoring engine          (UNIT-NEW)
│   ├── alarm_limits.c               # Alarm limit config + check    (UNIT-ALM)
│   ├── trend.c                      # Trend analysis + extraction   (UNIT-TRD)
│   ├── app_config.c                 # Config persistence            (UNIT-CFG)
│   ├── gui_main.c                   # Win32 GUI (5 windows)         (UNIT-GUI)
│   └── main.c                       # Console entry point
│
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_vitals.cpp          # 80 tests — SWR-VIT-001–008
│   │   ├── test_alerts.cpp          # 11 tests — SWR-ALT
│   │   ├── test_patient.cpp         # 29 tests — SWR-PAT
│   │   ├── test_auth.cpp            # 41 tests — SWR-GUI/SEC
│   │   ├── test_news2.cpp           # 53 tests — SWR-NEW-001
│   │   ├── test_alarm_limits.cpp    # 31 tests — SWR-ALM-001
│   │   ├── test_trend.cpp           # 18 tests — SWR-TRD-001
│   │   ├── test_hal.cpp             # 12 tests — SWR-GUI-005/006
│   │   ├── test_config.cpp          # 10 tests — SWR-GUI-010
│   │   └── test_localization.cpp    # 8 tests — SWR-GUI-012
│   └── integration/
│       ├── test_patient_monitoring.cpp  # 7 tests — SWR-PAT-*, SWR-VIT-*, SWR-ALT-*
│       └── test_alert_escalation.cpp    # 7 tests — SWR-VIT-*, SWR-ALT-*, SWR-PAT-007
│
├── requirements/
|   |-- UNS.md                       # User Needs (17 items)
|   |-- SYS.md                       # System Requirements (21 items)
│   ├── SWR.md                       # Software Requirements (40 items)
│   └── TRACEABILITY.md              # RTM — 17/17 UNS, 40/40 SWR, 307 tests
│
├── build.bat                        # Configure + build + launch GUI
|-- run_tests.bat                    # Run all 307 tests
├── run_coverage.bat                 # GCC coverage report (gcov + gcovr)
├── generate_docs.bat                # Doxygen HTML + XML documentation
├── create_installer.bat             # Build release + compile Windows installer
├── installer.iss                    # Inno Setup 6 installer script
├── make_icon.py                     # Regenerate resources/app.ico from source
│
├── resources/
│   ├── app.ico                      # Medical cross icon (16/32/48 px)
│   └── app.rc                       # Windows resource script (icon + version info)
│
├── dvt/
│   ├── run_dvt.py                   # DVT automation script
│   └── results/                     # DVT execution reports
│
└── dist/                            # Release artifacts (exe, zip, installer)
```

