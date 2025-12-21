# Nightscout Clock

![Nightscout clock logo](https://github.com/ktomy/nightscout-clock/assets/1446257/1198c06d-b017-409d-aca3-2bca63581ecb)

### Current version: 0.25.2

![Build and Release](https://github.com/ktomy/nightscout-clock/actions/workflows/build_release.yml/badge.svg)

_Nightscout Clock (or NSClock) is an open-source product aimed to help caregivers of people with type 1 diabetes have more piece of mind by being able to better glance at their loved onve blood glucose values._

![Photo of the NSClock](https://github.com/user-attachments/assets/9c5d810a-76c0-414b-8d93-d46a6afa8bf6)

## Here is what it can do

- 6 colorful clockfaces
- Can get glucose data from Dexcom Share or Nightscout
- Supports mg/dl and mmol/l
- 10 minutes setup through web browser
- Configurable low/high limits
- Audible alarms in case the blood sugar is too low or too high
- Automatic brightness adjustment
- Notifies of stall data
- ...and more

### YouTube review

[![YouTube video](https://img.youtube.com/vi/7GmDflLxqLs/0.jpg)](https://www.youtube.com/watch?v=7GmDflLxqLs)

Thanks [@CallumMcK](https://github.com/CallumMcK)

## How to install

1. Buy [Ulanzi TC001](https://www.ulanzi.com/products/ulanzi-pixel-smart-clock-2882?aff=1191), it is about $50, so you don't have to sell a kidney, it is available both in US and Europe (Aliexpress sells it as well, for the same price)
2. Wait a few days for the delivery
3. Unpack, turn on (press on `<` and `>` buttons for a few seconds)
4. Connect the USB-C cable (comes with the clock) to your computer
5. Go to the [installation page](https://ktomy.github.io/nightscout-clock/)
6. Follow the instructions
7. Once the clock installed, take out your phone and join `nsclock` wi-fi network. Then go to `http://192.168.4.1/`
8. Set up your device, provide the Wi-Fi network details, your Dexcom or Nightscout credentials, glucose warning limits and other parameters
9. You're all set, enjoy!

## How to update

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/7mFZJ7_EFN4/0.jpg)](https://www.youtube.com/watch?v=7mFZJ7_EFN4)

Thanks [@CallumMcK](https://github.com/CallumMcK)

## More information for people who needs it

Nightscout CLock is a custom firmware for Ulanzi TC001. It can also run (with minor changes) on AWTRIX-Light custom hardware, so if you need bigger display, feel free to research.

### Clockfaces

| Name            | Look                                                                                                 |
| --------------- | ---------------------------------------------------------------------------------------------------- |
| SImple          | ![resized_IMG_0225](https://github.com/user-attachments/assets/79cbda6d-5c0b-47fa-b5b4-2a5a10322a7d) |
| BIG DIGITS      | ![resized_IMG_0222](https://github.com/user-attachments/assets/59d5bea9-977b-4c40-b308-636d3d70055a) |
| 3-hours graph   | ![resized_IMG_0220](https://github.com/user-attachments/assets/86f36885-4479-412b-95fc-9fa527e12050) |
| Graph and value | ![resized_IMG_0221](https://github.com/user-attachments/assets/cb21ac92-a6d5-408c-b116-69726b58abc1) |
| Delta           | ![resized_IMG_0223](https://github.com/user-attachments/assets/dce1ecd4-a51b-4596-a292-0535c66f505c) |
| Time and value  | ![resized_IMG_0217](https://github.com/user-attachments/assets/d92832c2-8713-4ccf-9cc7-67202153d111) |

### Configuration web interface

<img alt="webUI" src="https://github.com/user-attachments/assets/94222c87-3f96-46f9-a773-02f7cdb16e6b" />

### Features (technical stuff, feel free to ignore)

- Web-based installation (no need to install flashing tools, you just need the clock and a web browser). The clock hosts a website where the user can configure all the parameters (data source, limits, alarms, display)
  - Access-Point mode to set the initial configuration
  - Support for a secondary WiFi (e.g. if you want to take the clock in your car for a long trip)
  - Support for WPA-Enterprise
  - Ability to set custom hostname in in case you have multiple NSClocks on the same network
- Simple glucose value display with trend arrow
- Changing color based on limits
- Nightscout data source, the clock gett units type and value boundaries from Nightscout (see [how to](https://youtu.be/GGiep2gdx_o) set up using [Nightscout.pro](https://www.nightscout.pro/) as data source)
- [Juggluco](https://www.juggluco.nl/) data source (support for HTTP Nightscout endpoints)
- [Improve WiFi](https://github.com/improv-wifi) compatibility (setting up WiFi during the installation)
- [Gluroo](https://gluroo.com/) data source (API_SECRET within the URL parameters) (see how to setup [video](https://youtu.be/unG-l6XXWxw))
- Dexcom Share data source
- LibreLinkUp (libreview) data source
- Brightness adjustment
  - Brightness can be adjusted within the Web UI
  - Automatic brightness adjustment based on the ambient light
  - Double-click on the middle button on the clock turns the display on and off
- Multiple clock faces support
  - Default clock face can be selected in the Web UI
  - Clock faces can be changed using arrow buttons on the clock
  - Simple clock face (value and trend arrow)
  - Full-width glucose graph
  - Graph, value and trend indicator
  - BIG DIGITS
  - Value, trend and delta
  - Clock and BG value (timezone is set in the clock's web interface)
- Changes color to gray if the data is too old
- Smart data and screen update timings: read data once it appears, refresh screen when needed
- API data source. The clock has a simple Nightscout-like API which can receive glucose values from an external source. The main purpose of this feature is the ability to test the clock during the clockfaces development. In order to activate this feature, select the API data source within the clock's Web UI. Here are the endpoints:
  - /api/v1/entries POST endpoint receives an array of Nightscout-like entries. The only significant fields are `sgv`, `date` and `trend` or `direction`. Due to the limited memory the API is stable when sent less than 10 recotds
  - /api/v1/entries DELETE endpoint deleted all entries no matter the payload
- Firmware versioning
- Alarms with configurable Thresholds, snooze times and silence intervals
- To turn the device on or off press both arrow buttons for 3 seconds
- To reset the device to factory defaults (hard reset) during boot sequence (when version number is displayed) keep the center (select) button pressed.

### My TODO list

- Add more clock faces
  - Battery, humidity and temperature
- Smooth color change (rainbow) based on the value and boundaries
- Add more data sources
  - Medtronic
  - ...more... (if you are the author of a CGM data collecting app/service nad you want your data to be displayed on the Nightscout Clock, please contact me)

## Changes

### 0.25.2

- Fixed support for Dexcom Japan
- Added custom alarm melodies, one can set them through RTTTL strings in alrts settings

### 0.25.1

- Added support for Dexcom Japan: [#110](https://github.com/ktomy/nightscout-clock/discussions/110)
- Fixed typo in Dexcom credentials error message: [#111](https://github.com/ktomy/nightscout-clock/issues/111)

### 0.25.0

- Implemented Alarm Intensive Mode, addressing [#61](https://github.com/ktomy/nightscout-clock/discussions/61)

### 0.24.4

- Fixed WiFi password validation for Open WiFi networks
- Added possibility of factory resetting the device on start-up (when version string is displaying press the select button)
- Better AP mode handling

### 0.24.3

- Implemented open WiFi connectivity, closing [#105](https://github.com/ktomy/nightscout-clock/issues/105)
- Changed age bars colors so that they do not disappear when the brightness is minimal. [#101](https://github.com/ktomy/nightscout-clock/discussions/101)

### 0.24.2

- Increased version presented to LibreLinkUp servers, closing [#88](https://github.com/ktomy/nightscout-clock/issues/88)

### 0.24

- Added "dark rooms" brightness mode and changed curve for the manual brightness. Thanks [@unxmaal](https://github.com/unxmaal) for [#67](https://github.com/ktomy/nightscout-clock/pull/67)

### 0.23.1

- Fixed trend arrow display on the Big Text clockface when brightness is not in auto mode. [#47](https://github.com/ktomy/nightscout-clock/issues/47)
- Adjusted version display on start-up

### 0.23

- Added version number, update check and link to the updating page to the WebUI. [#10](https://github.com/ktomy/nightscout-clock/issues/10)
- Added development environment setup instructions. [#26](https://github.com/ktomy/nightscout-clock/issues/26)
- Fixed LibreLink Up Server label. [#50](https://github.com/ktomy/nightscout-clock/issues/50)

### 0.22

- Fixed delta for LibreLinkUp source
- Now I can consider LibreLinkUp as being functioning. So closing [#9](https://github.com/ktomy/nightscout-clock/issues/9) and [#27](https://github.com/ktomy/nightscout-clock/issues/27)

### 0.21

- Yet another attempt to fix LibreLinkUp

### 0.20

- Another attempt to get values from LibreLinkUp

### 0.19

- Added custom "Data is old" threshold, (thanks [@TheBeardedTechGuy](https://github.com/TheBeardedTechGuy) for [#43](https://github.com/ktomy/nightscout-clock/pull/43))
- Added glucose age display as bars in the bottom part of most of the clockfaces (thanks [@TheBeardedTechGuy](https://github.com/TheBeardedTechGuy) for [#41](https://github.com/ktomy/nightscout-clock/pull/41))

### 0.18

- Fixed Nightscout domain name validation, closed [#39](https://github.com/ktomy/nightscout-clock/issues/39)

### 0.17

- Added API endpoint to switch display power on and off as in the example below

```
curl -X POST http://nsclock.lan/api/displaypower \
  -H "Content-Type: application/json" \
  -d '{"power": "off"}'
```

### 0.16

- Another attempt to fix [#25](https://github.com/ktomy/nightscout-clock/issues/25) and [#31](https://github.com/ktomy/nightscout-clock/issues/31)

### 0.15

- Fixed delta showing 0 when there are multiple sources uploading data to Nightscout. [#25](https://github.com/ktomy/nightscout-clock/issues/25)
- Implemented WPA-Enterprise connectivity (Wifi type selector available for additional WiFi network). [#23](https://github.com/ktomy/nightscout-clock/issues/23)
- Fixed project dependencies divergence. [#30](https://github.com/ktomy/nightscout-clock/issues/30)
- Implemented possibility of setting custom hostname. [#18](https://github.com/ktomy/nightscout-clock/issues/18)
- Implementing possibility of having additional WiFi network. [#7](https://github.com/ktomy/nightscout-clock/issues/7)

### 0.14

- Updated README to be more customer-friendly
- Started preparing for LibreLink Up data source integration, [#9](https://github.com/ktomy/nightscout-clock/issues/9))
- Improved memory management to avoid NOMEMORY issues and unplanned restarts, [#19](https://github.com/ktomy/nightscout-clock/issues/19)
- Improved WiFi SSID validation, [#16](https://github.com/ktomy/nightscout-clock/issues/16)
- Fixed typos in Web UI (thanks @motinis)

### 0.13

- Finished alarms feature
- Improved WiFi reconnection handling [#15](https://github.com/ktomy/nightscout-clock/issues/15)
- Migrated development environment to Linux
- Removed boot sound

## Do you want to contribute?

Contributions are welcomed. I will look into all the pull requests and will most probably merge yours if it brings additional value to the project. Before creating a pull request, please create a discussion topic or an issue so that we can talk about what you intend to improve and come up with the best way to do it.

### Setting up development environment

My computer is running linux, but you can use Windows or MacOS as well, there are no showstoppers. The only part which is unix-oriented are some additional scripts which help me automate building, testing and monitoring. But in most cases they are not needed.

- Install Visual Studio Code
- Install PlatformIO
  - install python as a PlatformIO dependency
- clone the project using Visual Studio Code
- PlatformIO should detect the project
- You should be able to see PlatformIO tab in the sidebar
  - Select `ulanzi_debug` -> `General` -> `Build`
  - `ulanzi_debug` -> `Platform` -> `Build Filesystem image`
  - Connect the clock to the USB port of your computer
  - `ulanzi_debug` -> `Platform` -> `Upload Filesystem Image`
  - `ulanzi_debug` -> `General` -> `Upload and monitor`
- If you are lucky enough, the clock should restart and run your local version of NSClock
- You should be able to see the debug output in the VS Code terminal
- If something goes wrong and you are stuck, feel free to start a [discussion](https://github.com/ktomy/nightscout-clock/discussions)

---

The code is heavily inspired by (has a lot of copy-pasted code from :D ) [AWTRIX Light](https://github.com/Blueforcer/awtrix-light) project.
