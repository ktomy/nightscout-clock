# Nightscout Clock
![Nihtscout clock logo](https://github.com/ktomy/nightscout-clock/assets/1446257/1198c06d-b017-409d-aca3-2bca63581ecb)


*Nightscout Clock (or NSClock) is an open-source product aimed to help caregivers of people with type 1 diabetes have more piece of mind by being able to better glance at their loved onve blood glucose values.*

![resized_IMG_0217](https://github.com/user-attachments/assets/9c5d810a-76c0-414b-8d93-d46a6afa8bf6)

## Here is what it can do
* 6 colorful clockfaces
* Can get glucose data from Dexcom Share or Nightscout
* Supports mg/dl and mmol/l
* 10 minutes setup through web browser
* Configurable low/high limits
* Audible alarms in case the blood sugar is too low or too high
* Automatic brightness adjuistment
* Notifies of stall data
* ...and more

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

## More information for people who needs it

Nightscout CLock is a custom firmware for Ulanzi TC001. It can also run (with minor changes) on AWTRIX-Light custom hardware, so if you need bigger display, feel free to research.

### Clockfaces
| Name            | Look     |
|-----------------|----------|
| SImple          | ![resized_IMG_0225](https://github.com/user-attachments/assets/79cbda6d-5c0b-47fa-b5b4-2a5a10322a7d) |
| BIG DIGITS      | ![resized_IMG_0222](https://github.com/user-attachments/assets/59d5bea9-977b-4c40-b308-636d3d70055a) |
| 3-hours graph   | ![resized_IMG_0220](https://github.com/user-attachments/assets/86f36885-4479-412b-95fc-9fa527e12050) |
| Graph and value | ![resized_IMG_0221](https://github.com/user-attachments/assets/cb21ac92-a6d5-408c-b116-69726b58abc1) |
| Delta           | ![resized_IMG_0223](https://github.com/user-attachments/assets/dce1ecd4-a51b-4596-a292-0535c66f505c) |
| Time and value  | ![resized_IMG_0217](https://github.com/user-attachments/assets/d92832c2-8713-4ccf-9cc7-67202153d111) |

### Configuration web interface
![copped](https://github.com/user-attachments/assets/6a04b1f2-6c07-49ee-8c63-e145d3823ee9)


### Current version: 0.13

### Features (technical stuff, feel free to ignore)
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
