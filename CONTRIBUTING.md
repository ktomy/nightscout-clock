## Do you want to contribute?

Contributions are welcomed. I will look into all the pull requests and will most probably merge yours if it brings additional value to the project. Before creating a pull request, please create a discussion topic or an issue so that we can talk about what you intend to improve and come up with the best way to do it.

### Setting up development environment

My computer is running linux, but you can use Windows or MacOS as well, there are no showstoppers. The only part which is unix-oriented are some additional scripts which help me automate building, testing and monitoring. But in most cases they are not needed.

- Install Visual Studio Code
- Install PlatformIO
  - install python as a PlatformIO dependency
- clone the project using Visual Studio Code
- PlatformIO should detect the project
- Configure the serial port for your machine:
  - Copy `platformio.local.ini.example` to `platformio.local.ini` (`cp platformio.local.ini.example platformio.local.ini`)
  - Uncomment the appropriate `upload_port` example and set it to your device
  - Optionally uncomment `monitor_port` for PlatformIO's native serial monitor
  - Optionally set `upload_speed` if you want a machine-specific speed override
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
