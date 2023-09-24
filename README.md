# Nightscout Clock

## This project is in the MVP state and is actively worked on

Nightscout Clock (or NSClock) is a custom firmware for [Ulanzi TC001](https://www.aliexpress.com/item/1005005531845548.html?spm=a2g0o.order_list.order_list_main.33.3aa11802BcFCFy) allowing it to connect to Nightscout server, read blood glucose values and show them on the matrix screen

<p align=center>
<img height="400" src="https://ktomy.github.io/nightscout-clock/nightscout_clock_simple_face.jpg" />
</p>

If you already have an Ulanzi clock, you can try [installing](https://ktomy.github.io/nightscout-clock/) the Nightscout clock firmware

### What already works
* Simple web-based installation (no need to install flashing tools, you just need the clock and a web browser)
* AP mode and web-based configuration: the clock hosts a website where the user can configure Nightscout hostname, units type and low/high limits
* Simple glucose value display with trend arrow
* Changing color based on limits (yellow for high, red for low)

### My TODO list
* Add multiple faces and enable navigation between them
    * Glucose graph
    * BIG DIGITS
    * Clock (and setting the timezone)
    * Battery, humidity and temperature
* Smooth color change (rainbow) based on the value and boundaries
* Add brightness adjustment and Night mode (automated brightness diming)
* Add error message if no connection to Nightscout or NTP servers is possible
* Add smart update timings (calculate 5 minutes since last reading)
* Get units and value boundaries from Nightscout
* Change color to gray if the data is too old
* Configuration during the installation (Simep WiFi)
* Create installation/configuration video
* Create a guide for setting up the development environment and code walkthrough for contributions
* Add audible alarms
 
---
The code is heavily inspired by (has a lot of copy-pasted code from :D ) [AWTRIX Light](https://github.com/Blueforcer/awtrix-light) project.
