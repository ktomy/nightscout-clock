# Nightscout Clock
![Nihtscout clock logo](https://github.com/ktomy/nightscout-clock/assets/1446257/1198c06d-b017-409d-aca3-2bca63581ecb)
## This project is in the MVP state and is actively worked on

Nightscout Clock (or NSClock) is a custom firmware for [Ulanzi TC001](https://www.ulanzi.com/products/ulanzi-pixel-smart-clock-2882?aff=1191) allowing it to connect to Nightscout server, read blood glucose values and show them on the matrix screen

<p align=center>
<img height="400" src="https://ktomy.github.io/nightscout-clock/nightscout_clock_simple_face.jpg" />
</p>

If you already have an Ulanzi clock, you can try [installing](https://ktomy.github.io/nightscout-clock/) the Nightscout clock firmware

### What already works
* Simple web-based installation (no need to install flashing tools, you just need the clock and a web browser)
* AP mode and web-based configuration: the clock hosts a website where the user can configure Nightscout hostname, units type and low/high limits
* Simple glucose value display with trend arrow
* Changing color based on limits (partially hard-coded)
* [Juggluco](https://www.juggluco.nl/) data source (support for HTTP Nightscout endpoints)
* [Improv WiFi](https://github.com/improv-wifi) compatibility (setting up WiFi during the installation)
* [Gluroo](https://gluroo.com/) data source (API_SECRET within the URL parameters)
* Dexcom Share data source
* Brightness adjustment
   * Brightness can be adjusted within the Web UI
   * Automatic brightness adjustment based on the ambient light
   * Double-click on the middle button on the clock turns the display on and off
* Better connection errors handling and display
* Multiple clock faces support
   * Default clock face can be selected in the Web UI
   * Clock faces can be changed using arrow buttons on the clock
   * Simple clock face (value and trend arrow)
   * Full-width glucose graph
   * Graph, value and trend indicator
   * BIG DIGITS
* Added API data dource. The clock has a simple Nightscout-like endpoint which can receive glucose values from an external source. The main purpose of this feature is to be able to test the clock during the clockfaces development. In order to activate this feature, select the API data source within the clock's Web UI. Here are the endpoints:
    * /api/v1/entries POST endpoint receives an array of Nightscout-like entries. The only significant fields are `sgv`, `date` and `trend` or `direction`. Due to the limited memory the API is stable when sent less than 10 recotds
    * /api/v1/entries DELETE endpoint deleted all entries no matter the payload
### My TODO list
* Add multiple faces ~~and enable navigation between them~~ (done)
    * ~~Glucose graph~~ (done)
    * ~~BIG DIGITS~~ (done)
    * Value, trend and difference from the old value
    * Clock (and setting the timezone)
    * Battery, humidity and temperature
* Smooth color change (rainbow) based on the value and boundaries
* ~~Add brightness adjustment and Night mode (automated brightness diming)~~ (done)
* ~~Add error message if no connection to Nightscout or NTP servers is possible~~ (done)
* Add smart update timings (calculate 5 minutes since last reading)
* Get units and value boundaries from Nightscout
* Change color to gray if the data is too old
* Support multiple WiFi network configurations (WiFi backup)
* ~~Configuration during the installation (Improv WiFi)~~ (done)
* Create installation/configuration video
* Create a guide for setting up the development environment and code walkthrough for contributions
* Add more data sources
   * ~~Dexcom~~ (done)
   * Medtronic
   * xDrip
   * [~~Juggluco~~](https://www.juggluco.nl/) (done)
   * [~~Gluroo~~](https://gluroo.com/) (done)
   * ...more... (if you are the author of a CGM data collecting app/service nad you want your data to be displayed on the Nightscout Clock, please contact me)
* Add audible alarms
 
---
The code is heavily inspired by (has a lot of copy-pasted code from :D ) [AWTRIX Light](https://github.com/Blueforcer/awtrix-light) project.
