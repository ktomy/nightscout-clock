# Nightscout Clock
![Nihtscout clock logo](https://github.com/ktomy/nightscout-clock/assets/1446257/1198c06d-b017-409d-aca3-2bca63581ecb)
## This project is in the MVP state and is actively worked on

Nightscout Clock (or NSClock) is a custom firmware for [Ulanzi TC001](https://www.ulanzi.com/products/ulanzi-pixel-smart-clock-2882?aff=1191) allowing it to connect to Nightscout server, read blood glucose values and show them on the matrix screen

<p align=center>
<img height="400" src="https://ktomy.github.io/nightscout-clock/nightscout_clock_simple_face.jpg" />
</p>

If you already have an Ulanzi clock, you can try [installing](https://ktomy.github.io/nightscout-clock/) the Nightscout clock firmware

### Current version: 0.13

### Features
* Web-based installation (no need to install flashing tools, you just need the clock and a web browser)
* AP mode and web-based configuration: the clock hosts a website where the user can configure all the  parameters (data source, limits, alarms, display)
* Simple glucose value display with trend arrow
* Changing color based on limits
* Nightscout data source, the clock gett units type and value boundaries from Nightscout
* [Juggluco](https://www.juggluco.nl/) data source (support for HTTP Nightscout endpoints)
* [Improv WiFi](https://github.com/improv-wifi) compatibility (setting up WiFi during the installation)
* [Gluroo](https://gluroo.com/) data source (API_SECRET within the URL parameters)
* Dexcom Share data source
* Brightness adjustment
   * Brightness can be adjusted within the Web UI
   * Automatic brightness adjustment based on the ambient light
   * Double-click on the middle button on the clock turns the display on and off
* Multiple clock faces support
   * Default clock face can be selected in the Web UI
   * Clock faces can be changed using arrow buttons on the clock
   * Simple clock face (value and trend arrow)
   * Full-width glucose graph
   * Graph, value and trend indicator
   * BIG DIGITS
   * Value, trend and delta
   * Clock and BG value (timezone is set in the clock's web interface)
* Changes color to gray if the data is too old
* Smart data and screen update timings: read data once it appears, refresh screen when needed
* API data source. The clock has a simple Nightscout-like API which can receive glucose values from an external source. The main purpose of this feature is the ability to test the clock during the clockfaces development. In order to activate this feature, select the API data source within the clock's Web UI. Here are the endpoints:
    * /api/v1/entries POST endpoint receives an array of Nightscout-like entries. The only significant fields are `sgv`, `date` and `trend` or `direction`. Due to the limited memory the API is stable when sent less than 10 recotds
    * /api/v1/entries DELETE endpoint deleted all entries no matter the payload
* Firmware versioning
* Alarms with configurable Thresholds, snooze times and silence intervals

### My TODO list
* Add more clock faces
    * Battery, humidity and temperature
* Smooth color change (rainbow) based on the value and boundaries
* Support multiple WiFi network configurations (WiFi backup)
* Create installation/configuration video
* Create a guide for setting up the development environment and code walkthrough for contributions
* Add more data sources
   * Medtronic
   * ...more... (if you are the author of a CGM data collecting app/service nad you want your data to be displayed on the Nightscout Clock, please contact me)
 
---
The code is heavily inspired by (has a lot of copy-pasted code from :D ) [AWTRIX Light](https://github.com/Blueforcer/awtrix-light) project.
