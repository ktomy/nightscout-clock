import json
import random
import requests
from datetime import datetime, timedelta
from requests_toolbelt.utils import dump
import math

nightscout_url = "http://192.168.86.24/"

response = requests.delete(nightscout_url + "api/v1/entries", timeout=2)
if response.status_code == 200:
    print("Data deleted successfully")
else:
    print("Failed to send data")
    print(dump.dump_all(response).decode('utf-8'))
    exit(1)

since_count = 36
current_time = datetime.now() - timedelta(minutes=since_count * 5)
for i in range(since_count, 0, -1):
    value = int(150 + 50 * math.sin(math.radians(i * 20)))
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
