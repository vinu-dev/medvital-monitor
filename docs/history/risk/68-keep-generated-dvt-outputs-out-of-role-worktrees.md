# Risk Note: Issue 68

Issue: `#68`
Branch: `codex/dvt-results-ignore`

## proposed change

Treat `dvt/results` as a runtime artifact directory rather than a
source-controlled results directory by ignoring generated contents, preserving
the directory via `.gitkeep`, and removing the stale tracked generated files
already committed there.

The intended scope is limited to repository hygiene for DVT evidence handling.
Existing scripts and workflows continue to write outputs to the same
`dvt/results` path, and CI artifacts remain the retention path for generated
evidence.

## medical-safety impact

No direct runtime, clinical-threshold, alerting, NEWS2, authentication, or
patient-record behavior changes are proposed. Medical-safety impact is indirect
and low: dirty worktrees caused only by regenerated DVT artifacts can obscure
whether a branch contains genuine source modifications, which is undesirable
for review and validation discipline, but it does not alter bedside behavior as
long as the change remains limited to ignore rules and tracked artifact cleanup.

## security and privacy impact

No new privacy-impacting data flow is expected. No patient data, credentials,
access control, or network behavior changes are in scope.

The main security and integrity concern is repository provenance. Tracked,
stale generated artifacts can be mistaken for maintained evidence inputs or can
mask whether a role worktree is dirty due to real source edits. Ignoring fresh
runtime outputs and keeping CI artifacts as the evidence path reduces that
ambiguity. Privacy impact is none expected.

## threat-model note

- Attack surface: none added. The issue does not introduce a new executable,
  network path, parser, dependency, remote service, or privilege boundary.
- Data flow: unchanged for product and patient data. DVT helpers still emit
  transient XML/JSON/text outputs under `dvt/results`; the change only removes
  those runtime artifacts from source control expectations.
- Trust boundaries: the relevant boundary is between repository-controlled
  source content and transient validation artifacts generated during local or CI
  execution. This change clarifies that boundary by keeping generated files out
  of Git while preserving artifact upload in CI.
- Vulnerability monitoring: existing CI, static analysis, CodeQL, and DVT
  workflow monitoring remain unchanged. There is no new component requiring
  additional monitoring.
- Coordinated disclosure and patch expectations: if future issues show that DVT
  evidence is being silently lost, mislabeled, or written to an unsafe
  location, treat that as a normal repository defect and correct it through the
  standard patch flow. No separate emergency disclosure path is needed for this
  low-risk hygiene correction.
- Privacy impact: none. The change does not create, collect, transmit, log, or
  expose new patient, operator, credential, or telemetry data.
- Provenance linkage: DVT evidence provenance stays anchored to the existing
  local scripts and GitHub Actions workflows that generate and publish
  `dvt/results` artifacts. The patch only changes source-control treatment of
  those outputs.

## affected requirements or "none"

none. The issue does not propose any change to approved UNS, SYS, or SWR
behavior. It is a source-control hygiene correction for generated validation
artifacts.

## hazards and failure modes

- An implementer or tester run leaves timestamped DVT outputs in the worktree,
  causing false dirty-state detection and blocking isolated role automation.
- Reviewers misinterpret tracked generated DVT files as maintained repository
  assets instead of transient execution outputs.
- The ignore rule is applied too broadly and accidentally hides required
  hand-authored files under `dvt/results`.
- The placeholder directory is removed entirely, causing local scripts or CI
  steps that expect `dvt/results` to fail.
- The issue expands beyond hygiene into unreviewed changes to DVT output paths
  or evidence-retention semantics.

## existing controls

- The issue body explicitly scopes the change to ignoring generated
  `dvt/results` files, retaining the output directory, removing stale tracked
  artifacts, and keeping CI green.
- `dvt/run_dvt.py`, `dvt/automation/run_dvt.py`, `.github/workflows/dvt.yml`,
  `.github/workflows/pipeline.yml`, and `scripts/run_pipeline.sh` already
  establish `dvt/results` as the runtime output path.
- CI workflows upload `dvt/results` as artifacts, providing an evidence
  retention path that does not rely on committing generated files to source
  control.
- The proposed implementation keeps `.gitkeep`, which preserves the directory
  contract without keeping generated content tracked.

## required design controls

- Restrict the implementation to `.gitignore`, `dvt/results/.gitkeep`, and the
  removal of stale tracked generated artifacts under `dvt/results`.
- Keep `dvt/results` as the output path unless a separate reviewed issue
  approves changing the script and CI contract.
- Preserve exactly one tracked placeholder file (`.gitkeep`) so fresh clones
  and worktrees still contain the expected directory path.
- Review the changed file list and diff to confirm no production code,
  requirements, tests, or workflow logic changed unexpectedly.
- If implementation discovers a need to retain some generated outputs in Git,
  stop and escalate rather than mixing tracked runtime artifacts with the new
  ignore policy heuristically.

## validation expectations

- Confirm `git check-ignore -v` reports the expected ignore rule for sample
  generated files under `dvt/results`.
- Confirm `python dvt\run_dvt.py --help` still advertises `dvt/results` as the
  default output directory, proving the path contract was not broken.
- Confirm bounded repository searches still show the expected workflow/script
  references to `dvt/results`, with no unreviewed path migration.
- Inspect the changed file list and diff to verify that the patch is limited to
  ignore rules, the placeholder file, and removal of stale generated artifacts.
- Confirm the PR's GitHub Actions pipeline remains green after the repair.

## residual risk for this pilot

Low. Residual risk is limited to either missing a stale generated artifact or
overlooking a downstream consumer that expected tracked files under
`dvt/results`, not to patient-facing behavior. With the controls above, the
issue is appropriate to proceed as a low-risk repository-hygiene correction.
