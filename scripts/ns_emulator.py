import json
import random
import requests
from datetime import datetime, timedelta
from requests_toolbelt.utils import dump
import math
import sys

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
