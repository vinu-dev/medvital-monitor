# Design Spec: Keep generated DVT outputs out of role worktrees

Issue: #68
Branch: `codex/dvt-results-ignore`
Spec path: `docs/history/specs/68-keep-generated-dvt-outputs-out-of-role-worktrees.md`

## Problem

Issue #68 identifies a repository hygiene defect around DVT output handling.
The repo's DVT and pipeline helpers intentionally write runtime XML, JSON, and
text reports under `dvt/results`, but the directory was not ignored and
historical generated files were still tracked in Git. As a result, isolated
Agentry role worktrees could become dirty after validation runs even when the
implementation itself made no unexpected source change.

This is primarily an engineering-process and evidence-retention problem, not a
product-runtime problem. Role worktrees should stay clean after generating DVT
evidence so that implementer and tester runs can complete without false dirty
state from timestamped artifacts.

## Goal

Make `dvt/results` behave as a runtime artifact directory rather than a
source-controlled evidence directory, while preserving the directory path that
existing local scripts and GitHub Actions already expect.

The intended outcome is:

- generated DVT reports and GTest XML outputs under `dvt/results` do not dirty
  a worktree after validation
- the `dvt/results` directory still exists in the repo via `.gitkeep`
- current DVT scripts and workflow paths keep working without code changes
- stale tracked generated result files are removed from version control

## Non-goals

- No change to clinical thresholds, NEWS2 scoring, alarm generation,
  authentication, localization, or patient-data handling.
- No change to DVT verdict logic, requirement mappings, or test executable
  behavior.
- No change to the chosen runtime output location for existing scripts and
  workflows beyond making Git ignore generated contents there.
- No change to release artifacts, production binaries, installer behavior, or
  requirements baselines.
- No rewrite of historical issue records outside the specific stale generated
  artifacts currently tracked under `dvt/results`.

## Current behavior

Current DVT helpers already treat `dvt/results` as an output directory:

- `dvt/run_dvt.py` defaults `--output-dir` to `<project-root>/dvt/results`
- `dvt/automation/run_dvt.py` generates reports into `dvt/results`
- `.github/workflows/dvt.yml` creates `dvt/results`, writes XML and text
  outputs there, and uploads that directory as an artifact
- `.github/workflows/pipeline.yml` writes DVT JSON and report outputs under
  `dvt/results` and publishes that directory as an artifact
- `scripts/run_pipeline.sh` also copies transient result files into
  `dvt/results`

The mismatch is that Git still tracked old generated files in that directory,
and the repo-level `.gitignore` did not exclude new runtime outputs from future
role runs.

## Proposed change

Implement the issue as a scoped repository-hygiene fix with these decisions:

1. Treat `dvt/results` as a runtime output directory whose generated contents
   are intentionally excluded from source control.
2. Update `.gitignore` to ignore generated files under `dvt/results/*` while
   preserving `dvt/results/.gitkeep` so the directory path stays available in a
   fresh clone or worktree.
3. Remove the stale tracked generated XML and text result artifacts already
   committed under `dvt/results`.
4. Do not move or rename the output directory, because current scripts and CI
   already reference `dvt/results` explicitly and are functioning.
5. Keep the patch limited to ignore rules, the placeholder directory entry, and
   removal of tracked generated artifacts. If implementation discovers a need
   to change DVT output paths or artifact publication semantics, stop and
   escalate rather than broadening scope silently.

## Files expected to change

Expected files:

- `.gitignore`
- `dvt/results/.gitkeep`
- tracked generated files currently under `dvt/results/`

Expected files to inspect but not modify:

- `dvt/run_dvt.py`
- `dvt/automation/run_dvt.py`
- `.github/workflows/dvt.yml`
- `.github/workflows/pipeline.yml`
- `scripts/run_pipeline.sh`

Expected new design-control files for this issue:

- `docs/history/specs/68-keep-generated-dvt-outputs-out-of-role-worktrees.md`
- `docs/history/risk/68-keep-generated-dvt-outputs-out-of-role-worktrees.md`

Files that should not change:

- `src/**`
- `include/**`
- `tests/**`
- `requirements/**`
- any generated Doxygen HTML/XML

## Requirements and traceability impact

No approved UNS, SYS, or SWR behavior changes are expected. This issue changes
repository hygiene for generated validation artifacts only.

Traceability impact is none for functional requirements:

- no `@req` tags change
- no test IDs change
- no requirement statements change
- no traceability matrix rows change

The only evidence impact is operational: validation artifacts continue to be
produced under `dvt/results`, but they are treated as ephemeral outputs and CI
artifacts instead of tracked source files.

## Medical-safety, security, and privacy impact

Medical-safety impact is low and indirect. No patient-facing behavior, bedside
monitoring logic, alert thresholds, NEWS2 scoring, or clinical workflow changes
are proposed. The safety benefit is process clarity: validation runs should not
leave misleading source changes that obscure whether a worktree is dirty due to
code or only due to regenerated evidence files.

Security impact is low and limited to repository integrity and audit hygiene.
Ignoring runtime-generated DVT outputs reduces the chance that stale generated
files are mistaken for maintained source assets. Existing CI artifact upload and
retention remain the evidence path.

Privacy impact is none expected. The affected files are test and DVT outputs,
not patient records, credentials, or exported operator data.

## AI/ML impact assessment

This issue does not add, change, remove, or depend on an AI-enabled device
software function.

- Model purpose: none
- Input data: none beyond existing non-AI test output files
- Output: none
- Human-in-the-loop limits: unchanged
- Transparency needs: unchanged
- Dataset and bias considerations: not applicable
- Monitoring expectations: unchanged
- PCCP impact: none

## Validation plan

Validation should prove that the repository now ignores generated DVT outputs
without breaking the expected output directory contract.

Primary ignore-rule check:

```powershell
git check-ignore -v dvt/results/dvt_report_20990101_000000.txt dvt/results/test_unit_20990101_000000.xml dvt/results/dvt_results_2099-01-01_000000.json dvt/results/unit_results.xml
```

Directory-contract check:

```powershell
python dvt\run_dvt.py --help
```

Reference-path cross-check:

```powershell
rg -n "dvt/results" dvt .github/workflows scripts
```

Diff-scope review:

```powershell
git diff --stat origin/main...HEAD
git diff -- .gitignore dvt/results
```

Remote validation:

- GitHub Actions pipeline for the PR should remain green after the change

No functional unit, integration, or requirement-traceability rerun is required
for the design intent of this issue because production behavior is unchanged.
If implementation expands into script logic, output-path changes, or altered
artifact publication behavior, stop and re-evaluate the scope.

## Rollback or failure handling

If implementation discovers that a downstream tool requires specific tracked
files under `dvt/results`, stop and escalate rather than partially ignoring the
directory and leaving an inconsistent rule set.

If CI or local DVT helpers fail because the directory no longer exists, restore
the placeholder approach with `.gitkeep` rather than reintroducing tracked
generated outputs.

If reviewers require a broader evidence-retention policy discussion, keep this
issue scoped to local/CI runtime artifact hygiene and raise a separate issue
for any longer-term archival design change.
