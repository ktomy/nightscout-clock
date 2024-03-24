@echo off
:retry
    REM Upload firmware
    C:\Users\User\.platformio\penv\Scripts\python.exe "C:\Users\User\.platformio\packages\tool-esptoolpy\esptool.py" ^
        --chip esp32 --port "COM6" --baud 921600 --before default_reset --after hard_reset write_flash -z ^
        --flash_mode dio --flash_freq 40m --flash_size 4MB ^
        0x1000 C:\Users\User\Documents\PlatformIO\Projects\NSClock\.pio\build\ulanzi_debug\bootloader.bin ^
        0x8000 C:\Users\User\Documents\PlatformIO\Projects\NSClock\.pio\build\ulanzi_debug\partitions.bin ^
        0xe000 C:\Users\User\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin ^
        0x10000 C:\Users\User\Documents\PlatformIO\Projects\NSClock\.pio\build\ulanzi_debug\firmware.bin ^
        0x210000 C:\Users\User\Documents\PlatformIO\Projects\NSClock\.pio\build\ulanzi_debug\littlefs.bin
REM Check result
if errorlevel 1 goto retry
REM Upload success
echo "Upload success"
