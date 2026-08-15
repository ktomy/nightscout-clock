import json
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
VERSION_FILE = REPO_ROOT / "data" / "version.txt"
GLOBALS_FILE = REPO_ROOT / "src" / "globals.h"
MANIFEST_FILE = REPO_ROOT / "www" / "manifest.json"
README_FILE = REPO_ROOT / "README.md"
RELEASE_FILES = (VERSION_FILE, GLOBALS_FILE, MANIFEST_FILE, README_FILE)


def fail(message):
    print(f"Release failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def run_git(*args, check=True, capture_output=False):
    return subprocess.run(
        ["git", *args],
        cwd=REPO_ROOT,
        check=check,
        capture_output=capture_output,
        text=True,
    )


def git_output(*args):
    return run_git(*args, capture_output=True).stdout.strip()


def get_bump_type():
    if len(sys.argv) > 2:
        fail("usage: release.py [major|minor|patch]")

    bump_type = sys.argv[1].lower() if len(sys.argv) == 2 else "patch"
    if bump_type not in ("major", "minor", "patch"):
        fail("usage: release.py [major|minor|patch]")
    return bump_type


def get_next_version(current_version, bump_type):
    parts = current_version.split(".")
    if len(parts) == 2:
        parts.append("0")
    if len(parts) != 3:
        fail(f"invalid version in {VERSION_FILE.relative_to(REPO_ROOT)}: {current_version}")

    try:
        major, minor, patch = (int(part) for part in parts)
    except ValueError:
        fail(f"invalid version in {VERSION_FILE.relative_to(REPO_ROOT)}: {current_version}")

    if bump_type == "major":
        major += 1
        minor = 0
        patch = 0
    elif bump_type == "minor":
        minor += 1
        patch = 0
    else:
        patch += 1

    return f"{major}.{minor}.{patch}"


def get_unexpected_changes():
    status_lines = git_output("status", "--porcelain", "--untracked-files=all").splitlines()
    return [line for line in status_lines if line[3:] != "README.md"]


def ensure_release_worktree():
    unexpected_changes = get_unexpected_changes()
    if unexpected_changes:
        fail(
            "commit or stash all changes except README.md changelog edits:\n"
            + "\n".join(unexpected_changes)
        )


def ensure_tag_does_not_exist(tag_name):
    local_tag = run_git("show-ref", "--verify", "--quiet", f"refs/tags/{tag_name}", check=False)
    if local_tag.returncode == 0:
        fail(f"local tag {tag_name} already exists")
    if local_tag.returncode != 1:
        fail(f"could not check local tag {tag_name}")

    remote_tag = run_git(
        "ls-remote",
        "--exit-code",
        "--tags",
        "origin",
        f"refs/tags/{tag_name}",
        check=False,
        capture_output=True,
    )
    if remote_tag.returncode == 0:
        fail(f"remote tag {tag_name} already exists")
    if remote_tag.returncode != 2:
        if remote_tag.stderr:
            print(remote_tag.stderr, file=sys.stderr, end="")
        fail(f"could not check remote tag {tag_name}")


def preflight():
    if git_output("branch", "--show-current") != "main":
        fail("releases must be made from the main branch")

    try:
        upstream = git_output("rev-parse", "--abbrev-ref", "--symbolic-full-name", "@{upstream}")
    except subprocess.CalledProcessError:
        fail("main must have an upstream branch")
    if upstream != "origin/main":
        fail(f"main must track origin/main, currently tracking {upstream}")

    ensure_release_worktree()

    print("Updating main from origin/main...", flush=True)
    run_git("pull", "--ff-only", "origin", "main")
    ensure_release_worktree()

    print("Checking push access...", flush=True)
    run_git("push", "--dry-run", "origin", "HEAD:refs/heads/main")


def replace_single_line(path, prefix, replacement):
    with path.open("r", newline="") as input_file:
        lines = input_file.readlines()
    matches = [index for index, line in enumerate(lines) if line.startswith(prefix)]
    if len(matches) != 1:
        fail(f"expected exactly one {prefix!r} line in {path.relative_to(REPO_ROOT)}")

    line_ending = "\r\n" if lines[matches[0]].endswith("\r\n") else "\n"
    lines[matches[0]] = replacement + line_ending
    with path.open("w", newline="") as output_file:
        output_file.writelines(lines)


def update_release_files(version):
    VERSION_FILE.write_text(version)
    replace_single_line(GLOBALS_FILE, '#define VERSION "', f'#define VERSION "{version}"')

    manifest_data = json.loads(MANIFEST_FILE.read_text())
    manifest_data["version"] = version
    MANIFEST_FILE.write_text(json.dumps(manifest_data, indent=4) + "\n")

    replace_single_line(README_FILE, "### Current version:", f"### Current version: {version}")


def commit_and_push(version, tag_name):
    relative_release_files = [str(path.relative_to(REPO_ROOT)) for path in RELEASE_FILES]
    run_git("add", *relative_release_files)

    staged_changes = run_git("diff", "--cached", "--quiet", check=False)
    if staged_changes.returncode == 0:
        fail("release metadata did not change")
    if staged_changes.returncode != 1:
        fail("could not inspect staged release changes")

    run_git("commit", "-m", f"Bump version to {version}")
    run_git("tag", tag_name)

    print(f"Atomically pushing main and {tag_name}...", flush=True)
    try:
        run_git(
            "push",
            "--atomic",
            "origin",
            "HEAD:refs/heads/main",
            f"refs/tags/{tag_name}",
        )
    except subprocess.CalledProcessError:
        print(
            "Atomic push failed. The local release commit and tag remain; inspect the remote refs "
            "before retrying.",
            file=sys.stderr,
        )
        raise


def main():
    bump_type = get_bump_type()
    preflight()

    current_version = VERSION_FILE.read_text().strip()
    version = get_next_version(current_version, bump_type)
    tag_name = f"v-{version}"

    ensure_tag_does_not_exist(tag_name)
    update_release_files(version)
    commit_and_push(version, tag_name)
    print(
        f"Released {version}. Monitor the tag-triggered GitHub Actions workflow before considering it complete.",
        flush=True,
    )


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(f"Release stopped because a command failed: {' '.join(error.cmd)}", file=sys.stderr)
        raise SystemExit(error.returncode or 1)
