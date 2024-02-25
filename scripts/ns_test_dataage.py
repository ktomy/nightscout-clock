import time
import requests

url = "https://ktomy.nsromania.info/api/v1/entries?count=1"
headers = {"Accept": "application/json"}

prev_unixtime = 0

while True:
    response = requests.get(url, headers=headers, timeout=3)
    if response.status_code == 200:
        data = response.json()
        if data:
            entry = data[0]
            date_unixtime = entry["date"] / 1000
            current_unixtime = time.time()
            difference_seconds = current_unixtime - date_unixtime
            if int(date_unixtime) != int(prev_unixtime):
                prev_unixtime = date_unixtime
                print("current_unixtime:", int(current_unixtime), "date_unixtime:", int(date_unixtime), "difference_seconds:", int(difference_seconds))
            
    time.sleep(1)
