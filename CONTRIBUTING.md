## Do you want to contribute?

Contributions are welcomed. I will look into all the pull requests and will most probably merge yours if it brings additional value to the project. Before creating a pull request, please create a discussion topic or an issue so that we can talk about what you intend to improve and come up with the best way to do it.

### Setting up development environment

My computer is running linux, but you can use Windows or MacOS as well, there are no showstoppers. The only part which is unix-oriented are some additional scripts which help me automate building, testing and monitoring. But in most cases they are not needed.

- Install Visual Studio Code
- Install PlatformIO
  - install python as a PlatformIO dependency
- clone the project using Visual Studio Code
- Install Node.js using the version in `.node-version` for filesystem builds and screenshots. No npm packages are required.
- Filesystem builds also find that version under NVM (`NVM_DIR` or `~/.nvm`) when VS Code does not inherit your shell's Node.js path. For other installations, start VS Code from a terminal where `node --version` works.
- PlatformIO should detect the project
- Configure the serial port for your machine:
  - Copy `platformio.local.ini.example` to `platformio.local.ini`
  - Uncomment the appropriate `upload_port` example and set it to your device
  - Optionally uncomment `monitor_port` for PlatformIO's native serial monitor
  - Find available ports with `pio device list`; prefer a stable `/dev/serial/by-id/` path on Linux
  - `platformio.local.ini` is ignored by Git and overrides the shared settings in `platformio.ini`
- You should be able to see PlatformIO tab in the sidebar
  - Select `ulanzi_debug` -> `General` -> `Build`
  - `ulanzi_debug` -> `Platform` -> `Build Filesystem image`
  - Connect the clock to the USB port of your computer
  - `ulanzi_debug` -> `Platform` -> `Upload Filesystem Image`
  - `ulanzi_debug` -> `General` -> `Upload and monitor`
- If you are lucky enough, the clock should restart and run your local version of NSClock
- You should be able to see the debug output in the VS Code terminal
- If something goes wrong and you are stuck, feel free to start a [discussion](https://github.com/ktomy/nightscout-clock/discussions)

### Updating the web UI screenshot

The web UI source is in `web/src/`, including the original icon and timezone JSON in
`web/src/assets/`. PlatformIO generates the compressed files in `data/` before building
LittleFS, including IDE filesystem actions, `scripts/build.sh --fs` / `--all`, and both
GitHub Actions build workflows. Firmware-only builds do not require Node.js.

The generated `.gz` files are ignored by Git. Commit changes to their sources, not the
compressed outputs. Run `node web/build.mjs` to generate assets without building LittleFS.

The web UI's clock preview runs the display code as `data/clockemu.js.gz`. After changing a face or
other display code, build the `ulanzi` environment once, install
[emsdk](https://emscripten.org/docs/getting_started/downloads.html) 6.0.9
(`./emsdk install 6.0.9 && ./emsdk activate 6.0.9`), run `EMSDK=<emsdk folder> web/emulator/build.sh`
and `node web/build.mjs`, and commit `data/clockemu.js.gz`. An unchanged checkout rebuilt with the same
PlatformIO libraries gives the same file, so `git status` shows whether the committed one matches.

`scripts/screenshot_web_ui.py` renders the current `data/` web UI in headless Chromium
and saves a full-page PNG to `docs/images/web-ui.png`, the image used in the README.
It regenerates the web assets first, so it also works from a clean checkout.
It uses factory defaults plus sample WiFi, Nightscout, and status data. No clock or
running web server is needed, and rendering makes no external network requests.
The current checkout's version is also used as the simulated latest version.
The default viewport is 1440 by 900 pixels, a desktop screen.

Install the screenshot dependencies once (Python 3.9 or newer):

```sh
python3 -m venv /tmp/nsclock-screenshot-venv
/tmp/nsclock-screenshot-venv/bin/python -m pip install playwright
/tmp/nsclock-screenshot-venv/bin/python -m playwright install chromium
```

Then regenerate the screenshot after changing the UI:

```sh
/tmp/nsclock-screenshot-venv/bin/python scripts/screenshot_web_ui.py
```

Use `--output /tmp/web-ui.png` for a preview, or `--width 390 --height 844` for a
phone viewport. The image always includes the full page. To use an existing
Chrome/Chromium installation instead of downloading Chromium, pass
`--browser /path/to/chrome`. Commit the regenerated README image with relevant UI changes.

### Release procedure

Use `scripts/release.py` for `patch`, `minor`, and `major` releases.

- Treat `data/version.txt` as the source of truth for the current version. The next version is calculated from that file.
- Make sure all intended release changes are already committed before starting the release, except pending changelog edits in `README.md`.
- Check whether `README.md` already contains changelog information for the upcoming version.
- If the changelog is not updated yet, add the release notes for the upcoming version to `README.md`, but do not manually change the `Current version` line.
- Run `scripts/release.py patch`, `scripts/release.py minor`, or `scripts/release.py major`.
- The script requires `main` to track `origin/main`, permits only pending `README.md` changelog edits, and fast-forward pulls before changing release metadata.
- The script exits on any failed Git operation and atomically pushes the release commit with only its release tag, so neither remote ref is updated if the push is rejected.
- After the tag is pushed, monitor the tag-triggered GitHub Actions workflow `Build and deploy on Github Pages`. The release is not complete until that workflow succeeds.
