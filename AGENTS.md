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

`data/` contains the runtime web assets packed into the LittleFS image served by the clock. The configuration UI source is in `web/src/`, with the original icon and timezone JSON in `web/src/assets/`. PlatformIO runs `node web/build.mjs` before filesystem builds to generate `data/index.html.gz`, `data/favicon.ico.gz`, and `data/tzdata.json.gz`. These outputs are ignored by Git; commit their sources instead. Node.js must match the version in `.node-version`; no npm packages are required.

`test/` contains the host-side tests; see Testing Guidelines.

`www/` is the GitHub Pages site, including the browser installer and debug pages; it is not part of the firmware filesystem image. `scripts/` contains contributor tooling for local build, upload, monitoring, release, and emulator/test-data tasks. Core firmware build configuration lives in `platformio.ini` and `partitions.csv`; `wokwi.toml` and `diagram.json` describe the Wokwi simulator project (see "Testing in the Wokwi simulator").

## Build, Test, and Development Commands

Use the IDE's PlatformIO actions or the helper scripts in `scripts/`; avoid calling `platformio` directly from the shell for normal development.

### Build and flash workflows

- `scripts/build.sh`: the usual local build command for firmware changes.
- `scripts/build.sh --upload --monitor`: the usual edit-build-flash-monitor loop when testing on hardware.
- `scripts/build.sh --fs --upload --monitor`: use this when files in `data/` changed and the LittleFS web UI image must be rebuilt and reflashed.

### Less common helpers

- `scripts/monitor.sh`: opens the serial monitor directly and saves logs under `logs/` using PlatformIO's `log2file` filter.
- `scripts/reset.sh`: use this if the device becomes unstable and does not restart cleanly after upload.
- `scripts/upload.sh --all`: use only for a full device refresh, when bootloader, partitions, firmware, and LittleFS all need to be reflashed.
- `scripts/ns_emulator.py`: sends sample glucose entries to the device API for testing data processing and display behavior. Requires the clock's data source set to "API"; see the script's docstring for usage.
- `scripts/clock_address.py`: resolves the clock's address for scripts that call its API: `--url` first, then the git-ignored `.local_clock_ip` file in the repository root. New scripts that call the clock should use it.
- `scripts/merge_bins.sh`: merges the build outputs into a single distributable binary, but this is not part of the active day-to-day workflow.

### Release workflow

For releases, follow the canonical procedure in `CONTRIBUTING.md`.

- Use `scripts/release.py [patch|minor|major]`.
- Treat `data/version.txt` as the version source of truth.
- If needed, prepare the upcoming version changelog in `README.md`, but do not manually bump the `Current version` line before running the script.
- After the release tag is pushed, monitor the GitHub Actions workflow `Build and deploy on Github Pages` and require it to succeed.

`platformio.ini` defines the `ulanzi_debug` environment and loads the git-ignored `platformio.local.ini` for machine-specific settings. Copy `platformio.local.ini.example` and define `upload_port` there before using the upload, reset, or monitor scripts.

## Coding Style & Naming Conventions

Follow `.clang-format` for C++ changes: 4-space indentation, no tabs, attached braces, left-aligned pointers, sorted includes, and a soft line limit of 105 columns. Match the existing style before submitting firmware changes.

Use the repository's current naming patterns instead of introducing a new scheme:

- Classes and manager types use PascalCase, for example `ServerManager_`, `BGAlarmManager`, and `SettingsManager`.
- Related feature families are grouped by filename prefix, such as `BGSource*` for data providers and `BGDisplayFace*` for display variants.
- Headers and implementation files should stay paired as `Name.h` and `Name.cpp`.
- Settings fields use `snake_case`; keep new persisted configuration names aligned with the existing JSON and settings model.
- Global/shared items are kept explicit in files like `globals.*` and `enums.h`; avoid adding new cross-cutting globals unless there is no cleaner boundary.

For the web UI, keep JavaScript style consistent with the existing files in `web/src/js/`: semicolon-light, mostly `const`/`let`, early returns for small guards, and descriptive function names such as `loadSettings`. Reuse the existing HTML/JS structure instead of introducing new frontend tooling, frameworks or libraries; the page must work on a phone connected to the clock's setup network, with no internet.

## Compatibility and Validation Scope

- Firmware upgrades use a full reflash, including replacement of the configuration from `data/config_initial.json`. Do not add migration code, legacy-setting fallbacks, or downgrade compatibility unless explicitly requested.
- Configuration is managed through the provided Web UI. Hand-edited configurations and hostile configuration submissions to `/api/save` are outside the supported workflow. Validate user-editable fields in the UI; avoid duplicating those checks throughout the firmware without a concrete need.
- Keep checks required for normal operational failures, such as failed network requests, unavailable time, missing files, and malformed responses from external services. Keep bounds checks needed for memory safety and existing authentication controls.

## Testing Guidelines

`test/run_tests.sh` runs the host-side tests, which need only `g++` and Node.js and also run on every pull request. They cover logic that compiles without Arduino, such as `src/SettingsAlertWindow.h`, and the web UI's `web/src/js/schema.js`. Tests must exercise the real source rather than a copy of it; to test firmware logic, move it into an Arduino-free header in `src/`.

Host-side tests do not replace hardware testing: firmware changes must be validated by building and testing on a real device before a pull request is created, as described in "Pull request rules" in `CONTRIBUTING.md`. The normal workflow is `scripts/build.sh --upload --monitor`, or `scripts/build.sh --fs --upload --monitor` when files in `data/` changed.

Prefer testing against the real data source affected by your change. For display logic, parsing, and update flow, `scripts/ns_emulator.py` can push sample glucose entries into the device API so you can exercise behavior without waiting for live data.

Watch the serial monitor for boot issues, configuration errors, Wi-Fi problems, and source-specific failures. If the device does not restart cleanly after flashing, use `scripts/reset.sh` and retest.

### Testing in the Wokwi simulator

The Wokwi simulator runs the firmware on a simulated ESP32 with the 32x8 matrix, the three buttons, the light sensor, and the buzzer. It does not replace hardware testing.

Coding agents run Wokwi simulations only when the user explicitly asks for them. They are not part of the standard verification sequence: do not run them automatically after a change, before a commit, or before a pull request. Each run is slow and uses the user's Wokwi CI minutes.

- `scripts/wokwi.sh [--no-build] [wokwi-cli options]` builds the `wokwi` PlatformIO environment (`-DWOKWI`) and a LittleFS image, then runs `wokwi-cli` in Wokwi's cloud. It needs `wokwi-cli` and a CI token in `WOKWI_CLI_TOKEN` or the git-ignored `.wokwi_token` file. The `wokwi` environment builds its LittleFS image with `scripts/wokwi_data.py`: `data/config_initial.json` plus `scripts/wokwi_config.json` (Wokwi-GUEST Wi-Fi, "API" data source), never your `data/config.json`. In VS Code, select the `wokwi` environment and run Build, Build Filesystem Image, and the custom "Merge Wokwi image" task (or `scripts/merge_bins.sh wokwi`) to make the `merged_firmware.bin` that `wokwi.toml` loads.
- `wokwi-cli` cannot reach the clock over HTTP. The `wokwi` build replaces Improv's serial input with the commands in `src/WokwiConsole.h`: `bg <sgv> [trend] [minutes_ago]`, `clear`, and `screen`, which prints the LED colors.
- Scenarios in `scripts/wokwi_scenarios/` drive a run: `wait-serial`, `write-serial` for those commands, and `set-control` for buttons (`btn_left`, `btn_select`, `btn_right`) and the light sensor (`ldr`). Start from `smoke.yaml`.
- `python3 scripts/wokwi_screen.py <serial log> --ascii` turns the `screen` dumps into PNG files and text, because Wokwi cannot take screenshots of its NeoPixel canvas.
- `--timeout` counts simulated time, and the simulation runs at about half of real-time speed. Run long simulations in the background, and keep timeouts close to what the scenario needs.

## Commit & Pull Request Guidelines

Before committing or opening a pull request, read and follow the introduction, "Pull request rules", "Commit messages", and "Pull request branch permissions" in [`CONTRIBUTING.md`](CONTRIBUTING.md), the canonical source for these rules.

## Configuration Tips

The upload, reset, and monitor scripts resolve the same `upload_port` through `scripts/get_port.sh`. If flashing or monitoring fails on a different machine or after reconnecting the clock, update `upload_port` in `platformio.local.ini` before troubleshooting deeper firmware issues.

Treat `web/src/` as the web UI source and `data/` as the runtime payload built from it. If you change the device-served UI, verify the browser behavior; also verify the rebuilt filesystem image on hardware when the same change touches firmware.
