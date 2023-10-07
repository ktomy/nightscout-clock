C:\Users\User\.platformio\penv\Scripts\python.exe "C:\Users\User\.platformio\packages\tool-esptoolpy\esptool.py" ^
    --chip esp32 --port "COM6" --baud 921600 --before default_reset --after hard_reset write_flash -z ^
    --flash_mode dio --flash_freq 40m --flash_size 4MB ^
    0x210000 C:\Users\User\Documents\PlatformIO\Projects\NSClock\.pio\build\ulanzi\littlefs.bin
