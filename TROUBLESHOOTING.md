# Troubleshooting

If you are reading this page, something did not go as expected. Don't worry: most problems people report turn out to be a USB port, a cable, or the WiFi network, and can be fixed without any special tools. Below are the issues reported most often in [issues](https://github.com/ktomy/nightscout-clock/issues?q=is%3Aissue) and [discussions](https://github.com/ktomy/nightscout-clock/discussions), with what helped.

- [Installation](#installation)
- [What the clock shows when it starts](#what-the-clock-shows-when-it-starts)
- [WiFi problems](#wifi-problems)
- [Data source errors](#data-source-errors)
- [Restarts, freezes and other intermittent problems](#restarts-freezes-and-other-intermittent-problems)
- [Collecting logs with the debug firmware](#collecting-logs-with-the-debug-firmware)
- [Reporting an issue](#reporting-an-issue)

## Installation

### Where is the installation page?

The installation page is [here](https://ktomy.github.io/nightscout-clock/). It uses Web Serial, so open it in **Chrome** or **Edge** on a computer. Firefox, Safari and phone browsers cannot talk to the clock over USB.

### The clock does not appear in the list

When clicking **CONNECT** you should see a window with "USB Serial" (or a CH340 device) listed.

| Right | Wrong |
|-------|-------|
| <img width="501" height="487" alt="Port list showing USB Serial" src="https://github.com/user-attachments/assets/776ab9a1-a35d-4837-b5a7-bce5ea4d2684" /> | <img width="478" height="483" alt="Port list without USB Serial" src="https://github.com/user-attachments/assets/e6edb2cd-6ab0-4802-9419-4f68c372cf38" /> |
| <img width="962" height="991" alt="Device manager showing the clock's serial port" src="https://github.com/user-attachments/assets/feb29015-8338-4c7b-b675-884b2deb536e" /> | <img width="964" height="964" alt="Device manager without the clock's serial port" src="https://github.com/user-attachments/assets/f18fdbf6-2056-4033-a857-da3681e23d28" /> |

If you can't see "USB Serial" in the list, there may be a few reasons:

- **The cable.** Some USB cables only carry power, and some are simply broken. Try a different cable, preferably a short one, and make sure it is plugged in firmly at both ends. The LED near the clock's USB connector should be green or red; both are fine.
- **The clock is off.** If the display is black and the LED near the USB connector is on, press the `<` and `>` buttons on top of the clock together for 2-3 seconds. The clock should start and beep for a second or two. A continuous beep with a black screen just means no firmware is installed yet.
- **A missing driver.** The clock uses a CH340 USB-serial chip. Recent Windows, macOS and Linux versions usually include a driver, but some (often older) systems need one. Windows users can get it from the [manufacturer's download page](https://www.wch-ic.com/downloads/CH341SER_EXE.html), which also works on Windows on ARM.
- **Linux permissions.** If the port is listed but opening it fails with `Failed to open serial port`, your user probably has no access to `/dev/ttyUSB*`. Add yourself to the `dialout` group (or `uucp` on some distributions) and log in again.

### "Failed to initialize" or the installation stops halfway

This is the most common installation problem: the clock gives a short beep, and the page shows _Failed to initialize. Try resetting your device or holding the BOOT button while clicking INSTALL._

It is caused by an unreliable USB connection between the clock and the computer, which is very sensitive to the USB port and the computer it is plugged into. What helps, roughly in order:

1. **Simply retry.** Click **INSTALL** again, 5-10 times if needed. Some people also succeeded after first choosing the reset option in the installer's menu, and then **INSTALL**.
2. **Use a different USB port.** Try every port on your computer: some people found that only one or two of their ports worked.
3. **Use a USB 2.0 port.** USB 3.x and newer ports (often blue, or marked SS) are the most frequent cause. If your computer only has newer ports, plug the clock into a cheap USB 2.0 hub, which has fixed the problem for several people.
4. **Try a different cable.** Prefer a short USB-A to USB-C cable, and avoid long cables and extensions.
5. **Try a different computer.** For many people, the installation worked right away on another computer, often an older one.

Holding the clock's buttons while installing does not affect the installation, so you can ignore the BOOT button hint. More reports and tips are collected in the discussion [Are you unable to install? Read this](https://github.com/ktomy/nightscout-clock/discussions/57); please add your experience there.

If nothing works, the last resort is to set up the [development environment](CONTRIBUTING.md) and flash the clock with PlatformIO, whose `scripts/upload.sh --all` retries automatically.

### The screen is black and the clock beeps continuously

This means the clock has no working firmware, typically because an installation was interrupted. It is not broken: connect it and install again, following the tips above.

## What the clock shows when it starts

Knowing the normal startup sequence helps you tell where something goes wrong:

1. `V 1.x.x` in rainbow colors: the firmware version. Holding the middle button at this moment resets the clock to factory settings.
2. While connecting to WiFi, the screen waits. If the connection fails, the clock starts its own WiFi network and scrolls `Join nsclock Wi-fi network and go to http://192.168.4.1/` (with your custom hostname instead of `nsclock`, if you set one).
3. `Nightscout clock | To configure go to http://<address>/`: the clock is on your network. Note the address; it is where you'll find the settings page.
4. `Connect`, then `To API`: the clock is fetching glucose data from your data source.
5. Your clock face.

If the clock gets stuck at `To API`, see [Data source errors](#data-source-errors). If it never gets past `V 1.x.x`, reinstall the firmware; if that keeps happening, [collect logs](#collecting-logs-with-the-debug-firmware).

### I can't open the settings page

- Use the address shown at startup (step 3 above), and type `http://` in front of it. The clock has no `https://` page.
- Your phone or computer must be on the same WiFi network as the clock.
- During the first setup, connect to the clock's own `nsclock` WiFi network and open `http://192.168.4.1/`.

## WiFi problems

If the clock keeps going back to `Join nsclock Wi-fi network...` after you saved your settings, it could not connect to your WiFi. Check that:

- **The network is 2.4 GHz.** The clock does not support 5 GHz or 6 GHz networks. If your router uses one network name for all bands, it usually still works; if not, ask whether a 2.4 GHz-only network can be set up.
- **The security is WPA2.** WPA2-Personal (WPA2-PSK) and WPA2-Enterprise are supported. Old WPA (without the 2) networks may fail to connect.
- **The network name and password are exactly right**, including uppercase and lowercase letters.
- **The signal is strong enough.** Try placing the clock closer to the router. A weak signal can also cause late or missing readings.
- **The network has no captive portal.** Networks where you have to log in or accept terms on a web page (hotels, many schools and offices) are not supported. Ask the network administrator to let the clock through as an IoT device. If the network only lets known devices in, give the administrator the clock's MAC address, or enter another device's allowed MAC address in **Custom MAC address** on the **WiFi & system** tab.
- **Phone hotspots** work if the hotspot is in 2.4 GHz mode: on iPhone, turn on **Maximize Compatibility** in the Personal Hotspot settings.

If you use the clock in more than one place (for example at home and in the car), you can set up an **Additional WiFi network** on the **WiFi & system** tab. The clock tries it when the main network can't be joined.

## Data source errors

The status bar at the top of the settings page shows whether the clock has **WiFi**, **Internet**, and a working **Source**. Check it first: it tells you whether the problem is the network or the data source.

Short failures are retried quietly. When the clock can't get data at all, it scrolls an error message and restarts after a couple of minutes to try again.

### "Error connecting to Nightscout: -1" (or Dexcom server: -1)

`-1` means the clock could not open a connection to the data source at all: the connection timed out, was refused, or the server's name could not be found. This is almost always a network problem, not a problem with your account or the clock:

- **Check the WiFi.** Look at the startup sequence and the **Internet** status on the settings page. Move the clock closer to the router, and restart the router if other devices also have problems.
- **Check the address.** Open your Nightscout URL in a browser on a phone connected to the **same WiFi** as the clock. If it doesn't open there, the clock can't reach it either.
- **Check filters and firewalls.** DNS filters such as Pi-hole, parental controls, and school or office firewalls can block the clock. For Dexcom, the clock needs `share1.dexcom.com` (US) or `shareous1.dexcom.com` (outside the US) on port 443.
- **Self-hosted Nightscout:** make sure the server accepts connections from other devices on your network, not only from the computer it runs on.

If everything looks fine and the error persists, [collect logs](#collecting-logs-with-the-debug-firmware): they show the exact reason, for example `DNS Failed` or `Connection reset by peer`.

Other numbers are HTTP status codes from the server. For example, `301` usually means a wrong address or protocol (`http` instead of `https`), and `401` or `403` means the API secret or credentials are wrong.

### "NO DATA" or no new readings

- **Dexcom:** enter the username and password of the Dexcom account that runs the sensor (the one used in the Dexcom app), not a follower's account. Sharing must be turned on in the Dexcom app, with at least one follower. Choose the right server: US, or outside the US.
- **LibreLinkUp:** if the clock says `LibreLinkUp requires action in the LibreLinkUp app`, open the LibreLinkUp app and accept the new terms or confirm the requested step, then restart the clock.
- **Any source:** check that the data reaches other followers (a follower app, or your Nightscout site). The clock can only show what the data source has.

Occasional messages such as `Invalid LibreLinkUp response: EmptyInput` that go away by themselves within a minute or two are caused by short outages of the data source.

### The display looks dark or turns off at night

When readings get old, the value is drawn in the **old-data color**, which is gray by default. Gray can be invisible at the lowest brightness, so with automatic brightness in a dark room the clock can look switched off. Choose cyan, magenta or blue under **When data is old** on the **Display** tab. If short gaps in your data are normal, a **Custom no data timer** of more minutes keeps the readings in their normal colors longer.

## Restarts, freezes and other intermittent problems

Apart from saving settings, the clock restarts by itself only after an error, which it scrolls on the display first. If it restarts unexpectedly, freezes, loses its settings, or shows `Error loading software, please reinstall` without an obvious reason:

1. Make sure you run the [latest version](https://ktomy.github.io/nightscout-clock/), since many such problems have been fixed over time.
2. Try a different power supply and cable. A weak USB charger can cause random restarts.
3. If possible, try another WiFi network (for example your phone's hotspot) and another data source. This tells us whether the problem follows the network, the data source or the clock itself. Occasionally the clock's hardware is faulty.
4. If the problem persists, [collect logs with the debug firmware](#collecting-logs-with-the-debug-firmware) and [report an issue](#reporting-an-issue). Without logs, intermittent problems are very hard to find.

## Collecting logs with the debug firmware

The regular firmware writes very little diagnostic output. The debug firmware works the same way but logs in detail what the clock is doing, which is what's needed to find the cause of most problems.

### 1. Back up your settings

Installing a different firmware resets the clock's settings. On the **WiFi & system** tab, use **Backup and restore** > **Download settings** first, so you can load the file again afterwards.

### 2. Install the debug firmware

Open the [debug installation page](https://ktomy.github.io/nightscout-clock/debug/) and install it the same way as the regular firmware. Then set up the clock again, or load your backup on the **WiFi & system** tab (check **Also load the WiFi settings**) and select **Save and restart**. Keep using the clock as usual until the problem happens again.

### 3. Get the logs

There are two ways to get them. Use the first one for restarts and problems during startup.

**With a computer connected (recommended).** This records everything, including what happens just before a restart:

1. Connect the clock to the computer and open the [debug installation page](https://ktomy.github.io/nightscout-clock/debug/) in Chrome or Edge.
2. Click **CONNECT**, select the clock, and choose **Logs & Console** instead of **Install**.
3. Click **Reset Device** to restart the clock, so the log starts from the beginning.
4. Wait until the problem happens. For intermittent problems, keep the page open for as long as needed, even overnight; make sure the computer doesn't go to sleep.
5. Click **Download Logs** to save the log file.

**From the settings page, without a computer.** On the **WiFi & system** tab, the **Debug logs** card has a **Download logs** button. It saves the most recent part of the log (about 16 KB) that the clock keeps in memory. That memory is cleared when the clock restarts, so download it as soon as you notice the problem, and before restarting the clock.

### 4. Remove private data

Logs can contain your WiFi and data source passwords, API secrets, session tokens and glucose readings. Open the file in a text editor and replace those with `***` before sharing it.

### 5. Go back to the regular firmware

When you're done, install the regular firmware again from the [installation page](https://ktomy.github.io/nightscout-clock/) and restore your settings backup.

## Reporting an issue

If this page did not help, please [create an issue](https://github.com/ktomy/nightscout-clock/issues/new/choose) and include:

- the firmware version (shown at startup and on the **WiFi & system** tab) and your data source (Nightscout, Dexcom, LibreLinkUp, Medtrum, ...)
- what the clock shows, ideally with a photo or short video, and when the problem happens
- what you already tried from this page
- the **log file**, attached to the issue (drag it into the comment box), with private data removed as described [above](#4-remove-private-data)

For questions and installation problems, you can also search and ask in [discussions](https://github.com/ktomy/nightscout-clock/discussions).
