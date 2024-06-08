C:\Users\User\.platformio\penv\Scripts\python.exe "C:\Users\User\.platformio\packages\tool-esptoolpy\esptool.py" ^
    --chip esp32 --port "COM6" --baud 921600 ^
    read_flash 0x0000 0x3FFFFF flash.bin 
