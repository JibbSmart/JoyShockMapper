@echo off

REM build both version
cmake --build ../build-jsm-win64-jsl/ --config Release
cmake --build ../build-jsm-win64-sdl/ --config Release

REM install both versions
cmake --install ../build-jsm-win64-jsl/ --config Release --prefix ../install
cmake --install ../build-jsm-win64-sdl/ --config Release --prefix ../install

REM Zip both versions
Del /Q ..\JoyShockMapper_x64.zip ..\JoyShockMapper_x64_legacy.zip
PowerShell -command  "Compress-Archive -Path ../install/JoyShockMapper_x64 -DestinationPath ../JoyShockMapper_x64.zip"
PowerShell -command  "Compress-Archive -Path ../install/JoyShockMapper_x64_legacy -DestinationPath ../JoyShockMapper_x64_legacy.zip"

pause