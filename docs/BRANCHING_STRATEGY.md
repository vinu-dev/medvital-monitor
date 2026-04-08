# Branching Strategy

**Document ID:** SCM-002-REV-A
**Version:** 1.0.0
**Date:** 2026-04-08
**Parent document:** SCM Plan (SCM-001-REV-A)
**Standard:** IEC 62304 §8 — Software configuration management

---

## 1. Overview

This document defines the Git branching model for the Patient Vital Signs
Monitor. The strategy is designed for IEC 62304 Class B compliance: every
change is traceable, reviewed, and verified through the gated CI/CD pipeline
before reaching the production baseline.

**Principles:**

1. `main` is always releasable — it represents the current production baseline
2. All work happens on short-lived topic branches
3. Every change enters `main` through a reviewed, pipeline-verified Pull Request
4. Releases are cut by tagging `main` — never from a side branch
5. Branch naming is enforced by Git hooks

## 2. Branch Types

### 2.1 Permanent Branch

| Branch | Purpose | Protection |
|--------|---------|------------|
| `main` | Production-ready baseline. Every commit is verified by the full 6-stage pipeline. | Protected: no direct push, PR required, status checks required |

### 2.2 Temporary Branches

All temporary branches are created from `main` and merge back into `main`
via Pull Request. They are deleted after merge.

| Pattern | Purpose | Example |
|---------|---------|---------|
| `feature/<id>-<desc>` | New functionality or requirement implementation | `feature/SWR-GUI-012-localization` |
| `fix/<id>-<desc>` | Bug fix for an existing requirement or defect | `fix/BUG-042-spo2-boundary` |
| `hotfix/<id>-<desc>` | Critical production fix (expedited review) | `hotfix/SEC-001-hash-collision` |
| `release/<version>` | Release stabilisation (freeze for final testing) | `release/2.9.0` |
| `docs/<desc>` | Documentation-only changes (no code changes) | `docs/update-traceability` |
| `ci/<desc>` | CI/CD and infrastructure changes | `ci/add-coverage-stage` |

### 2.3 Naming Rules

Branch names **must** match this regex (enforced by the `pre-push` hook):

```
^(main|feature/[A-Za-z0-9._-]+|fix/[A-Za-z0-9._-]+|hotfix/[A-Za-z0-9._-]+|release/[0-9]+\.[0-9]+\.[0-9]+|docs/[A-Za-z0-9._-]+|ci/[A-Za-z0-9._-]+)$
```

Examples of **valid** names:
```
feature/SWR-VIT-008-trend-export
fix/BUG-101-null-patient-name
hotfix/SEC-002-password-timing
release/2.9.0
docs/update-scm-plan
ci/add-sarif-upload
```

Examples of **invalid** names (blocked by hook):
```
my-branch           # no prefix
Feature/something   # uppercase prefix
feature/            # missing description
feature/has spaces  # spaces not allowed
```

## 3. Workflow

### 3.1 Feature Development

```
main ──────────────────────────────────────────────── main
  │                                                     ▲
  ├─▸ feature/SWR-GUI-012-localization ─────── PR ──────┘
  │       │         │          │
  │    commit 1  commit 2   commit 3
  │    (code)    (tests)    (docs)
```

**Steps:**

1. **Create branch** from latest `main`:
   ```bash
   git checkout main && git pull
   git checkout -b feature/SWR-GUI-012-localization
   ```

2. **Develop** — commit frequently with descriptive messages:
   ```bash
   git commit -m "feat(vitals): add localization string table

   Implements SWR-GUI-012. Static string arrays for EN/ES/FR/DE.
   Zero heap allocation — IEC 62304 Class B compliant."
   ```

3. **Push** and open Pull Request:
   ```bash
   git push -u origin feature/SWR-GUI-012-localization
   ```

4. **Pipeline runs** — all 6 stages must pass:
   - Build → Static Analysis + SAST → Tests → Coverage → DVT → (Release skipped)

5. **Reviewer approves** — verifying the change checklist is complete

6. **Squash-merge** into `main` — creates a single, clean commit

7. **Delete branch** — automatically or manually after merge

### 3.2 Bug Fix

Same as feature development, but with a `fix/` prefix and an associated defect ID:

```bash
git checkout -b fix/BUG-042-spo2-boundary
```

### 3.3 Hotfix (Critical Production Fix)

For urgent fixes that cannot wait for a normal development cycle:

```
main (v2.8.2) ──────────────────────────────── main
  │                                               ▲
  ├─▸ hotfix/SEC-001-hash-collision ─── PR ───────┘
  │       │                                        │
  │    commit 1                               tag v2.8.3
  │    (fix + test)
```

**Steps:**

1. Create `hotfix/` branch from `main`
2. Apply minimal fix + add regression test
3. Open PR — reviewer prioritises review
4. Pipeline must still pass all stages (no shortcuts)
5. Merge, tag, and release immediately

### 3.4 Release Stabilisation

For major/minor releases that need a stabilisation period:

```
main ─────────────────────────────────────── main
  │                                            ▲
  ├─▸ release/3.0.0 ─────────── PR ───────────┘
  │       │         │                           │
  │    fix 1     fix 2                     tag v3.0.0
```

**Steps:**

1. Create `release/<version>` from `main`
2. Only bug fixes allowed on this branch (no new features)
3. When stable, merge back to `main` and tag
4. If `main` has advanced, rebase or merge `main` into release first

## 4. Commit Message Convention

### 4.1 Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 4.2 Types

| Type       | Description                              | Example                        |
|------------|------------------------------------------|--------------------------------|
| `feat`     | New feature / requirement implementation  | `feat(alerts): add NEWS2 score`|
| `fix`      | Bug fix                                  | `fix(vitals): clamp HR to 300` |
| `docs`     | Documentation only                       | `docs: update TRACEABILITY.md` |
| `test`     | Add or update tests                      | `test(patient): add boundary`  |
| `ci`       | CI/CD pipeline changes                   | `ci: add coverage stage`       |
| `refactor` | Code restructuring (no behaviour change) | `refactor(alerts): extract fn` |
| `chore`    | Build system, dependencies               | `chore: bump GTest to 1.14`    |

### 4.3 Rules (enforced by `commit-msg` hook)

1. Subject line ≤ 72 characters
2. Subject line must start with a valid type prefix
3. Body lines ≤ 100 characters
4. Requirement references (`SWR-xxx`, `BUG-xxx`) are encouraged in the body

### 4.4 Examples

**Good:**
```
feat(localization): add 4-language string table

Implements SWR-GUI-012. Static arrays for EN, ES, FR, DE with 50+
strings each. Zero heap allocation per IEC 62304 Class B.

Tested: test_localization.cpp (20 new tests)
```

**Bad:**
```
updated stuff          # no type prefix, vague
FEAT: add feature      # type must be lowercase
feat(localization): add multi-language support for the patient vital signs monitor GUI  # > 72 chars
```

## 5. Merge Strategy

| Merge Type       | When Used                              | Git Command                       |
|------------------|----------------------------------------|-----------------------------------|
| **Squash merge** | Feature, fix, docs, ci branches        | GitHub PR "Squash and merge"      |
| **Merge commit** | Release branches (preserve history)     | GitHub PR "Create a merge commit" |
| **Rebase**       | Never on `main`; only on local branches| `git rebase main` (local only)    |

**Why squash merge by default:**
- Each feature = one clean commit on `main`
- Clean `git log` for audit trail
- Easy to revert a feature (single commit)

## 6. Tag and Release Process

### 6.1 When to Tag

| Change Type                 | Version Bump | Example         |
|-----------------------------|--------------|-----------------|
| New clinical feature        | MINOR        | v2.8.0 → v2.9.0 |
| Bug fix / documentation     | PATCH        | v2.8.0 → v2.8.1 |
| Breaking change             | MAJOR        | v2.8.0 → v3.0.0 |

### 6.2 How to Tag

```bash
# Ensure main is up to date and pipeline is green
git checkout main && git pull

# Update version in CMakeLists.txt
# project(PatientMonitor VERSION 2.9.0 LANGUAGES C CXX)

# Commit version bump
git commit -am "chore: bump version to 2.9.0"

# Tag and push
git tag v2.9.0
git push origin main v2.9.0
```

### 6.3 Automated Release

When a `v*` tag is pushed, the pipeline stage 6 automatically:

1. Builds the release GUI executable
2. Builds the Inno Setup installer
3. Creates a portable ZIP
4. Creates/updates the GitHub Release with all artefacts

## 7. Enforcement Summary

| Rule                                | Enforcement Mechanism         | Location                        |
|-------------------------------------|-------------------------------|----------------------------------|
| Branch name format                  | `pre-push` Git hook           | `scripts/hooks/pre-push`        |
| No direct push to `main`            | `pre-push` Git hook           | `scripts/hooks/pre-push`        |
| Commit message format               | `commit-msg` Git hook         | `scripts/hooks/commit-msg`      |
| Basic lint on staged files           | `pre-commit` Git hook         | `scripts/hooks/pre-commit`      |
| All 6 pipeline stages pass          | GitHub Actions `needs`        | `.github/workflows/pipeline.yml`|
| Change checklist completed           | PR template                   | `.github/PULL_REQUEST_TEMPLATE.md`|
| Branch protection (PR + approvals)   | GitHub branch protection      | Configured via API/UI           |

**Install hooks:**
```bash
./scripts/install-hooks.sh
```

## 8. Diagram — Full Flow

```
Developer                   GitHub                        CI/CD Pipeline
─────────                   ──────                        ──────────────
  │                           │                               │
  ├── git checkout -b         │                               │
  │   feature/SWR-xxx-desc    │                               │
  │                           │                               │
  ├── [develop + commit] ─────│                               │
  │   (commit-msg hook        │                               │
  │    validates format)      │                               │
  │                           │                               │
  ├── git push ───────────────┤                               │
  │   (pre-push hook          │                               │
  │    validates branch name, │                               │
  │    blocks main push)      │                               │
  │                           │                               │
  ├── Open PR ────────────────┤                               │
  │                           ├── PR template checklist       │
  │                           ├── Trigger pipeline ──────────▸│
  │                           │                               ├── 1. Build
  │                           │                               ├── 2. Analysis
  │                           │                               ├── 3. Tests
  │                           │                               ├── 4. Coverage
  │                           │                               ├── 5. DVT
  │                           │                               └── (6. Release — tags only)
  │                           │                               │
  │                           ├◂── Status checks ────────────┤
  │                           │                               │
  ├── Review feedback ◂───────┤                               │
  ├── Fix + push ─────────────┤                               │
  │                           ├── Re-run pipeline ───────────▸│
  │                           │                               │
  ├── Approved ◂──────────────┤                               │
  │                           ├── Squash merge to main        │
  │                           │                               │
  ├── git tag v2.9.0 ────────┤                               │
  │                           ├── Pipeline stage 6 ──────────▸│
  │                           │                               └── Release artefacts
  │                           ├◂── GitHub Release created ────┤
  ▼                           ▼                               ▼
```

## 9. Document History

| Revision | Date       | Author         | Changes                          |
|----------|------------|----------------|----------------------------------|
| REV-A    | 2026-04-08 | SCM Admin      | Initial release                  |
