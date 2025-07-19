import os
import json
import subprocess
import sys

# Determine which part to increment
if len(sys.argv) > 1:
    bump_type = sys.argv[1].lower()
    if bump_type not in ("major", "minor", "patch"):
        print("Usage: release.py [major|minor|patch]")
        sys.exit(1)
else:
    bump_type = "patch"

# Load contents of version.txt
version_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "data", "version.txt"))
with open(version_file_path, "r") as version_file:
    version = version_file.read().strip()

# Parse version as major.minor.patch (default patch to 0 if missing)
version_parts = version.split(".")
if len(version_parts) == 2:
    major, minor = version_parts
    patch = 0
elif len(version_parts) == 3:
    major, minor, patch = version_parts
    patch = int(patch)
else:
    raise ValueError("Invalid version format in version.txt")

major = int(major)
minor = int(minor)
patch = int(patch)

if bump_type == "major":
    major += 1
    minor = 0
    patch = 0
elif bump_type == "minor":
    minor += 1
    patch = 0
else:  # patch
    patch += 1

version = f"{major}.{minor}.{patch}"

# Update version file
with open(version_file_path, "w") as version_file:
    version_file.write(version)

# Update globals.h file
globals_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "src", "globals.h"))
with open(globals_file_path, "r") as globals_file:
    lines = globals_file.readlines()

updated_lines = []
for line in lines:
    if line.startswith('#define VERSION "'):
        line = f'#define VERSION "{version}"\n'
    updated_lines.append(line)

with open(globals_file_path, "w") as globals_file:
    globals_file.writelines(updated_lines)

# Update manifest.json file
manifest_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "www", "manifest.json"))
with open(manifest_file_path, "r") as manifest_file:
    manifest_data = json.load(manifest_file)

manifest_data["version"] = version

with open(manifest_file_path, "w") as manifest_file:
    json.dump(manifest_data, manifest_file, indent=4)

# Update README.md file
readme_file_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "README.md"))
with open(readme_file_path, "r") as readme_file:
    lines = readme_file.readlines()

updated_lines = []
for line in lines:
    if line.startswith('### Current version:'):
        line = f'### Current version: {version}\n'
    updated_lines.append(line)

with open(readme_file_path, "w") as readme_file:
    readme_file.writelines(updated_lines)

# Commit the changes
commit_message = f"Bump version to {version}"
subprocess.run(["git", "add", version_file_path, globals_file_path, manifest_file_path, readme_file_path])

# Check if files are modified
modified_files = subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=no"]).decode("utf-8").splitlines()
modified_files = [file.split(" ", 1)[1] for file in modified_files]

version_file_name = os.path.basename(version_file_path)
globals_file_name = os.path.basename(globals_file_path)
manifest_file_name = os.path.basename(manifest_file_path)
readme_file_name = os.path.basename(readme_file_path)

modified_file_names = [os.path.basename(file.strip().split(' ')[-1]) for file in modified_files]

if version_file_name not in modified_file_names or globals_file_name not in modified_file_names or manifest_file_name not in modified_file_names or readme_file_name not in modified_file_names:
    print("No changes detected. Nothing to do.")

subprocess.run(["git", "commit", "-m", commit_message])

# Create git tag
# Use v-major.minor.patch format
tag_name = f"v-{version}"
subprocess.run(["git", "tag", tag_name])

subprocess.run(["git", "push"])
subprocess.run(["git", "push", "--tags"])




