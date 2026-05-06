# DevOps Workflows

**Document ID:** SCM-003-REV-A
**Version:** 1.0.0
**Date:** 2026-04-08
**Parent document:** SCM Plan (SCM-001-REV-A)
**Standard:** IEC 62304 §5.5, §5.6, §5.8 — Verification, validation, release

---

## 1. Overview

This document describes the CI/CD workflows that automate build, analysis,
testing, verification, and release for the Patient Vital Signs Monitor.

All workflows run on GitHub Actions. The primary workflow is the **gated
pipeline** (`pipeline.yml`) which enforces a strict stage sequence via `needs`
dependencies. Individual workflows are retained for scheduled scans and manual
dispatch.

## 2. Pipeline Architecture

```
 ┌─────────────────────────────────────────────────────────────────────────┐
 │                        GATED PIPELINE (pipeline.yml)                    │
 │                                                                         │
 │  ┌───────────┐    ┌──────────────────┐    ┌───────────────────────┐     │
 │  │ 1. BUILD  │───▸│ 2a. CPPCHECK     │───▸│ 3. UNIT & INTEGRATION │     │
 │  │   ALL     │    │    (ubuntu)       │    │    TESTS              │     │
 │  │ (windows) │    ├──────────────────┤    │    312 GTest cases    │     │
 │  └───────────┘    │ 2b. CODEQL       │───▸│    (windows)          │     │
 │                   │    (windows)      │    └───────────┬───────────┘     │
 │                   └──────────────────┘                │                 │
 │                                                        ▼                 │
 │  ┌───────────────────────┐    ┌───────────┐    ┌───────────────────┐   │
 │  │ 6. RELEASE ARTEFACTS  │◂───│ 5. DVT    │◂───│ 4. CODE COVERAGE  │   │
 │  │    (tags only)        │    │  Report   │    │    gcovr           │   │
 │  │    GUI + Installer    │    │  IEC 62304│    │    line + branch   │   │
 │  └───────────────────────┘    └───────────┘    └───────────────────┘   │
 └─────────────────────────────────────────────────────────────────────────┘
```

**Dependency chain (enforced by `needs`):**

```
build ──▸ static-analysis ──┐
                             ├──▸ tests ──▸ coverage ──▸ dvt ──▸ release
build ──▸ sast ─────────────┘
```

If any stage fails, all downstream stages are skipped. The release stage
additionally requires `startsWith(github.ref, 'refs/tags/v')`.

## 3. Triggers

### 3.1 Pipeline (pipeline.yml)

| Event              | Trigger                                     | Stages Run    |
|--------------------|---------------------------------------------|---------------|
| Push to `main`     | Every commit merged to main                 | 1 → 2 → 3 → 4 → 5 |
| Pull request       | PR opened/updated targeting `main`          | 1 → 2 → 3 → 4 → 5 |
| Tag push (`v*`)    | `git push origin v2.9.0`                    | 1 → 2 → 3 → 4 → 5 → 6 |
| Manual dispatch    | Actions → Pipeline → Run workflow           | 1 → 2 → 3 → 4 → 5 |

### 3.2 Individual Workflows (schedule / manual)

| Workflow              | Schedule               | Manual Dispatch |
|-----------------------|------------------------|-----------------|
| `ci.yml`              | —                      | Yes             |
| `static-analysis.yml` | Monday 03:30 UTC       | Yes             |
| `codeql.yml`          | Monday 03:00 UTC       | Yes             |
| `dvt.yml`             | —                      | Yes             |
| `release.yml`         | —                      | Yes (tag input) |

## 4. Stage Details

### 4.1 Stage 1 — Build All

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `windows-latest`                              |
| Toolchain   | MinGW-w64 GCC, Ninja generator                |
| Targets     | `test_unit`, `test_integration`, `patient_monitor_gui` |
| Cache       | GTest FetchContent (`build/_deps`)             |
| Duration    | ~2–3 minutes                                   |

**What it verifies:**
- All source files compile without errors or warnings (treated as errors)
- GTest is fetched and builds successfully
- Both test and GUI targets link correctly

### 4.2 Stage 2a — Static Analysis (cppcheck)

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `ubuntu-latest`                               |
| Tool        | cppcheck (apt-get install)                     |
| Scope       | `vitals.c`, `alerts.c`, `patient.c`, `gui_auth.c`, `gui_users.c` |
| Output      | SARIF → GitHub Code Scanning                   |
| Suppressions| `unusedFunction`, `missingIncludeSystem`, `missingInclude` |

**What it verifies:**
- No null-pointer dereferences, uninitialised variables, buffer overflows
- No code quality issues (redundant code, missing braces)
- No portability issues (undefined/implementation-defined behaviour)

**IEC 62304 reference:** §5.5.4 — static analysis is a required verification
activity for Class B software.

### 4.3 Stage 2b — SAST (CodeQL)

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `windows-latest`                              |
| Tool        | GitHub CodeQL (`security-extended` query suite)|
| Scope       | All C/C++ code (excluding `build/_deps`)       |
| Output      | SARIF → GitHub Code Scanning                   |

**What it verifies:**
- CWE-120 (buffer copy without size check)
- CWE-134 (format string vulnerability)
- CWE-190 (integer overflow)
- CWE-476 (null pointer dereference)
- ~80 additional security rules

### 4.4 Stage 3 — Unit & Integration Tests

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `windows-latest`                              |
| Framework   | Google Test (release-1.10.0)                   |
| Unit tests  | 298 tests across 32 suites                     |
| Integration | 14 tests across 2 suites                       |
| Output      | JUnit XML → artefact (90-day retention)        |
| Summary     | Pass/fail table in Actions job summary         |

**Automated test files / suite groups:**

| Group                    | Tests | Covers                          |
|--------------------------|------:|----------------------------------|
| `test_vitals.cpp`        | 80    | SWR-VIT-001..008                 |
| `test_alerts.cpp`        | 11    | SWR-ALT-001..004                 |
| `test_patient.cpp`       | 29    | SWR-PAT-001..008                 |
| `test_news2.cpp`         | 53    | SWR-NEW-001                      |
| `test_alarm_limits.cpp`  | 31    | SWR-ALM-001                      |
| `test_trend.cpp`         | 18    | SWR-TRD-001                      |
| `test_auth.cpp`          | 41    | SWR-GUI-001..002, SWR-GUI-007, SWR-SEC-001..004 |
| `test_hal.cpp`           | 12    | Supporting HAL / simulator checks |
| `test_config.cpp`        | 10    | Supporting config persistence checks |
| `test_localization.cpp`  | 8     | SWR-GUI-012                      |
| `test_dashboard_freshness.cpp` | 5 | SWR-GUI-014                      |
| `test_patient_monitoring.cpp` | 7 | End-to-end vital to alert flow plus historical event retention |
| `test_alert_escalation.cpp` | 7  | Alert to NEWS2 to alarm escalation plus parameter-set review transitions |

**Gate:** 100% pass rate required. Any failure blocks downstream stages.

### 4.5 Stage 4 — Code Coverage

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `windows-latest`                              |
| Tool        | GCC `--coverage` + gcovr                       |
| Scope       | `src/` (production code only, not tests)       |
| Output      | Cobertura XML → artefact                       |
| Summary     | Line + branch percentages in job summary       |

**IEC 62304 Class B targets:**
- Statement coverage: required
- Branch (decision) coverage: required
- Source-level measurement: `gcovr --filter 'src/'`

### 4.6 Stage 5 — DVT (Design Verification Tests)

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `windows-latest`                              |
| Protocol    | `dvt/DVT_Protocol.md` (DVT-001-REV-A)         |
| Output      | Text report + JUnit XML → artefact (90 days)  |

**DVT report contents:**
- Timestamped test results (unit + integration)
- Pass/fail verdict per suite
- Overall pass criteria check
- Repository, commit, and ref identification
- Protocol and traceability matrix references

**Gate:** Overall verdict must be PASS. Any failure blocks release.

### 4.7 Stage 6 — Release Artefacts

| Property    | Value                                         |
|-------------|-----------------------------------------------|
| Runner      | `windows-latest`                              |
| Condition   | `startsWith(github.ref, 'refs/tags/v')`       |
| Artefacts   | GUI `.exe`, Inno Setup installer, portable ZIP |
| Destination | GitHub Releases (permanent retention)          |

**Release artefacts:**

| Artefact                          | Description                        |
|-----------------------------------|------------------------------------|
| `PatientMonitor-v<x.y.z>.exe`    | Standalone GUI executable          |
| `PatientMonitor-v<x.y.z>-portable.zip` | Portable archive with README |
| `PatientMonitorSetup-<x.y.z>.exe` | Inno Setup Windows installer     |

## 5. Artefact Retention

| Artefact Type          | Retention  | Location                      |
|------------------------|------------|-------------------------------|
| Test results (XML)     | 90 days    | GitHub Actions artefacts      |
| DVT reports            | 90 days    | GitHub Actions artefacts      |
| Coverage reports       | 90 days    | GitHub Actions artefacts      |
| SARIF scan results     | Permanent  | GitHub Security Code Scanning |
| Release binaries       | Permanent  | GitHub Releases               |
| Git history            | Permanent  | Git repository                |

> **Regulatory note:** For IEC 62304 audit, increase retention to match your
> device lifecycle documentation requirements. GitHub Actions artefact retention
> can be extended in repository settings or by archiving to external storage.

## 6. Environment and Secrets

### 6.1 Environment Variables

| Variable                            | Purpose                            |
|-------------------------------------|------------------------------------|
| `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24`| Force Node.js 24 for GitHub Actions|

### 6.2 Secrets

| Secret          | Purpose                                  | Used In       |
|-----------------|------------------------------------------|---------------|
| `GITHUB_TOKEN`  | Automatic token for release uploads      | Stage 6       |

No additional secrets are required. The project uses no external services.

## 7. Local Development Workflow

### 7.1 First-Time Setup

```bash
git clone https://github.com/vinu-dev/medvital-monitor.git
cd medvital-monitor

# Install Git hooks (enforces branch names, commit messages, basic lint)
./scripts/install-hooks.sh

# Build
cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DBUILD_TESTS=ON
cmake --build build

# Run tests
./build/tests/test_unit.exe
./build/tests/test_integration.exe
```

### 7.2 Daily Workflow

```bash
# 1. Start from latest main
git checkout main && git pull

# 2. Create feature branch
git checkout -b feature/SWR-xxx-description

# 3. Develop (hooks enforce commit message format)
git add -A && git commit -m "feat(module): description"

# 4. Push (hook validates branch name, blocks direct main push)
git push -u origin feature/SWR-xxx-description

# 5. Open PR on GitHub — template auto-fills checklist

# 6. Pipeline runs automatically — monitor in Actions tab

# 7. Address review feedback, push fixes

# 8. Squash merge after approval + green pipeline
```

### 7.3 Release Workflow

```bash
# 1. Ensure main is green (all pipeline stages pass)
# 2. Update version in CMakeLists.txt
# 3. Commit: git commit -am "chore: bump version to 2.9.0"
# 4. Tag:    git tag v2.9.0
# 5. Push:   git push origin main v2.9.0
# 6. Pipeline stage 6 creates GitHub Release with artefacts
```

## 8. Troubleshooting

### 8.1 Pipeline Failures

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Stage 1 fails | Compilation error | Read build log, fix source code |
| Stage 2a fails | cppcheck finding | Review SARIF in Code Scanning, fix or suppress |
| Stage 2b fails | CodeQL finding | Review alert in Security tab, fix code |
| Stage 3 fails | Test assertion failure | Run tests locally, debug failing case |
| Stage 4 fails | gcovr error | Check coverage flags, ensure tests run |
| Stage 5 fails | DVT verdict FAIL | Same as stage 3 — fix failing tests |
| Stage 6 fails | Build or installer error | Check CMake release config, Inno Setup script |

### 8.2 Hook Issues

| Symptom | Fix |
|---------|-----|
| Hook not running | Run `./scripts/install-hooks.sh` |
| Commit blocked by commit-msg hook | Fix message format (see BRANCHING_STRATEGY.md §4) |
| Push blocked by pre-push hook | Rename branch to match required pattern |
| Need to bypass hook (emergency) | `git commit --no-verify` (requires justification in PR) |

### 8.3 Cache Issues

If builds are picking up stale GTest sources:

```bash
# Delete GitHub Actions cache via API or UI
# Or change the cache key by modifying CMakeLists.txt slightly
```

## 9. Workflow Files Reference

| File | Purpose | Trigger |
|------|---------|---------|
| `.github/workflows/pipeline.yml` | Gated 6-stage pipeline | Push main, PR, tags, manual |
| `.github/workflows/ci.yml` | Standalone build + test | Manual only |
| `.github/workflows/static-analysis.yml` | cppcheck scan | Weekly Monday 03:30 UTC, manual |
| `.github/workflows/codeql.yml` | CodeQL SAST scan | Weekly Monday 03:00 UTC, manual |
| `.github/workflows/dvt.yml` | DVT report generation | Manual only |
| `.github/workflows/release.yml` | Release artefact build | Manual only (tag input) |

## 10. Document History

| Revision | Date       | Author         | Changes                          |
|----------|------------|----------------|----------------------------------|
| REV-A    | 2026-04-08 | SCM Admin      | Initial release                  |
