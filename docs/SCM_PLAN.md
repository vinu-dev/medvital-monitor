# Software Configuration Management Plan

**Document ID:** SCM-001-REV-A
**Version:** 1.0.0
**Date:** 2026-04-08
**Standard:** IEC 62304 §5.1.9, §8 — Software configuration management
**Classification:** IEC 62304 Class B

---

## 1. Purpose

This SCM Plan defines the configuration management practices for the Patient
Vital Signs Monitor — an IEC 62304 Class B medical device software. It ensures
that every change to source code, documentation, requirements, and build
artefacts is traceable, reproducible, and auditable throughout the software
lifecycle.

**Regulatory references:**

| Standard              | Clause                  | Topic                          |
|-----------------------|-------------------------|--------------------------------|
| IEC 62304:2006+A1     | §5.1.9, §8             | SCM planning and activities    |
| IEC 62304:2006+A1     | §8.1                   | Configuration identification   |
| IEC 62304:2006+A1     | §8.2                   | Change control                 |
| IEC 62304:2006+A1     | §8.3                   | Configuration status accounting|
| FDA SW Guidance        | §5.2.3                 | SCM                            |
| ISO 13485:2016         | §7.5.6                 | Control of changes             |

## 2. Scope

This plan covers:

- **Source code** — all `.c`, `.h`, `.cpp` files under `src/`, `include/`, `tests/`
- **Build system** — `CMakeLists.txt`, `installer.iss`, `resources/`
- **Requirements** — `requirements/UNS.md`, `SYS.md`, `SWR.md`, `TRACEABILITY.md`
- **Documentation** — `docs/`, `README.md`, `CLAUDE.md`
- **CI/CD configuration** — `.github/workflows/`, `.github/codeql/`
- **DVT artefacts** — `dvt/DVT_Protocol.md`, generated reports
- **Release artefacts** — executables, installers, portable ZIPs

## 3. Roles and Responsibilities

| Role                  | Responsibilities                                          |
|-----------------------|-----------------------------------------------------------|
| **Developer**         | Create branches, write code, run local tests, open PRs    |
| **Reviewer**          | Review PRs against requirements and checklist, approve     |
| **Release Manager**   | Tag releases, verify pipeline passes, publish artefacts    |
| **SCM Administrator** | Maintain branch protection rules, hooks, pipeline config   |

## 4. Configuration Identification

### 4.1 Configuration Items (CIs)

| CI Category          | Location                       | Naming Convention              |
|----------------------|--------------------------------|--------------------------------|
| Source code          | `src/`, `include/`             | Module-based (vitals.c, etc.)  |
| Test code            | `tests/unit/`, `tests/integration/` | `test_<module>.cpp`       |
| Requirements         | `requirements/`                | `UNS.md`, `SYS.md`, `SWR.md`  |
| Traceability matrix  | `requirements/TRACEABILITY.md` | RTM-001-REV-x                  |
| Architecture         | `docs/ARCHITECTURE.md`         | ARCH-001-REV-x                 |
| DVT protocol         | `dvt/DVT_Protocol.md`          | DVT-001-REV-x                  |
| Build configuration  | `CMakeLists.txt`               | Versioned via `project()`      |
| CI/CD pipeline       | `.github/workflows/`           | `pipeline.yml` + individual    |
| Release artefacts    | GitHub Releases                | `v<MAJOR>.<MINOR>.<PATCH>`     |

### 4.2 Version Numbering

Semantic Versioning (SemVer) is used: `MAJOR.MINOR.PATCH`

| Component | When to increment                                         |
|-----------|-----------------------------------------------------------|
| MAJOR     | Incompatible changes to clinical interfaces or data model |
| MINOR     | New features, new requirements (backward-compatible)       |
| PATCH     | Bug fixes, documentation updates, CI/CD improvements       |

The authoritative version is defined in `CMakeLists.txt`:
```cmake
project(PatientMonitor VERSION 2.8.0 LANGUAGES C CXX)
```

Git tags use the `v` prefix: `v2.8.0`, `v2.8.1`, `v2.8.2`.

### 4.3 Baseline Identification

A **baseline** is established at each tagged release. The Git tag SHA
permanently identifies the exact state of all configuration items.

```
Baseline = Git tag (v<x.y.z>) → SHA → full source tree snapshot
```

## 5. Branching Strategy

The full branching strategy is documented in `docs/BRANCHING_STRATEGY.md`.

### 5.1 Summary

| Branch Pattern           | Purpose                   | Merges Into  | Lifetime    |
|--------------------------|---------------------------|-------------|-------------|
| `main`                   | Production-ready baseline | —           | Permanent   |
| `feature/<ticket>-<desc>`| New features              | `main`      | Temporary   |
| `fix/<ticket>-<desc>`    | Bug fixes                 | `main`      | Temporary   |
| `hotfix/<ticket>-<desc>` | Critical production fixes | `main`      | Temporary   |
| `release/<version>`      | Release stabilisation     | `main`      | Temporary   |
| `docs/<desc>`            | Documentation-only        | `main`      | Temporary   |

### 5.2 Rules (enforced by hooks — see §7)

1. **No direct pushes to `main`** — all changes via Pull Request
2. **Branch names must match the pattern** — enforced by pre-push hook
3. **Commits must reference a requirement or ticket** — enforced by commit-msg hook
4. **PRs must pass all 6 pipeline stages** before merge
5. **PRs must use the change checklist** (auto-populated from template)

## 6. Change Control Process

### 6.1 Change Request Flow

```
 ┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
 │  1. Create   │────▸│  2. Develop  │────▸│  3. Review   │────▸│  4. Merge   │
 │    Branch    │     │  & Commit   │     │    PR        │     │  to main    │
 └─────────────┘     └─────────────┘     └─────────────┘     └──────┬──────┘
                                                                      │
                      ┌─────────────┐     ┌─────────────┐            │
                      │  6. Release  │◂────│  5. Tag      │◂───────────┘
                      │  Artefacts  │     │  Version    │
                      └─────────────┘     └─────────────┘
```

### 6.2 Step Details

| Step | Action | Gate | Artefact |
|------|--------|------|----------|
| 1. Create branch | `git checkout -b feature/TICKET-desc` | Branch name validated by pre-push hook | — |
| 2. Develop & commit | Write code, tests, docs | Commit message validated by commit-msg hook | Commits |
| 3. Open PR | Submit PR against `main` | Template checklist auto-populated | PR |
| 4. Pipeline passes | 6-stage gated pipeline runs | All stages must pass | Test results, coverage, DVT report |
| 5. Review & merge | Reviewer approves, squash-merge | At least 1 approval required | Merge commit on `main` |
| 6. Tag & release | `git tag v<x.y.z>` and push | Pipeline stage 6 creates artefacts | Executable, installer, ZIP |

### 6.3 Change Impact Assessment

Every PR must complete the change checklist (`.github/PULL_REQUEST_TEMPLATE.md`):

- [ ] User needs reviewed and updated if needed
- [ ] System requirements reviewed and updated if needed
- [ ] Software requirements reviewed and updated if needed
- [ ] README or other documentation reviewed and updated if needed
- [ ] Unit tests reviewed and updated if needed
- [ ] Integration tests reviewed and updated if needed
- [ ] DVT tests reviewed and updated if needed
- [ ] Coverage impact reviewed
- [ ] Release artifact impact reviewed

## 7. Enforcement Mechanisms

### 7.1 Git Hooks (Local)

Local enforcement scripts are installed by running:

```bash
./scripts/install-hooks.sh
```

| Hook         | Script                          | Enforcement                              |
|--------------|---------------------------------|------------------------------------------|
| `commit-msg` | `scripts/hooks/commit-msg`      | Validates commit message format           |
| `pre-push`   | `scripts/hooks/pre-push`        | Blocks direct push to `main`, validates branch names |
| `pre-commit` | `scripts/hooks/pre-commit`      | Runs basic lint checks on staged files    |

### 7.2 CI/CD Pipeline (Remote)

The gated pipeline (`.github/workflows/pipeline.yml`) enforces:

| Stage | Gate                                      | Blocks Release If |
|-------|-------------------------------------------|--------------------|
| 1     | Build succeeds                            | Compilation errors |
| 2a    | cppcheck — zero errors                    | Static analysis findings |
| 2b    | CodeQL — zero high/critical               | Security vulnerabilities |
| 3     | 145 tests pass (100% pass rate)           | Any test failure |
| 4     | Coverage report generated                 | Coverage regression |
| 5     | DVT report — PASS                         | Verification failure |
| 6     | Release artefacts built (tags only)       | Build failure |

### 7.3 Branch Protection (GitHub)

Recommended branch protection rules for `main`:

| Rule                                          | Setting  |
|-----------------------------------------------|----------|
| Require pull request before merging            | Yes      |
| Required approvals                             | ≥ 1      |
| Require status checks to pass (Pipeline)       | Yes      |
| Require branches to be up to date              | Yes      |
| Require conversation resolution                | Yes      |
| Do not allow bypassing above settings           | Yes      |
| Restrict who can push to matching branches      | Admins   |

To configure via GitHub API, run:

```bash
./scripts/configure-branch-protection.sh
```

## 8. Configuration Status Accounting

### 8.1 Traceability

Full bidirectional traceability is maintained in `requirements/TRACEABILITY.md`:

```
User Need (UNS) → System Req (SYS) → Software Req (SWR) → Code → Test
```

### 8.2 Audit Trail

| Record               | Location                                  |
|-----------------------|-------------------------------------------|
| All code changes      | Git history (`git log`)                   |
| PR reviews            | GitHub Pull Request history                |
| Test results (XML)    | GitHub Actions artefacts (90-day retention)|
| DVT reports           | GitHub Actions artefacts (90-day retention)|
| Coverage reports      | GitHub Actions artefacts (90-day retention)|
| SARIF scan results    | GitHub Security → Code Scanning            |
| Release artefacts     | GitHub Releases (permanent)                |

### 8.3 Reporting

The pipeline writes summaries to the GitHub Actions job summary page:

- **Stage 3:** Test results table (suite / total / passed / failed / status)
- **Stage 4:** Coverage percentages (line + branch)
- **Stage 5:** DVT pass/fail verdict with protocol reference

## 9. Configuration Audits

### 9.1 Per-Release Audit Checklist

Before tagging a release, verify:

- [ ] All PRs since last release are merged and reviewed
- [ ] `CMakeLists.txt` version matches intended release tag
- [ ] `requirements/TRACEABILITY.md` is complete (no gaps)
- [ ] Pipeline passes on `main` (all 6 stages green)
- [ ] DVT report shows PASS for all test suites
- [ ] Coverage meets IEC 62304 Class B targets
- [ ] Release notes document all changes since last release
- [ ] No open critical/high findings in Code Scanning

### 9.2 Periodic Audit

Quarterly review of:

- Branch protection rules are active and correctly configured
- Git hooks are installed on all developer machines
- Scheduled workflows (weekly CodeQL, cppcheck) are running
- Artefact retention policies are adequate for regulatory needs

## 10. Tools

| Tool                 | Purpose                        | Version/Source           |
|----------------------|--------------------------------|--------------------------|
| Git                  | Version control                | ≥ 2.30                  |
| GitHub               | Repository hosting, CI/CD      | github.com               |
| GitHub Actions       | CI/CD pipeline                 | Workflows in `.github/`  |
| CMake + Ninja        | Build system                   | CMake ≥ 3.15             |
| GCC (MinGW-w64)      | C/C++ compiler                 | ≥ 7.0                   |
| Google Test          | Unit/integration testing       | release-1.10.0           |
| cppcheck             | Static analysis                | Latest via apt           |
| CodeQL               | SAST / security scanning       | github/codeql-action@v4  |
| gcovr                | Code coverage reporting        | Latest via pip           |
| Inno Setup 6         | Windows installer              | ≥ 6.7.1                 |
| Doxygen              | API documentation              | ≥ 1.9                   |

## 11. Document History

| Revision | Date       | Author         | Changes                          |
|----------|------------|----------------|----------------------------------|
| REV-A    | 2026-04-08 | SCM Admin      | Initial release                  |
