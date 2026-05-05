# `agentry/` — local Agentry installation for this repo

This folder is the gtest-style "fetched dependency" for [Agentry](https://github.com/vinu-dev/agentry). It's how the orchestrator runs against THIS repo. Each target repo gets its own copy.

## What's in here

| File | Purpose | Committed? |
|------|---------|-----------|
| `config.yml` | Picks which LLM CLI handles each role (Claude / Codex / etc.) and the timeouts | yes |
| `start.ps1` / `start.sh` | The entry point. Run this every time you want Agentry running. Creates `.venv/` and installs Agentry on first run. | yes |
| `.env.example` | Template for your secrets (GITHUB_TOKEN etc.) | yes |
| `.gitignore` | Ignores the next four entries | yes |
| `.env` | Your actual secrets — copied from `.env.example` and filled in by you | **no, gitignored** |
| `.venv/` | Python venv with Agentry pip-installed; auto-created on first run | **no, gitignored** |
| `logs/` | Per-role agent stdout, one timestamped file per run | **no, gitignored** |
| `state/` | Runtime sessions and work packets | **no, gitignored** |
| `worktrees/` | Per-role git worktrees when enabled | **no, gitignored** |

## Where role rule files live

NOT here. They live at the **standard target-repo location**:

```
<this-repo>/docs/ai/roles/
├── researcher.md
├── architect.md
├── implementer.md
├── tester.md
├── reviewer.md
└── release.md
```

Edit those for project-specific instructions per role. The prompts in `agentry/config.yml` already point at them.

## How to use

### One time per machine
Install Python, Node.js, Claude Code, Codex CLI:

```powershell
# Windows
iwr -useb https://raw.githubusercontent.com/vinu-dev/agentry/main/scripts/install-deps.ps1 | iex
```

```bash
# Linux
curl -fsSL https://raw.githubusercontent.com/vinu-dev/agentry/main/scripts/install-deps.sh | bash
```

Then authenticate the LLM CLIs (browser flow):

```
claude login
codex login
```

### One time per repo (this folder)
1. Copy `.env.example` to `.env` and fill in `GITHUB_TOKEN`
2. Edit `config.yml` — pick which CLI handles each role
3. Edit `../docs/ai/roles/*.md` if you want project-specific role instructions

### Every time you want Agentry running

```powershell
# Windows
.\agentry\start.ps1
```

```bash
# Linux
./agentry/start.sh
```

Foreground. Ctrl-C to stop. Close the terminal to stop. Reboot kills it. **No service.** Run the script again when you want it running again.

## Token controls in this repo

This repo pins Agentry to the version declared in `start.ps1` and `start.sh`.
Current releases add token-burn controls
used here:

- `context.work_packets: true` writes a bounded
  `agentry/state/workpackets/<role>.md` file before a role starts.
- Each work packet names one `Selected Candidate`; the role must process only
  that item and treat the rest of the queue as read-only awareness.
- Reviewer uses `trigger.pr_check_gate: settled`, so it does not launch while
  all matching `ready-for-review` PR checks are still pending or queued.
- Researcher uses the platform backlog guard and this repo's
  `research.backlog_labels: ["ready-for-design", "needs-risk"]`, so it is not
  launched while the pre-design supply is already at two issues.

Role prompts should read the work packet first, tail logs instead of reading
full historical logs, inspect PR file lists before full diffs, and use targeted
diffs for large changes.

For this experiment, every role is routed through Codex and the default model is
`gpt-5.4-mini`. That keeps the first flow-validation run cheap. After the
mechanics are proven, compare profiles by changing only `-m` in `config.yml`
and recording per-session tokens, duration, pass/fail outcome, and whether a
human had to intervene. The comparison protocol lives in
`docs/ai/agentry-model-characterization.md`.

Wrapper commands such as `status`, `doctor`, `configure`, and `gui` reuse an
existing `.venv/` even if the pinned ref changed. To intentionally refresh the
venv, stop Agentry first and set `AGENTRY_FORCE_INSTALL=1`.

## To upgrade Agentry

Update the pinned ref in `start.ps1` and `start.sh`, stop any running Agentry
process that uses this venv, then rerun with `AGENTRY_FORCE_INSTALL=1`. If the
venv is corrupted, delete `.venv/` and run `start.ps1` / `start.sh` again.

## To remove Agentry from this repo

Just delete this `agentry/` folder. Optionally also delete `docs/ai/roles/` (or keep them — they're useful project documentation regardless of whether Agentry is running).
