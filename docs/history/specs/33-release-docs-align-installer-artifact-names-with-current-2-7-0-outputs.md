# Design Spec: Release docs align installer artifact names with current 2.7.0 outputs

Issue: #33
Branch: `feature/33-release-docs-align-installer-artifact-names-with-current-2-7-0-outputs`
Spec path: `docs/history/specs/33-release-docs-align-installer-artifact-names-with-current-2-7-0-outputs.md`

## Problem

Issue #33 identifies drift between the current installer naming authorities and
the current-facing release documentation and helper-script text. The repo's
installer script already defines `AppVersion = 2.7.0` and emits
`PatientMonitorSetup-<version>.exe`, while the README and helper output text
still reference stale or contradictory names such as
`PatientMonitor-v2.6.0-portable.exe`, `PatientMonitorSetup-1.5.0.exe`, and
`PatientMonitorSetup-2.0.0.exe`.

This is a release-evidence and provenance defect, not a runtime defect. Humans
using the README, release records, or installer helper output can be pointed to
the wrong artifact name, or can confuse the standalone executable, portable
archive, and Windows installer as if they were the same deliverable.

## Goal

Produce a narrow documentation-and-helper-text change set that makes the
current-facing installer references match the current 2.7.0 release outputs and
their naming authorities.

The intended outcome is that a reviewer can verify, without guessing, which
release asset is:

- the standalone GUI executable
- the portable ZIP
- the Windows installer built from `installer.iss`

## Non-goals

- No production code, clinical thresholds, NEWS2 logic, alarm limits,
  authentication, persistence, test logic, or CI gating changes.
- No change to installer behavior, `AppVersion`, `OutputBaseFilename`, signing,
  packaging format, or release-process semantics.
- No rewrite of historical audit records under `docs/history/specs/**` or
  `docs/history/risk/**` solely to eliminate old version strings.
- No broad release-doc refresh beyond the files that currently publish
  contradictory artifact names.

## Current behavior

Current naming authorities already exist:

- `installer.iss` functionally defines `AppVersion = 2.7.0`
- `installer.iss` functionally defines
  `OutputBaseFilename=PatientMonitorSetup-{#AppVersion}`
- `.github/workflows/release.yml` creates and uploads:
  - `PatientMonitor-v<version>.exe`
  - `PatientMonitor-v<version>-portable.zip`
  - the installer found from `PatientMonitorSetup*.exe`

Current-facing drift remains in scoped files:

- `README.md` tells end users to download
  `PatientMonitor-v2.6.0-portable.exe` and then run a setup wizard, which mixes
  a stale version with an asset type that does not match the current release
  outputs.
- `README.md` also says `create_installer.bat` produces
  `dist\PatientMonitorSetup-1.5.0.exe`.
- `create_installer.bat` echoes `dist\PatientMonitorSetup-1.5.0.exe` in both
  the startup banner and success footer.
- `installer.iss` header comments still describe version `2.0.0` and output
  `dist\PatientMonitorSetup-2.0.0.exe`, even though the functional settings are
  already at `2.7.0`.
- `docs/DEVOPS_WORKFLOWS.md` documents the installer asset as
  `PatientMonitorSetup-v<x.y.z>.exe`, which conflicts with the actual
  `installer.iss` basename pattern and should be treated as an in-scope
  current-facing inconsistency if it remains unchanged at implementation time.

## Proposed change

Implement the issue as release-documentation reconciliation only, with the
following decisions:

1. Treat the functional `installer.iss` settings and the asset creation logic
   in `.github/workflows/release.yml` as the naming authorities.
2. Update `README.md` so the end-user installation guidance points to a real
   current release asset and does not describe a nonexistent `portable.exe`.
   The README should distinguish installer flow from portable/standalone assets
   instead of collapsing them into one filename.
3. Update the developer build example in `README.md` so the documented
   `create_installer.bat` output matches the installer name implied by
   `OutputBaseFilename` and `AppVersion`.
4. Update the echoed banner/footer text in `create_installer.bat` so the script
   reports the same installer filename that `installer.iss` is configured to
   generate.
5. Update only nonfunctional header comments in `installer.iss` to remove the
   stale `2.0.0` examples. Do not change the functional setup fields unless
   implementation discovers a genuine authority mismatch that requires
   escalation.
6. Inspect other current-facing release documentation surfaced by the bounded
   search and fix directly conflicting installer examples when they derive from
   the same stale naming problem, with `docs/DEVOPS_WORKFLOWS.md` the likely
   additional candidate.
7. Keep the final diff constrained to documentation, helper echo text, and
   nonfunctional comments. If implementation appears to require release-process
   or packaging behavior changes, stop and escalate rather than broadening the
   issue silently.

## Files expected to change

Expected files:

- `README.md`
- `create_installer.bat`
- `installer.iss`

Expected files to inspect and modify only if they still publish the same
contradictory installer naming:

- `docs/DEVOPS_WORKFLOWS.md`

Expected files to inspect but likely not modify:

- `.github/workflows/release.yml`
- `docs/history/risk/33-release-docs-align-installer-artifact-names-with-current-2-7-0-outputs.md`

Files that should not change:

- `src/**`
- `include/**`
- `tests/**`
- `requirements/**`
- `.github/workflows/ci.yml`
- `.github/workflows/pipeline.yml`
- `docs/history/specs/**` except this new issue-33 spec
- `docs/history/risk/**` except the pre-existing issue-33 risk note already on branch

## Requirements and traceability impact

No approved UNS, SYS, or SWR behavior changes are expected. This issue is a
release-documentation and helper-text correction only.

Traceability impact is indirect but relevant to release evidence:

- current-facing instructions should identify the correct installer artifact
- helper output should match the artifact a reviewer will actually find in
  `dist/`
- release documentation should not force reviewers to reconcile contradictory
  installer names by hand

No `@req` tags, test IDs, traceability rows, or requirement statements should
change.

## Medical-safety, security, and privacy impact

Medical-safety impact is low and indirect. No bedside behavior, classification
logic, NEWS2 scoring, alert generation, persistence, or authentication behavior
changes. The safety value is evidence clarity: operators and reviewers should
not be directed toward the wrong release artifact when validating a build.

Security impact is limited to integrity and provenance. Consistent artifact
naming reduces the chance of humans reviewing or distributing the wrong file.
No credential, RBAC, cryptographic, or data-ingress behavior changes.

Privacy impact is none expected. No patient data, logs, exports, or storage
paths change.

Because the touched files are release-sensitive, the implementation must call
out explicitly that it did not change installer behavior, release workflow
logic, or artifact semantics.

## Validation plan

Use bounded text validation against current-facing files and the canonical
authorities.

Primary stale-name cleanup check:

```powershell
rg -n "PatientMonitor-v2\.6\.0-portable\.exe|PatientMonitorSetup-1\.5\.0\.exe|Version\s*: 2\.0\.0|Output\s*: dist\\PatientMonitorSetup-2\.0\.0\.exe" README.md create_installer.bat installer.iss docs/DEVOPS_WORKFLOWS.md
```

Authority cross-check:

```powershell
rg -n "AppVersion|OutputBaseFilename" installer.iss
rg -n "PatientMonitor-\\$tag\\.exe|portable\\.zip|PatientMonitorSetup\\*" .github/workflows/release.yml
```

Release-doc consistency check:

```powershell
rg -n "PatientMonitorSetup-v<|PatientMonitorSetup-2\.7\.0|PatientMonitor-v2\.7\.0\.exe|PatientMonitor-v2\.7\.0-portable\.zip" README.md docs/DEVOPS_WORKFLOWS.md .github/workflows/release.yml installer.iss
```

Diff-scope review:

```powershell
git diff --stat
git diff -- README.md create_installer.bat installer.iss docs/DEVOPS_WORKFLOWS.md
```

No product regression, clinical verification, or requirement-traceability rerun
is required for this issue because executable behavior is unchanged. If
implementation discovers a mismatch between the README intent and the actual
release assets that cannot be resolved as a documentation-only clarification,
stop and escalate.

## Rollback or failure handling

If implementation discovers that the intended installer naming authority is
unclear between `installer.iss` and `release.yml`, stop and escalate rather
than choosing a filename convention heuristically.

If the diff expands beyond documentation, helper echo text, and nonfunctional
comments, revert the extra scope before handoff.

If a current-facing document contains a broader release naming inconsistency
than issue #33 anticipated, prefer adding that bounded file only when the
conflict is direct and evidence-based; otherwise document the follow-up gap in
the implementation handoff rather than turning this into a release-process
rewrite.
