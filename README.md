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
* Brightness adjustment
   * Brightness can be adjusted within the Web UI
   * Automatic brightness adjustment based on the ambient light
   * Double-click on the middle button on the clock turns the display on and off
* Better connection errors handling and display
* Multiple clock faces support
   * Default clock face can be selected in the Web UI
   * Clock faces can be changed using arrow buttons on the clock
   * (there is only one available clock face for now, other faces are still work-in-progress)
### My TODO list
* Add multiple faces and enable navigation between them
    * Glucose graph
    * BIG DIGITS
    * Clock (and setting the timezone)
    * Battery, humidity and temperature
* Smooth color change (rainbow) based on the value and boundaries
* ~~Add brightness adjustment and Night mode (automated brightness diming)~~ (done)
* ~~Add error message if no connection to Nightscout or NTP servers is possible~~ (done)
* Add smart update timings (calculate 5 minutes since last reading)
* Get units and value boundaries from Nightscout
* Change color to gray if the data is too old
* ~~Configuration during the installation (Improv WiFi)~~ (done)
* Create installation/configuration video
* Create a guide for setting up the development environment and code walkthrough for contributions
* Add more data sources
   * Dexcom
   * Medtronic
   * xDrip
   * [~~Juggluco~~](https://www.juggluco.nl/) (done)
   * [~~Gluroo~~](https://gluroo.com/) (done)
   * ...more... (if you are the author of a CGM data collecting app/service nad you want your data to be displayed on the Nightscout Clock, please contact me)
* Add audible alarms
 
---
The code is heavily inspired by (has a lot of copy-pasted code from :D ) [AWTRIX Light](https://github.com/Blueforcer/awtrix-light) project.
