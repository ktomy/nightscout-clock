"""PlatformIO pre-script for the "wokwi" environment.

Builds the simulator's LittleFS image from .pio/wokwi_data instead of data/: the same files
without data/config.json, plus a config.json made from data/config_initial.json with the values
in scripts/wokwi_config.json applied. Personal settings never reach Wokwi's cloud.

Also adds the "Merge Wokwi image" task (target "merge"), which runs scripts/merge_bins.sh to
make the merged_firmware.bin that wokwi.toml loads.
"""

import json
import shutil
from pathlib import Path

Import("env")

project = Path(env.subst("$PROJECT_DIR"))
stage = project / ".pio" / "wokwi_data"
env.Replace(PROJECT_DATA_DIR=str(stage))

# scripts/build_web.py runs first and generates the web assets in data/.
if set(COMMAND_LINE_TARGETS) & {"buildfs", "uploadfs"}:
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)
    for item in (project / "data").iterdir():
        if item.name in ("config.json", ".gitignore"):
            continue
        if item.is_dir():
            shutil.copytree(item, stage / item.name)
        else:
            shutil.copy2(item, stage / item.name)
    config = json.loads((project / "data" / "config_initial.json").read_text())
    config.update(json.loads((project / "scripts" / "wokwi_config.json").read_text()))
    (stage / "config.json").write_text(json.dumps(config, indent=4))
    print("Staged the Wokwi filesystem in", stage)

env.AddCustomTarget(
    name="merge",
    dependencies=None,
    actions=['bash "$PROJECT_DIR/scripts/merge_bins.sh" wokwi'],
    title="Merge Wokwi image",
    description="Merge bootloader, partitions, firmware and LittleFS into merged_firmware.bin",
)
