@echo off
cd ..
C:\Users\User\.platformio\penv\Scripts\platformio.exe run --target buildfs --environment ulanzi_debug
cd Scripts
echo "Build success"