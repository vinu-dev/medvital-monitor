# Agentry Full Medical Pilot

This repository is the current Agentry full medical-software pilot target. The
goal is to exercise a regulated-style pipeline with Codex-only roles while
keeping the work small, auditable, and reversible.

## Active Flow

```text
researcher
  -> needs-risk
  -> risk_analyst
  -> ready-for-design
  -> architect
  -> ready-for-implementation
  -> implementer
  -> ready-for-test
  -> tester
  -> ready-for-code-review
  -> code_reviewer
  -> ready-for-quality-review
  -> quality_reviewer
  -> ready-for-cyber-review
  -> cybersecurity_reviewer
  -> ready-for-regulatory-review
  -> regulatory_reviewer
  -> ready-for-ai-safety-review
  -> ai_safety_reviewer
  -> ready-for-traceability
  -> traceability_tracker
  -> ready-for-merge
  -> merger
  -> release smoke check
```

## Model Policy

All configured roles use `npx.cmd @openai/codex exec` with `gpt-5.4` for the
full-medical pilot. Later characterization should downshift individual roles
only after the full path is proven.

## Safety Rules

- Researcher maintains a two-item pre-design supply for Architect by counting
  open `ready-for-design` plus `needs-risk` issues. It creates at most enough
  new `needs-risk` issues to restore that supply to two, with a hard cap of two
  new issues per run.
- Risk Analyst writes a lightweight risk note under `docs/history/risk/`.
- Clinical thresholds, diagnosis logic, treatment advice, and real patient-care
  workflow changes require explicit human acceptance criteria.
- Tester PR bodies must start with `Closes #<issue>` so GitHub closes issues on
  merge.
- Intermediate reviewers do not add `agent-approved`.
- AI Safety Reviewer explicitly classifies whether AI/ML is affected. AI/ML
  changes need data/model provenance, bias and subgroup evidence, transparency,
  monitoring, and PCCP/change-control rationale before traceability review.
- Traceability Tracker is the final approval gate and adds `agent-approved` plus
  `ready-for-merge`.
- Merger merges only green, mergeable PRs with `ready-for-merge` and
  `agent-approved`, then verifies the linked issue is closed.
- Release runs in smoke-test mode unless a human-approved issue is labeled
  `release-approved`.

## Evidence To Capture

- `agentry/state/workpackets/*.md`
- `agentry/state/sessions/*.json`
- per-role log tails from `agentry/logs/<role>/`
- GitHub label transitions and PR comments
- local and GitHub validation results
- token use per role
- AI/ML not-applicable rationale or AI safety evidence bundle when applicable
