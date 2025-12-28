import requests
from datetime import datetime, timezone, timedelta
import os

# read from environment variables
username = os.getenv('MEDTRUM_USERNAME')
password = os.getenv('MEDTRUM_PASSWORD')

header = {
    'DevInfo': 'Android 12;Xiamoi vayu;Android 12',
    'AppTag': 'v=1.2.70(112);n=eyfo;p=android',
    'User-Agent': 'okhttp/3.5.0'
}
url = 'https://easyview.medtrum.eu/mobile/ajax/login'
data = {
    'apptype': 'Follow',
    'user_name': username, 
    'password': password,
    'platform': 'google',
    'user_type': 'M',
}

r =requests.post(url, data=data, headers=header)

if (r.status_code != 200):
    print('Login failed')
    exit(1)

# error response samples:
# {"msg":"\u6ca1\u6709\u767b\u5f55","res":"ERR","resdetail":"MobResPasswordWrong"} # wrong password
# {"msg":"\u6ca1\u6709\u767b\u5f55","res":"ERR","resdetail":"MobResUsernameWrong"} # wrong username
if (r.json().get('res') != 'OK'):
    print('Login failed: ' + r.json().get('resdetail', 'Unknown error'))
    exit(1)

print('Login successful')

header['Cookie'] = r.headers['Set-Cookie']
url = 'https://easyview.medtrum.eu/mobile/ajax/monitor?flag=monitor_list'
r2 = requests.get(url, headers=header)

if (r2.status_code != 200):
    print('Error getting current glucose')
    exit(1)

if (r2.json().get('res') != 'OK'):
    print('Error getting current glucose: ' + r2.json().get('resdetail', 'Unknown error'))
    # actually we should try to re-login and retry here, but for simplicity just exit
    exit(1)

# current glucose level: monitorlist -> [0] -> sensor_status -> glucose
glucose = r2.json()['monitorlist'][0]['sensor_status']['glucose']
glucoseRate = r2.json()['monitorlist'][0]['sensor_status']['glucoseRate']
glucose_timestamp = r2.json()['monitorlist'][0]['sensor_status']['updateTime'] #unixtime in seconds
if (glucose == 0):
    print('No current sensor data')
units = "mgdl"

glucosemgdl = 0

if (glucose < 30):
    units = "mmol"
    glucosemgdl = int(glucose * 18)
else:
    glucosemgdl = glucose

trensString = ""
# 0, 8: flat
# 1:45-up
# 2:up
# 3:double-up
# 4:45-down
# 5:down
# 6:double-down
# 7:none

if (glucoseRate == 0 or glucoseRate == 8):
    trensString = "↔️"
elif (glucoseRate == 1):
    trensString = "↗️"
elif (glucoseRate == 2):
    trensString = "⬆️"
elif (glucoseRate == 3):
    trensString = "⏫"
elif (glucoseRate == 4):
    trensString = "↘️"
elif (glucoseRate == 5):
    trensString = "⬇️"
elif (glucoseRate == 6):
    trensString = "⏬"
else:
    trensString = "❓"


print(f'Current glucose level: {glucosemgdl} mg/dL ({glucose} {units}), Timestamp (UTC): {datetime.fromtimestamp(glucose_timestamp, timezone.utc)}, Trend: {trensString}')

lasthour = datetime.now(timezone.utc)-timedelta(hours=1)
now = datetime.now(timezone.utc)
et = now.strftime("%Y-%m-%d %H:%M:%S").replace(' ', '%20')
st = lasthour.strftime("%Y-%m-%d %H:%M:%S").replace(' ', '%20')

url = 'https://easyview.medtrum.eu/mobile/ajax/download?flag=sg&st='+st+'&et='+et+'&user_name=' + r2.json()['monitorlist'][0]['username']

r3 = requests.get(url, headers=header)

if (r3.status_code != 200):
    print('Error getting glucose history')
    exit(1)

if (r3.json().get('res') != 'OK'):
    print('Error getting glucose history: ' + r3.json().get('resdetail', 'Unknown error'))
    # actually we should try to re-login and retry here, but for simplicity just exit
    exit(1)

# example response:
# {
#     "data": [
#         [
#             "1234-1234567890-1-1234",
#             1766685642.0, # timestamp
#             12.51, # unknown
#             5.2, # glucose (depending on units)
#             "C", # unknown
#             7.0 # unknown
#         ],
#  ],
# }

for entry in r3.json().get('data', []):
    timestamp = datetime.fromtimestamp(entry[1], timezone.utc)
    glucose = entry[3]
    if (glucose < 30):
        glucosemgdl = int(glucose * 18)
        units = "mmol"
    else:
        glucosemgdl = glucose
        units = "mgdl"
    print(f'Time (UTC): {timestamp}, Glucose: {glucosemgdl} mg/dL ({glucose} {units})')


