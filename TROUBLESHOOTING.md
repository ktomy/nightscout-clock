# Troubleshooting

First, as you are reading this page, it means something did not went well. Yes, sometimes things just don't go as expected. But don't worry, we'll find out what is wrong and will make the clock work as it should. After all, there are a few hundred people for whom the system works well. Here I collected a few tips on how to deal with issues which can appear during installation, configuration and operation of the clock.

## Installation

### Where is the installation page?

If you don't know how to install the firmware, the installation page is [here](https://ktomy.github.io/nightscout-clock/)

### Connection issues

When clicking "CONNECT" you should see a window having "USB Serial" listed.
| Right | Wrong |
|-------|-------|
| <img width="501" height="487" alt="image" src="https://github.com/user-attachments/assets/776ab9a1-a35d-4837-b5a7-bce5ea4d2684" /> | <img width="478" height="483" alt="image" src="https://github.com/user-attachments/assets/e6edb2cd-6ab0-4802-9419-4f68c372cf38" /> |
| <img width="962" height="991" alt="image" src="https://github.com/user-attachments/assets/feb29015-8338-4c7b-b675-884b2deb536e" /> | <img width="964" height="964" alt="image" src="https://github.com/user-attachments/assets/f18fdbf6-2056-4033-a857-da3681e23d28" />
 |


If you can't see the "USB Serial" in the list, there may be a few reasons:
- Cable between the clock and the computer is not properly connected or the cable is broken. This happens sometimes. Just try a different cable and make sure it is connected to a USB port of your computer. There is an LED on the clock near the USB connector, it should be either green or red, both colors are good.
- Clock is not turned on. If the display is black and the LED near the USB connector is turned on, simultaneously press `<` and `>` buttons on top of the clock for 2-3 seconds, the clock shouds start and emit a beeping sound for a second or two. Don't worry if the clocks beeps continuously, it just means there is no firmware programmed
- No driver. Sometimes, for some (older) operating systems you'll need a driver. Just google `ESP32 Driver for <your opating system>`

### Errors during programming



## Configuration

## Operation

## If nothing helps
