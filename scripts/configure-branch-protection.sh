#!/usr/bin/env bash
# =============================================================================
# configure-branch-protection.sh — Configure GitHub branch protection for main
#
# Usage:  GITHUB_TOKEN=<token> ./scripts/configure-branch-protection.sh
#
# Requires: curl, a GitHub PAT with repo permissions
#
# Sets the following rules on the 'main' branch:
#   - Require pull request before merging (1 approval)
#   - Require status checks to pass (Pipeline stages)
#   - Require branches to be up to date before merging
#   - Require conversation resolution before merging
#   - Restrict direct pushes
# =============================================================================

set -euo pipefail

OWNER="vinu-engineer"
REPO="medicalUT_IT"
BRANCH="main"

if [ -z "${GITHUB_TOKEN:-}" ]; then
    echo "ERROR: GITHUB_TOKEN environment variable is required."
    echo ""
    echo "Usage:"
    echo "  GITHUB_TOKEN=ghp_xxxx ./scripts/configure-branch-protection.sh"
    exit 1
fi

echo "Configuring branch protection for $OWNER/$REPO:$BRANCH ..."

curl -s -X PUT \
    -H "Authorization: token $GITHUB_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    "https://api.github.com/repos/$OWNER/$REPO/branches/$BRANCH/protection" \
    -d '{
        "required_status_checks": {
            "strict": true,
            "contexts": [
                "1 — Build All",
                "2a — Static Analysis (cppcheck)",
                "2b — SAST (CodeQL)",
                "3 — Unit & Integration Tests",
                "4 — Code Coverage",
                "5 — DVT"
            ]
        },
        "enforce_admins": true,
        "required_pull_request_reviews": {
            "required_approving_review_count": 1,
            "dismiss_stale_reviews": true
        },
        "restrictions": null,
        "required_conversation_resolution": true
    }' | python3 -c "
import json, sys
try:
    data = json.load(sys.stdin)
    if 'url' in data:
        print('Branch protection configured successfully.')
        print(f'  URL: {data[\"url\"]}')
    elif 'message' in data:
        print(f'ERROR: {data[\"message\"]}')
        if 'documentation_url' in data:
            print(f'  See: {data[\"documentation_url\"]}')
    else:
        print(json.dumps(data, indent=2))
except:
    print('Response received (check GitHub settings to verify).')
"

echo ""
echo "Verify at: https://github.com/$OWNER/$REPO/settings/branches"
