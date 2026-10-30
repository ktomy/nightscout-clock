"""Nightscout emulator for the clock's built-in "API" data source.

The clock can act as its own tiny Nightscout server: when its data source is set
to "API" it exposes POST/DELETE /api/v1/entries, so you can push fake glucose
readings to it for testing (e.g. to exercise the different clock faces / moods)
without a real CGM.

Prerequisites
-------------
1. In the clock's Web UI (http://<clock-ip>/), set Data source -> "API" and let
   it reboot. Any other source (Nightscout, Dexcom, LibreLinkUp, ...) does NOT
   register the /api/v1/entries route and requests will 404.
2. Point `nightscout_url` below at your clock's IP (keep the trailing slash).
3. Install deps:  pip install requests requests_toolbelt

Usage
-----
    python ns_emulator.py --one-value 110      # send a single reading (sgv=110)
    python ns_emulator.py --sin                # send a sinusoid history of readings
    python ns_emulator.py --one-value 110 --no-delete   # keep existing readings

By default the script first DELETEs all existing entries, then sends new ones.
Pass --no-delete to append instead of replacing.

Handy values (mg/dL, assuming default limits) to exercise the Smiley face moods:
    50  -> urgent low  -> sad / blue
    110 -> in range    -> happy / green
    300 -> urgent high -> angry / red

curl equivalent (no Python needed) -- replace the IP and epoch-millis date:
    # wipe existing readings
    curl -X DELETE http://192.168.86.24/api/v1/entries
    # send one reading of 110
    curl -X POST http://192.168.86.24/api/v1/entries \\
         -H "Content-Type: application/json" \\
         -d '[{"sgv":110,"date":1690000000000,"dateString":"2023-07-22T00:00:00","trend":4}]'
"""
import json
import random
import requests
from datetime import datetime, timedelta
from requests_toolbelt.utils import dump
import math
import sys

# The clock's IP address (with trailing slash). Find it in your router or on the
# clock's Web UI; it must be running with Data source set to "API".
nightscout_url = "http://192.168.86.24/"
# read command arguments:
# --no-delete: do not delete the existing data
# --sin: to have sinusoid data
# --one-value=[value]: to send one value

if "--no-delete" not in sys.argv:
    response = requests.delete(nightscout_url + "api/v1/entries", timeout=2)
    if response.status_code == 200:
        print("Data deleted successfully")
    else:
        print("Failed to send data")
        print(dump.dump_all(response).decode('utf-8'))
        exit(1)

if "--one-value" in sys.argv:
    value = int(sys.argv[sys.argv.index("--one-value") + 1])
    entry = {
        "dateString": datetime.now().strftime("%Y-%m-%dT%H:%M:%S"),
        "date": int(datetime.now().timestamp()) * 1000,
        "sgv": value,
        "trend": 4  # Assuming constant trend
    }
    print("sgv:", entry["sgv"])
    print("dateString:", entry["dateString"])

    # Send the entry to Nightscout server
    headers = {
        "Content-Type": "application/json"
    }
    response = requests.post(nightscout_url + "api/v1/entries", json=[entry], headers=headers, timeout=2)
    if response.status_code == 200:
        print("Data sent successfully")
    else:
        print("Failed to send data")
        print(dump.dump_all(response).decode('utf-8'))
    exit(0)

if "--sin" in sys.argv:
    since_count = 36 # generate since 36 * 5 minutes ago
    to_count = 0 # generate to 4 * 5 minutes ago
    mid_value = 130
    amplitude = 80 * 2
    change_speed = 30

    current_time = datetime.now() - timedelta(minutes=since_count * 5)
    for i in range(since_count, to_count, -1):
        value = int(mid_value + amplitude / 2 * math.sin(math.radians(i * change_speed)))
        date_string = current_time.strftime("%Y-%m-%dT%H:%M:%S")
        date_epoch = int(current_time.timestamp()) * 1000
        entry = {
            "dateString": date_string,
            "date": date_epoch,
            "sgv": value,
            "trend": 4  # Assuming constant trend
        }
        current_time += timedelta(minutes=5)
        print("sgv:", entry["sgv"])
        print("dateString:", entry["dateString"])

        # Send the entry to Nightscout server
        headers = {
            "Content-Type": "application/json"
        }
        response = requests.post(nightscout_url + "api/v1/entries", json=[entry], headers=headers, timeout=2)
        if response.status_code == 200:
            print("Data sent successfully")
        else:
            print("Failed to send data")
            print(dump.dump_all(response).decode('utf-8'))
