r"""Nightscout emulator for the clock's "API" data source.

Pushes sample glucose readings to the clock so that data processing and display
behavior can be tested without a real CGM.

Prerequisites
-------------
1. In the clock's Web UI, set the data source to "API" and let the clock restart.
   /api/v1/entries is only available with this data source.
2. Turn off web interface authentication, otherwise requests get a 401 response.
3. Install the dependencies: pip install requests requests_toolbelt
4. Put the clock's IP address in the git-ignored .local_clock_ip file in the
   repository root, or pass --url on each run.

Usage
-----
    python scripts/ns_emulator.py --one-value 110
    python scripts/ns_emulator.py --sin
    python scripts/ns_emulator.py --one-value 110 --no-delete
    python scripts/ns_emulator.py --url http://<clock-ip>/ --one-value 110

--url         the clock's address; overrides .local_clock_ip
--one-value   send a single reading with this value
--sin         send a sinusoid history of readings
--no-delete   keep the existing readings; by default they are deleted first

Values to exercise each glucose range with the default limits (mg/dL):
    50  -> urgent low
    110 -> in range
    300 -> urgent high

curl equivalent:
    curl -X DELETE http://<clock-ip>/api/v1/entries
    curl -X POST http://<clock-ip>/api/v1/entries \
         -H "Content-Type: application/json" \
         -d "[{\"sgv\":110,\"date\":$(date +%s)000,\"dateString\":\"$(date +%Y-%m-%dT%H:%M:%S)\",\"trend\":4}]"
"""
import json
import random
import requests
from datetime import datetime, timedelta
from requests_toolbelt.utils import dump
import math
import sys

from clock_address import resolve_clock_url

# read command arguments:
# --url [url]: the clock's address, instead of the one in .local_clock_ip
# --no-delete: do not delete the existing data
# --sin: to have sinusoid data
# --one-value [value]: to send one value

url_argument = None
if "--url" in sys.argv:
    if sys.argv.index("--url") + 1 >= len(sys.argv):
        print(__doc__)
        exit(2)
    url_argument = sys.argv[sys.argv.index("--url") + 1]
nightscout_url = resolve_clock_url(url_argument)
if nightscout_url is None:
    print(__doc__)
    print("ERROR: No clock address: create .local_clock_ip or pass --url.")
    exit(2)

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
