#!/usr/bin/env python3
"""
create_btle_issues.py — Creates the six BTLE-audit gap-tracking issues.

Requires:
  GITHUB_TOKEN  — token with issues:write on the target repo
  GITHUB_REPOSITORY — owner/repo string (set automatically in Actions)

Run once; each issue is idempotent (skipped if title already exists).
After creation each issue is added to the MaximumTrainer user project #2.
"""

import json
import os
import pathlib
import sys
import urllib.request
import urllib.error

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

REPO = os.environ["GITHUB_REPOSITORY"]          # e.g. MaximumTrainer/MaximumTrainer_Redux
TOKEN = os.environ["GITHUB_TOKEN"]
API = "https://api.github.com"
PROJECT_URL = "https://github.com/users/MaximumTrainer/projects/2"

ISSUES_DIR = pathlib.Path(__file__).parent / "btle-issues"

ISSUES = [
    {
        "title": "[BTLE WASM] No auto-reconnection logic in BtleHubWasm (Gap 1)",
        "labels": ["BTLE", "Platform-Specific", "Priority-Low"],
        "body_file": "gap1.md",
    },
    {
        "title": "[BTLE WASM] deviceConnected emitted before async GATT connection is ready (Gap 2)",
        "labels": ["BTLE", "Platform-Specific", "Priority-Low"],
        "body_file": "gap2.md",
    },
    {
        "title": "[macOS] Add NSBluetoothAlwaysUsageDescription to Info.plist (Gap 3)",
        "labels": ["BTLE", "Platform-Specific", "Priority-Medium"],
        "body_file": "gap3.md",
    },
    {
        "title": "[Linux] Document Bluetooth runtime permissions in README (Gap 4)",
        "labels": ["BTLE", "documentation", "Priority-Low"],
        "body_file": "gap4.md",
    },
    {
        "title": "[Windows] Document Windows 10+ minimum requirement for BLE (Gap 5)",
        "labels": ["BTLE", "documentation", "Priority-Low"],
        "body_file": "gap5.md",
    },
    {
        "title": "[BTLE WASM] FTMS Request Control opcode 0x00 missing before ERG commands (Gap 6)",
        "labels": ["BTLE", "Platform-Specific", "Priority-Medium"],
        "body_file": "gap6.md",
    },
]

LABELS_TO_CREATE = [
    {"name": "BTLE",              "color": "0075ca", "description": "Bluetooth Low Energy subsystem"},
    {"name": "Platform-Specific", "color": "e4e669", "description": "Affects one or more specific platforms"},
    {"name": "Priority-High",     "color": "b60205", "description": "Must be fixed before next release"},
    {"name": "Priority-Medium",   "color": "fbca04", "description": "Should be fixed in the near term"},
    {"name": "Priority-Low",      "color": "0e8a16", "description": "Nice-to-have; does not block releases"},
    {"name": "documentation",     "color": "0052cc", "description": "Documentation-only change required"},
]

# ---------------------------------------------------------------------------
# HTTP helpers
# ---------------------------------------------------------------------------

def _headers():
    return {
        "Authorization": f"Bearer {TOKEN}",
        "Accept": "application/vnd.github+json",
        "Content-Type": "application/json",
        "X-GitHub-Api-Version": "2022-11-28",
    }


def _get(url):
    req = urllib.request.Request(url, headers=_headers())
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read())


def _post(url, payload):
    data = json.dumps(payload).encode()
    req = urllib.request.Request(url, data=data, headers=_headers(), method="POST")
    try:
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read()), resp.status
    except urllib.error.HTTPError as exc:
        body = exc.read().decode(errors="replace")
        return json.loads(body) if body else {}, exc.code


# ---------------------------------------------------------------------------
# Label helpers
# ---------------------------------------------------------------------------

def get_existing_labels():
    url = f"{API}/repos/{REPO}/labels?per_page=100"
    return {lbl["name"] for lbl in _get(url)}


def ensure_labels(existing):
    for lbl in LABELS_TO_CREATE:
        if lbl["name"] not in existing:
            result, status = _post(f"{API}/repos/{REPO}/labels", lbl)
            if status in (201, 200):
                print(f"  Created label: {lbl['name']}")
            elif status == 422:
                print(f"  Label already exists (race): {lbl['name']}")
            else:
                print(f"  Warning: label create returned {status}: {result}", file=sys.stderr)
        else:
            print(f"  Label already exists: {lbl['name']}")


# ---------------------------------------------------------------------------
# Issue helpers
# ---------------------------------------------------------------------------

def get_existing_issue_titles():
    titles = {}
    page = 1
    while True:
        url = f"{API}/repos/{REPO}/issues?state=all&per_page=100&page={page}"
        items = _get(url)
        if not items:
            break
        for item in items:
            titles[item["title"]] = item["number"]
        page += 1
    return titles


def create_issue(title, labels, body):
    payload = {"title": title, "labels": labels, "body": body}
    result, status = _post(f"{API}/repos/{REPO}/issues", payload)
    if status == 201:
        print(f"  Created issue #{result['number']}: {title}")
        return result["number"]
    else:
        print(f"  Error creating issue ({status}): {result}", file=sys.stderr)
        return None


# ---------------------------------------------------------------------------
# Project helpers (GraphQL)
# ---------------------------------------------------------------------------

def add_issue_to_project(issue_number, project_url):
    """Add the issue to the MaximumTrainer user project via GraphQL."""
    # Resolve project node ID from URL
    # project_url = "https://github.com/users/MaximumTrainer/projects/2"
    parts = project_url.rstrip("/").split("/")
    owner, project_num = parts[-3], int(parts[-1])

    graphql_url = "https://api.github.com/graphql"

    # Step 1: get project node ID
    query = """
    query($owner: String!, $number: Int!) {
      user(login: $owner) {
        projectV2(number: $number) {
          id
        }
      }
    }
    """
    payload = {"query": query, "variables": {"owner": owner, "number": project_num}}
    data = json.dumps(payload).encode()
    req = urllib.request.Request(graphql_url, data=data, headers=_headers(), method="POST")
    try:
        with urllib.request.urlopen(req) as resp:
            result = json.loads(resp.read())
    except urllib.error.HTTPError as exc:
        print(f"    GraphQL project lookup failed: {exc}", file=sys.stderr)
        return

    project_id = (result.get("data", {}).get("user", {}) or {}).get("projectV2", {})
    if not project_id:
        print(f"    Could not resolve project ID for {project_url}", file=sys.stderr)
        return
    project_id = project_id.get("id")

    # Step 2: get issue node ID
    issue_url = f"{API}/repos/{REPO}/issues/{issue_number}"
    issue_data = _get(issue_url)
    issue_node_id = issue_data.get("node_id")
    if not issue_node_id:
        print(f"    Could not resolve node_id for issue #{issue_number}", file=sys.stderr)
        return

    # Step 3: add item to project
    mutation = """
    mutation($projectId: ID!, $contentId: ID!) {
      addProjectV2ItemById(input: {projectId: $projectId, contentId: $contentId}) {
        item { id }
      }
    }
    """
    payload = {
        "query": mutation,
        "variables": {"projectId": project_id, "contentId": issue_node_id},
    }
    data = json.dumps(payload).encode()
    req = urllib.request.Request(graphql_url, data=data, headers=_headers(), method="POST")
    try:
        with urllib.request.urlopen(req) as resp:
            result = json.loads(resp.read())
        if result.get("errors"):
            print(f"    GraphQL errors: {result['errors']}", file=sys.stderr)
        else:
            print(f"    Added issue #{issue_number} to project.")
    except urllib.error.HTTPError as exc:
        print(f"    GraphQL mutation failed: {exc}", file=sys.stderr)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    print("=== Ensuring labels ===")
    existing_labels = get_existing_labels()
    ensure_labels(existing_labels)

    print("\n=== Fetching existing issues ===")
    existing_titles = get_existing_issue_titles()
    print(f"  Found {len(existing_titles)} existing issues.")

    print("\n=== Creating BTLE audit issues ===")
    created_numbers = []
    for spec in ISSUES:
        title = spec["title"]
        if title in existing_titles:
            num = existing_titles[title]
            print(f"  Skipping (already exists) #{num}: {title}")
            created_numbers.append(num)
            continue

        body_path = ISSUES_DIR / spec["body_file"]
        body = body_path.read_text(encoding="utf-8")
        num = create_issue(title, spec["labels"], body)
        if num:
            created_numbers.append(num)

    print("\n=== Adding issues to project ===")
    for num in created_numbers:
        print(f"  Issue #{num}...")
        add_issue_to_project(num, PROJECT_URL)

    print("\nDone.")


if __name__ == "__main__":
    main()
