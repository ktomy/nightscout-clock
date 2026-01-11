## Do you want to contribute?

Contributions are welcomed. I will look into all the pull requests and will most probably merge yours if it brings additional value to the project. Before creating a pull request, please create a discussion topic or an issue so that we can talk about what you intend to improve and come up with the best way to do it.

### Setting up development environment

My computer is running linux, but you can use Windows or MacOS as well, there are no showstoppers. The only part which is unix-oriented are some additional scripts which help me automate building, testing and monitoring. But in most cases they are not needed.

- Install Visual Studio Code
- Install PlatformIO
  - install python as a PlatformIO dependency
- clone the project using Visual Studio Code
- PlatformIO should detect the project
- Comment out (place `#` at the beginning of) [this](https://github.com/ktomy/nightscout-clock/blob/main/platformio.ini#L38) line
- You should be able to see PlatformIO tab in the sidebar
  - Select `ulanzi_debug` -> `General` -> `Build`
  - `ulanzi_debug` -> `Platform` -> `Build Filesystem image`
  - Connect the clock to the USB port of your computer
  - `ulanzi_debug` -> `Platform` -> `Upload Filesystem Image`
  - `ulanzi_debug` -> `General` -> `Upload and monitor`
- If you are lucky enough, the clock should restart and run your local version of NSClock
- You should be able to see the debug output in the VS Code terminal
- If something goes wrong and you are stuck, feel free to start a [discussion](https://github.com/ktomy/nightscout-clock/discussions)