"""PlatformIO pre-script for the "wokwi" environment.

Adds the "Merge Wokwi image" task (target "merge"), which runs scripts/merge_bins.sh to make the
merged_firmware.bin that wokwi.toml loads.

The simulator's settings need no filesystem image: the wokwi environment builds
scripts/wokwi_config.json into the firmware (platformio.ini), and its factory settings are
data/config_initial.json with those values applied. Personal settings never reach Wokwi's cloud.
"""

Import("env")

env.AddCustomTarget(
    name="merge",
    dependencies=None,
    actions=['bash "$PROJECT_DIR/scripts/merge_bins.sh" wokwi'],
    title="Merge Wokwi image",
    description="Merge bootloader, partitions, boot_app0 and firmware into merged_firmware.bin",
)
