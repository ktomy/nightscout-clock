"""Generate the web assets and the clock preview before PlatformIO builds the firmware.

The firmware serves the settings page from files built into it (board_build.embed_files in
platformio.ini), so every firmware build needs them current: the clock preview, which is the display
code compiled to WebAssembly with Emscripten, and the page itself.
"""

import os
from pathlib import Path
import shutil
import subprocess

Import("env")

# Targets that compile the firmware. Others, such as monitor, clean or the IDE's idedata, and uploads
# of an existing build (nobuild), need no web assets.
BUILD_TARGETS = {"buildprog", "upload", "program", "checkprogsize", "size", "debug", "__debug"}
targets = set(COMMAND_LINE_TARGETS)

if "nobuild" not in targets and (not targets or targets & BUILD_TARGETS):
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
    # The clock preview: the display code compiled to WebAssembly, rebuilt when it changed.
    libs = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env["PIOENV"]
    core = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32")) / "cores" / "esp32"
    try:
        subprocess.run([node, str(project / "web/emulator/build.mjs"), "--libs", str(libs), "--core", str(core)],
                       cwd=project, check=True)
    except subprocess.CalledProcessError:
        print("ERROR: The clock preview could not be built; firmware build stopped.")
        env.Exit(1)
    print("Generating web assets...")
    try:
        subprocess.run([node, str(project / "web/build.mjs")], cwd=project, check=True)
    except FileNotFoundError:
        print("ERROR: Node.js is required to build the web assets. Install the version in .node-version.")
        env.Exit(1)
    except subprocess.CalledProcessError:
        print("ERROR: Web asset generation failed; firmware build stopped.")
        env.Exit(1)
