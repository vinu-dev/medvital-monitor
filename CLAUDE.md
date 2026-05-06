# CLAUDE.md — Project Instructions

## Change Checklist

Before submitting any change, verify the following:

- [ ] User needs reviewed and updated if needed
- [ ] System requirements reviewed and updated if needed
- [ ] Software requirements reviewed and updated if needed
- [ ] README or other documentation reviewed and updated if needed
- [ ] Unit tests reviewed and updated if needed
- [ ] Integration tests reviewed and updated if needed
- [ ] DVT tests reviewed and updated if needed
- [ ] Coverage impact reviewed
- [ ] Release artifact impact reviewed

## CI/CD Pipeline

The pipeline (`.github/workflows/pipeline.yml`) enforces this sequence — each stage gates the next via `needs`:

1. **Build All** — CMake + Ninja, all targets (Windows / MinGW-w64)
2. **Static Analysis + SAST** — cppcheck + CodeQL (parallel, both gate on build)
3. **Unit & Integration Tests** — GTest, 317 test cases
4. **Code Coverage** — gcovr, source-level line + branch coverage
5. **DVT** — Design Verification Tests with IEC 62304 report
6. **Publish Reports** — GitHub Pages dashboard (main branch only)

Release artifacts (GUI + Installer + ZIP) are built by `release.yml` on tag push (`v*`)
or manual `workflow_dispatch`. This avoids duplicate pipeline runs.

Individual workflows (`ci.yml`, `static-analysis.yml`, `codeql.yml`, `dvt.yml`, `release.yml`)
are kept for scheduled runs and manual `workflow_dispatch` triggers.

## SCM Documentation

- `docs/SCM_PLAN.md` — Configuration management plan (IEC 62304 §8)
- `docs/BRANCHING_STRATEGY.md` — Branch naming, commit conventions, merge rules
- `docs/DEVOPS_WORKFLOWS.md` — Pipeline stages, artefacts, troubleshooting
- `scripts/install-hooks.sh` — Install local Git hooks (run once per clone)

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON
cmake --build build
```

## Tests

```bash
build/tests/test_unit.exe
build/tests/test_integration.exe
```
