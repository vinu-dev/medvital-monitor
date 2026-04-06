# Patient Vital Signs Monitor

Medical device software for real-time patient vital sign monitoring and alert generation.
Built to **IEC 62304 Class B** and **FDA SW Validation Guidance** standards.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Modules](#modules)
4. [Alert Thresholds](#alert-thresholds)
5. [Build](#build)
6. [Testing](#testing)
7. [Code Coverage](#code-coverage)
8. [Documentation](#documentation)
9. [Standards Compliance](#standards-compliance)
10. [Repository Structure](#repository-structure)

---

## Overview

The application monitors up to 10 sequential vital sign readings per patient,
classifies each parameter against clinical thresholds, generates structured
alert records, and displays a formatted summary with overall patient status.

**Supported vital signs:**
- Heart rate (bpm)
- Blood pressure — systolic and diastolic (mmHg)
- Body temperature (°C)
- Peripheral oxygen saturation — SpO2 (%)
- Body Mass Index — BMI (derived from weight and height)

---

## Architecture

```
+----------------------------------------------------------+
|                    patient_monitor                        |
|                      (main.c)                            |
|  Orchestrates patient sessions, reads vitals, prints UI  |
+-----------------------------+----------------------------+
                              |
              +---------------v---------------+
              |          patient.c             |
              |   PatientRecord management     |
              |   - Init / add reading         |
              |   - Status query               |
              |   - Print summary              |
              +-------+---------------+--------+
                      |               |
         +------------v---+   +-------v---------+
         |   vitals.c     |   |   alerts.c      |
         | Parameter      |   | Alert record    |
         | validation     |   | generation      |
         | BMI / strings  |   | from VitalSigns |
         +----------------+   +-----------------+
```

### Data Flow

```
VitalSigns  -->  check_*()           --> AlertLevel  (per parameter)
VitalSigns  -->  overall_alert_level --> AlertLevel  (aggregate)
VitalSigns  -->  generate_alerts()   --> Alert[]     (display records)
Alert[]     -->  patient_print_summary / print_reading
```

### Memory Model

No heap allocation is used anywhere in the production code.
All storage is stack or statically sized:

| Structure     | Size                                  |
|---------------|---------------------------------------|
| `VitalSigns`  | 5 fields — ~20 bytes                  |
| `Alert`       | level + 32 + 96 bytes = ~132 bytes    |
| `PatientRecord` | PatientInfo + 10 × VitalSigns + int |

---

## Modules

### `vitals.c` / `vitals.h` — UNIT-VIT

Core classification engine. Validates each vital sign parameter against
AHA/ACC-derived clinical thresholds and returns an `AlertLevel`.

| Function                  | Description                                          |
|---------------------------|------------------------------------------------------|
| `check_heart_rate()`      | Classifies bpm → NORMAL / WARNING / CRITICAL         |
| `check_blood_pressure()`  | Classifies systolic + diastolic together             |
| `check_temperature()`     | Classifies °C → NORMAL / WARNING / CRITICAL          |
| `check_spo2()`            | Classifies SpO2 % → NORMAL / WARNING / CRITICAL      |
| `overall_alert_level()`   | Returns the highest level across all four parameters |
| `calculate_bmi()`         | BMI = weight / height²; returns -1.0 for invalid height |
| `bmi_category()`          | Maps BMI float → WHO category string                 |
| `alert_level_str()`       | Maps `AlertLevel` → "NORMAL" / "WARNING" / "CRITICAL"|

---

### `alerts.c` / `alerts.h` — UNIT-ALT

Translates a `VitalSigns` snapshot into a list of human-readable `Alert`
records, one per out-of-range parameter.

| Function            | Description                                                |
|---------------------|------------------------------------------------------------|
| `generate_alerts()` | Fills caller-supplied buffer; returns count of alerts written |

---

### `patient.c` / `patient.h` — UNIT-PAT

Top-level patient record management. Stores demographics and a history of
up to `MAX_READINGS` (10) vital sign readings.

| Function                    | Description                                      |
|-----------------------------|--------------------------------------------------|
| `patient_init()`            | Zero-fills and populates a `PatientRecord`       |
| `patient_add_reading()`     | Appends a reading; returns 0 if buffer is full   |
| `patient_latest_reading()`  | Returns pointer to last reading, or NULL         |
| `patient_current_status()`  | Overall `AlertLevel` from latest reading         |
| `patient_is_full()`         | True when `reading_count == MAX_READINGS`        |
| `patient_print_summary()`   | Formatted terminal output with alerts            |

---

## Alert Thresholds

| Parameter      | NORMAL           | WARNING                         | CRITICAL              |
|----------------|------------------|---------------------------------|-----------------------|
| Heart Rate     | 60–100 bpm       | 41–59 bpm / 101–150 bpm         | ≤ 40 / ≥ 151 bpm      |
| Systolic BP    | 90–140 mmHg      | 71–89 mmHg / 141–180 mmHg       | ≤ 70 / ≥ 181 mmHg     |
| Diastolic BP   | 60–90 mmHg       | 41–59 mmHg / 91–120 mmHg        | ≤ 40 / ≥ 121 mmHg     |
| Temperature    | 36.1–37.2 °C     | 35.0–36.0 °C / 37.3–39.5 °C    | < 35.0 / > 39.5 °C   |
| SpO2           | 95–100 %         | 90–94 %                         | < 90 %                |

*Sources: AHA/ACC 2019, JNC-8, ESC 2018, WHO, British Thoracic Society*

---

## Build

### Prerequisites

| Tool        | Minimum Version | Install                                         |
|-------------|-----------------|--------------------------------------------------|
| CMake       | 3.15            | `winget install Kitware.CMake`                   |
| MSVC        | VS 2019         | Visual Studio with "Desktop development with C++"|
| Git         | Any             | `winget install Git.Git`                         |

### Windows scripts (double-click in File Explorer)

| Script                     | Purpose                                              |
|----------------------------|------------------------------------------------------|
| `setup_gtest.bat`          | **Full clean build** — delete old build, configure, compile |
| `build.bat`                | Incremental rebuild + run the app                    |
| `run_tests.bat`            | Rebuild + run all 106 tests                          |
| `install_coverage_tools.bat` | Install OpenCppCoverage (one time)               |
| `run_coverage.bat`         | Run tests with coverage, generate HTML + XML report  |
| `generate_docs.bat`        | Generate Doxygen architecture + design documentation |

### Manual build (any terminal)

```bat
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Debug
build\Debug\patient_monitor.exe
```

---

## Testing

**Framework:** Google Test v1.14.0 (downloaded automatically via CMake FetchContent)

### Test counts

| File                              | Tests | Requirements            |
|-----------------------------------|-------|-------------------------|
| `tests/unit/test_vitals.cpp`      | 54    | REQ-VIT-001 – REQ-VIT-007 |
| `tests/unit/test_alerts.cpp`      | 11    | REQ-ALT-001 – REQ-ALT-005 |
| `tests/unit/test_patient.cpp`     | 17    | REQ-PAT-001 – REQ-PAT-006 |
| `tests/integration/test_patient_monitoring.cpp` | 6 | REQ-INT-MON-001–006 |
| `tests/integration/test_alert_escalation.cpp`   | 6 | REQ-INT-ESC-001–005 |
| **Total**                         | **94 unit + 12 integration = 106** | |

### Test techniques applied

| Technique                    | Applied to                                     |
|------------------------------|------------------------------------------------|
| Equivalence Partitioning     | All four vital sign functions                  |
| Boundary Value Analysis      | Every threshold boundary (±1 from each limit)  |
| Boundary Transition Accuracy | Heart rate and SpO2 full boundary sweep tables |
| Escalation / Deescalation    | NORMAL → WARNING → CRITICAL and back           |
| Multi-parameter crisis       | All four parameters critical simultaneously    |
| Capacity enforcement         | `patient_add_reading()` beyond MAX_READINGS    |
| Independence verification    | Two patients with different statuses           |

### Run tests

```bat
run_tests.bat
```

Or manually:
```bat
ctest --test-dir build -C Debug --output-on-failure
```

---

## Code Coverage

**Tool:** OpenCppCoverage 0.9.9.0 (MSVC / Windows)

| File        | Line Coverage |
|-------------|---------------|
| `alerts.c`  | **100%**      |
| `patient.c` | **100%**      |
| `vitals.c`  | **98%**       |
| **Overall** | **99.2%** (244 / 246 lines) |

### Reports

| File                         | Format              | Purpose                    |
|------------------------------|---------------------|----------------------------|
| `coverage_combined/index.html` | Interactive HTML  | Line-by-line review        |
| `coverage_report.xml`          | Cobertura XML     | DHF / audit trail record   |
| `build/results_unit.xml`       | JUnit XML         | Unit test execution record |
| `build/results_integration.xml`| JUnit XML         | Integration test record    |

### Run coverage

```bat
install_coverage_tools.bat   REM first time only
run_coverage.bat
```

### IEC 62304 coverage targets

| Coverage Type | Class B  | Class C (safety-critical)          |
|---------------|----------|------------------------------------|
| Statement     | 100%     | 100%                               |
| Branch        | 100%     | 100%                               |
| MC/DC         | N/A      | 100% — requires VectorCAST/BullseyeCoverage |

---

## Documentation

**Tool:** Doxygen 1.9+ with Graphviz (for diagrams)

### Install tools

```bat
winget install DimitriVanHeesch.Doxygen
winget install Graphviz.Graphviz
```

### Generate

```bat
generate_docs.bat
```

Opens `docs/html/index.html` in your browser automatically.

### What is generated

| Section                  | Description                                         |
|--------------------------|-----------------------------------------------------|
| Module list              | File-level descriptions with IEC 62304 unit IDs     |
| Data structure reference | `VitalSigns`, `Alert`, `PatientRecord`, `AlertLevel` |
| Function reference       | Parameters, return values, pre/postconditions, `@req` tags |
| Requirement traceability | `REQ-VIT`, `REQ-ALT`, `REQ-PAT`, `REQ-INT` cross-references |
| Call graphs              | Per-function call and caller diagrams (SVG)         |
| Include dependency graph | Module dependency visualisation                     |
| Source browser           | Annotated source with cross-references              |
| Warnings log             | `docs/doxygen_warnings.log` — undocumented items    |

---

## Standards Compliance

| Standard                        | Area                                            |
|---------------------------------|-------------------------------------------------|
| **IEC 62304:2006+AMD1:2015**    | SW development lifecycle, Class B               |
| **FDA SW Validation Guidance**  | Test strategy, traceability, coverage           |
| **AHA/ACC 2019**                | Heart rate and blood pressure thresholds        |
| **JNC-8 / ESC 2018**            | Hypertension classification                     |
| **WHO Clinical References**     | Temperature ranges, BMI categories              |
| **British Thoracic Society**    | SpO2 supplemental oxygen thresholds             |

### Traceability matrix

| Requirement ID | Module     | Test File              |
|----------------|------------|------------------------|
| REQ-VIT-001–007| `vitals.c` | `test_vitals.cpp`      |
| REQ-ALT-001–005| `alerts.c` | `test_alerts.cpp`      |
| REQ-PAT-001–006| `patient.c`| `test_patient.cpp`     |
| REQ-INT-MON    | All        | `test_patient_monitoring.cpp` |
| REQ-INT-ESC    | All        | `test_alert_escalation.cpp`   |

---

## Repository Structure

```
medicalUT_IT/
├── CMakeLists.txt                  # Root build configuration
├── Doxyfile                        # Doxygen documentation configuration
├── README.md                       # This file
│
├── include/
│   ├── vitals.h                    # Vital signs types and validation API
│   ├── alerts.h                    # Alert record structure and generation API
│   └── patient.h                   # Patient record management API
│
├── src/
│   ├── vitals.c                    # Vital sign validation + BMI (UNIT-VIT)
│   ├── alerts.c                    # Alert record generation (UNIT-ALT)
│   ├── patient.c                   # Patient record management (UNIT-PAT)
│   └── main.c                      # Application entry point
│
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_vitals.cpp         # 54 unit tests — REQ-VIT
│   │   ├── test_alerts.cpp         # 11 unit tests — REQ-ALT
│   │   └── test_patient.cpp        # 17 unit tests — REQ-PAT
│   └── integration/
│       ├── test_patient_monitoring.cpp  # 6 tests — REQ-INT-MON
│       └── test_alert_escalation.cpp    # 6 tests — REQ-INT-ESC
│
├── build.bat                       # Incremental build + run app
├── setup_gtest.bat                 # Full clean build
├── run_tests.bat                   # Run all 106 tests
├── run_coverage.bat                # Generate coverage report
├── install_coverage_tools.bat      # Install OpenCppCoverage
└── generate_docs.bat               # Generate Doxygen documentation
```
