# Repository Guidelines

## Project Structure & Module Organization

`src/` contains the main firmware code and is the primary place contributors will work. `main.cpp` handles startup and wires the subsystems together. The rest of the directory is organized by module family:

- `BGSource*`: glucose data providers such as Nightscout, Dexcom, LibreLinkUp, Medtrum, Medtronic, and API-backed input.
- `BGDisplayFace*`: clock face implementations and rendering logic for the different display layouts.
- `BGDisplayManager`, `DisplayManager`: display update flow, screen state, and rendering coordination.
- `Settings*`: configuration models, persistence, and settings management.
- `BGAlarmManager`: threshold checking, snoozing, and alarm behavior.
- `PeripheryManager`: hardware-facing device input and peripheral handling.
- `ServerManager`: the embedded web/configuration server exposed by the device.
- `globals.*` and `enums.h`: shared firmware definitions and support code.
- `improv_consume.*`: integration layer that consumes the vendored Improv library during initial Wi-Fi setup.

When adding firmware behavior, prefer extending the existing module family instead of creating unrelated one-off files.

`lib/` contains third-party libraries vendored into the repository, including `LightResistor/`, `Hashing/`, `MelodyPlayer/`, `Fonts/`, and `Improv/`. Prefer keeping application-specific code in `src/`; only add to `lib/` when importing or maintaining an external library.

`data/` contains the on-device configuration UI and the runtime web assets packed into the LittleFS image served by the clock. `data_dev/` supports local web UI development when using the IDE's local web server; pages in `data/` may reference assets from both `data/` and `data_dev/` during that workflow.

`www/` is the GitHub Pages site, including the browser installer and debug pages; it is not part of the firmware filesystem image. `scripts/` contains contributor tooling for local build, upload, monitoring, release, and emulator/test-data tasks. Core firmware build configuration lives in `platformio.ini` and `partitions.csv`; `wokwi.toml` is only for experimental emulator setup and is not part of the regular test workflow.

## Build, Test, and Development Commands

Use the IDE's PlatformIO actions or the helper scripts in `scripts/`; avoid calling `platformio` directly from the shell for normal development.

### Build and flash workflows

- `scripts/build.sh`: the usual local build command for firmware changes.
- `scripts/build.sh --upload --monitor`: the usual edit-build-flash-monitor loop when testing on hardware.
- `scripts/build.sh --fs --upload --monitor`: use this when files in `data/` changed and the LittleFS web UI image must be rebuilt and reflashed.

### Less common helpers

- `scripts/monitor.sh`: opens the serial monitor directly and saves logs under `log/`.
- `scripts/reset.sh`: use this if the device becomes unstable and does not restart cleanly after upload.
- `scripts/upload.sh --all`: use only for a full device refresh, when bootloader, partitions, firmware, and LittleFS all need to be reflashed.
- `scripts/ns_emulator.py`: sends sample glucose entries to the device API for testing data processing and display behavior.
- `scripts/merge_bins.sh`: merges the build outputs into a single distributable binary, but this is not part of the active day-to-day workflow.

### Release workflow

For releases, follow the canonical procedure in `CONTRIBUTING.md`.

- Use `scripts/release.py [patch|minor|major]`.
- Treat `data/version.txt` as the version source of truth.
- If needed, prepare the upcoming version changelog in `README.md`, but do not manually bump the `Current version` line before running the script.
- After the release tag is pushed, monitor the GitHub Actions workflow `Build and deploy on Github Pages` and require it to succeed.

`platformio.ini` defines the `ulanzi_debug` environment and the serial upload port used by the scripts. Check `upload_port` before flashing from a different machine or after reconnecting the device.

## Coding Style & Naming Conventions

Follow `.clang-format` for C++ changes: 4-space indentation, no tabs, attached braces, left-aligned pointers, sorted includes, and a soft line limit of 105 columns. Match the existing style before submitting firmware changes.

Use the repository's current naming patterns instead of introducing a new scheme:

- Classes and manager types use PascalCase, for example `ServerManager_`, `BGAlarmManager`, and `SettingsManager`.
- Related feature families are grouped by filename prefix, such as `BGSource*` for data providers and `BGDisplayFace*` for display variants.
- Headers and implementation files should stay paired as `Name.h` and `Name.cpp`.
- Settings fields use `snake_case`; keep new persisted configuration names aligned with the existing JSON and settings model.
- Global/shared items are kept explicit in files like `globals.*` and `enums.h`; avoid adding new cross-cutting globals unless there is no cleaner boundary.

For the web UI, keep JavaScript style consistent with the existing files in `data/` and `data_dev/`: semicolon-light, mostly `const`/`let`, early returns for small guards, and descriptive handler names such as `toggleWebAuthSettings`. Reuse the existing HTML/JS structure instead of introducing new frontend tooling or frameworks.

## Testing Guidelines

There is no formal automated test suite in the repository yet, so every change should be validated by building and testing on a real device. The normal workflow is `scripts/build.sh --upload --monitor`, or `scripts/build.sh --fs --upload --monitor` when files in `data/` changed.

Prefer testing against the real data source affected by your change. For display logic, parsing, and update flow, `scripts/ns_emulator.py` can push sample glucose entries into the device API so you can exercise behavior without waiting for live data.

Watch the serial monitor for boot issues, configuration errors, Wi-Fi problems, and source-specific failures. If the device does not restart cleanly after flashing, use `scripts/reset.sh` and retest. A Wokwi emulator setup exists, but it is experimental and should not replace hardware verification.

## Commit & Pull Request Guidelines

Keep commit messages short, imperative, and specific, following the existing history (for example, `Fixed typo in WebUI javascript` or `Added web auth in the advanced settings`). Avoid vague summaries that do not say what changed.

Before opening a pull request, start a discussion or issue for non-trivial changes so the approach can be aligned early. Pull requests should clearly state what changed, why it was needed, and how it was tested on hardware. Include screenshots when the change affects the web UI or display behavior.

## Configuration Tips

`scripts/` and `platformio.ini` assume a specific serial device path for upload. If flashing fails on a different machine or after reconnecting the clock, update `upload_port` before troubleshooting deeper firmware issues.

Treat `data/` as the runtime web UI payload and `data_dev/` as the local development source. If you change the device-served UI, verify both the browser behavior and the rebuilt filesystem image on hardware.
