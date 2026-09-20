"""Generate web assets before PlatformIO collects files for a filesystem image."""

import os
from pathlib import Path
import shutil
import subprocess

Import("env")

# Firmware-only builds and uploads of an existing image do not need Node.
if set(COMMAND_LINE_TARGETS) & {"buildfs", "uploadfs", "uploadfsota"} and "nobuild" not in COMMAND_LINE_TARGETS:
    project = Path(env.subst("$PROJECT_DIR"))
    node = shutil.which("node")
    if not node:
        # GUI-launched VS Code may not inherit the PATH set by NVM in a shell.
        version = (project / ".node-version").read_text().strip().removeprefix("v")
        nvm_dir = Path(os.environ.get("NVM_DIR") or Path.home() / ".nvm")
        candidate = nvm_dir / "versions" / "node" / ("v" + version) / "bin" / "node"
        if candidate.is_file() and os.access(candidate, os.X_OK):
            node = str(candidate)
    if not node:
        print("ERROR: Node.js was not found on PATH or in NVM for the version in .node-version.")
        print("Install that version and restart VS Code from a terminal where node --version works.")
        env.Exit(1)
    print("Generating web assets...")
    try:
        subprocess.run([node, str(project / "web/build.mjs")], cwd=project, check=True)
    except FileNotFoundError:
        print("ERROR: Node.js is required to build the web assets. Install the version in .node-version.")
        env.Exit(1)
    except subprocess.CalledProcessError:
        print("ERROR: Web asset generation failed; filesystem build stopped.")
        env.Exit(1)
