@echo off

cd ..
RMDIR /S /Q build-jsm-win32-jsl
mkdir build-jsm-win32-jsl
cmake . -B build-jsm-win32-jsl -A Win32 -DBUILD_SHARED_LIBS=1 -DSDL=0

echo Open the generated Visual Studio solution located at: build-jsm-win32-jsl\JoyShockMapper.sln

pause
