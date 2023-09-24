# Nightscout Clock

## This project is in a work-in-progress state

Nightscout Clock (or NSClock) is a custom firmware for [Ulanzi TC001](https://www.aliexpress.com/item/1005005531845548.html?spm=a2g0o.order_list.order_list_main.33.3aa11802BcFCFy) allowing it to connect to Nightscout server, read blood glucose values and show them on the matrix screen

If you already have an Ulanzi clock, you can try [installing](https://ktomy.github.io/nightscout-clock/) the Nightscout clock firmware

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
