# Risk Note: Issue 33

Issue: `#33`
Branch: `feature/33-release-docs-align-installer-artifact-names-with-current-2-7-0-outputs`

## proposed change

Align current-facing installer documentation and helper-script output text with
the installer naming already configured for version `2.7.0`. The intended scope
is limited to stale artifact references in files such as `README.md`,
`create_installer.bat`, and installer-script header comments, so that published
instructions and echoed filenames match the current installer configuration in
`installer.iss`.

## medical-safety impact

No direct runtime, clinical-threshold, alerting, NEWS2, authentication, or
patient-record behavior changes are proposed. The medical-safety impact is
indirect and low: stale installer names can confuse operators, reviewers, or
validators about which binary belongs to the approved `2.7.0` release evidence
set. That confusion can delay review or cause humans to inspect the wrong
artifact, but it does not itself change bedside monitoring behavior as long as
the implementation stays documentation-only and does not alter installer
configuration semantics.

## security and privacy impact

No privacy-impacting data flow changes are expected. No patient data,
credentials, or access-control behavior is in scope. The main security concern
is integrity and provenance: mismatched artifact names can weaken confidence
that the reviewed installer, echoed output path, and published instructions all
refer to the same signed release payload. Privacy impact is none expected.

## threat-model note

- Attack surface: none added. This issue changes release documentation, helper
  echo text, and nonfunctional installer comments only; it does not introduce a
  new executable, network path, installer privilege boundary, dependency,
  updater, or remote service interaction.
- Data flow: no product or patient-data flow changes. The only affected flow is
  release-evidence text derived from the authoritative installer metadata in
  `installer.iss` (`AppVersion`, `OutputBaseFilename`) and the release asset
  publication patterns in `.github/workflows/release.yml`.
- Trust boundaries: the relevant boundary is between repository-controlled
  release authorities (`installer.iss` and `.github/workflows/release.yml`) and
  humans consuming README/help text during packaging or release verification.
  This change reduces ambiguity at that boundary by making secondary documents
  echo the same artifact names as the authoritative sources.
- Vulnerability monitoring: existing repository controls remain unchanged. The
  branch adds no third-party software, binary generation step, or deployment
  endpoint beyond the already reviewed installer and release workflow; existing
  CI, static analysis, CodeQL, and release-review practices remain the
  monitoring path for future release-integrity defects.
- Coordinated disclosure and patch expectations: if a future naming mismatch
  could misdirect artifact verification, distribution, or operator download
  guidance, treat it as a release-integrity defect and correct it through the
  normal repository patch and release process. No separate emergency disclosure
  channel is required for this documentation-only correction because no new
  exploit path or exposed secret is introduced.
- Privacy impact: none. The change does not create, collect, transmit, log, or
  expose patient, operator, credential, or telemetry data.
- Provenance linkage: installer artifact provenance remains anchored to
  `installer.iss` as the naming authority and `.github/workflows/release.yml`
  as the asset publication authority. The patch only aligns dependent release
  guidance to those authorities and does not change installer semantics.

## affected requirements or "none"

none. The issue does not propose a change to approved UNS, SYS, or SWR
behavior. It is a release-documentation and build-helper consistency correction
for an already approved installer version.

## hazards and failure modes

- An operator or reviewer follows stale README text and looks for an outdated
  installer artifact name, delaying release verification or using the wrong
  binary as evidence.
- `create_installer.bat` echoes a filename that does not match the actual
  installer naming convention, causing confusion during packaging, handoff, or
  manual validation.
- An implementer expands a documentation-only task into a functional change to
  `AppVersion`, `OutputBaseFilename`, installer packaging behavior, or release
  process semantics.
- Historical records are rewritten unnecessarily in an attempt to force zero
  stale-version matches across audit history rather than fixing the current
  deliverables only.

## existing controls

- The issue body explicitly states that there is no product-behavior change and
  limits scope to release-documentation and build-helper consistency.
- `installer.iss` already defines the canonical installer version and basename
  via `AppVersion = 2.7.0` and `OutputBaseFilename=PatientMonitorSetup-{#AppVersion}`.
- The repository requirements and architecture separate clinical logic from
  release-documentation text, reducing the chance that a documentation fix must
  touch safety-critical code.
- The issue includes targeted `rg` validation commands that identify the known
  stale artifact references.

## required design controls

- Treat `installer.iss` version macros and output basename as the source of
  truth for installer naming; align documentation to that authority rather than
  inventing a new naming convention.
- Restrict implementation to current-facing documentation, echoed helper-script
  text, and nonfunctional installer comments; do not modify runtime code,
  clinical thresholds, requirements, tests, or installer behavior.
- Review the changed file list and diff to confirm the patch stays within the
  issue's declared documentation-only scope.
- If implementation discovers that the intended artifact naming should differ
  from `installer.iss`, stop and escalate instead of silently changing release
  semantics under a low-risk documentation issue.
- Leave historical `docs/history/**` audit records untouched unless one of
  those records is itself the broken deliverable.

## validation expectations

- Run the issue's targeted text searches and confirm the stale `2.6.0`,
  `1.5.0`, and `2.0.0` installer-name references are removed from the scoped
  current-facing files.
- Confirm the threat-model note explicitly records no new executable attack
  surface, no new data flow, the installer/release trust boundary, ongoing
  monitoring expectations, coordinated patch expectations, and no privacy
  impact for this installer-adjacent documentation change.
- Inspect the changed file list and diff to verify documentation-only scope and
  confirm that `installer.iss` functional settings such as `AppVersion` and
  `OutputBaseFilename` were not changed unexpectedly.
- If the helper script output text is edited, confirm the echoed `dist\...exe`
  path matches the canonical installer name derived from `installer.iss`.
- No product regression or clinical verification rerun is required for this
  risk conclusion because executable behavior is unchanged.

## residual risk for this pilot

Low. Residual risk is limited to human documentation transcription error or a
missed stale artifact reference, not to patient-facing behavior. With the
controls above, this issue is appropriate to move forward as a low-risk
release-documentation consistency item.
