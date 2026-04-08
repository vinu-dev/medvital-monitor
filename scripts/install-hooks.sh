#!/usr/bin/env bash
# =============================================================================
# install-hooks.sh — Install SCM enforcement Git hooks
#
# Usage:  ./scripts/install-hooks.sh
#
# Installs commit-msg, pre-push, and pre-commit hooks from scripts/hooks/
# into .git/hooks/. Existing hooks are backed up with a .bak suffix.
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_SRC="$SCRIPT_DIR/hooks"
HOOKS_DST="$REPO_ROOT/.git/hooks"

if [ ! -d "$HOOKS_DST" ]; then
    echo "ERROR: Not a Git repository (no .git/hooks found)."
    echo "  Run this script from the repository root."
    exit 1
fi

HOOKS="commit-msg pre-push pre-commit"
INSTALLED=0

for HOOK in $HOOKS; do
    SRC="$HOOKS_SRC/$HOOK"
    DST="$HOOKS_DST/$HOOK"

    if [ ! -f "$SRC" ]; then
        echo "SKIP: $HOOK (source not found at $SRC)"
        continue
    fi

    # Back up existing hook
    if [ -f "$DST" ] && [ ! -L "$DST" ]; then
        cp "$DST" "${DST}.bak"
        echo "  Backed up existing $HOOK → ${HOOK}.bak"
    fi

    # Install via symlink (so updates to scripts/hooks/ take effect immediately)
    ln -sf "$SRC" "$DST"
    chmod +x "$SRC"
    echo "  Installed: $HOOK"
    INSTALLED=$((INSTALLED + 1))
done

echo ""
echo "Done. $INSTALLED hook(s) installed."
echo ""
echo "Hooks enforce:"
echo "  commit-msg  — Conventional commit format (type(scope): subject)"
echo "  pre-push    — No direct push to main; branch name validation"
echo "  pre-commit  — No secrets, no large files, no credential files"
echo ""
echo "To bypass (emergency only):  git commit --no-verify"
