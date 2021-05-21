@echo off

cd ..
RMDIR /S /Q build-jsm-win32-sdl
mkdir build-jsm-win32-sdl
cmake . -B build-jsm-win32-sdl -A Win32 -DBUILD_SHARED_LIBS=1 -DSDL=1

echo Open the generated Visual Studio solution located at: build-jsm-win32-sdl\JoyShockMapper.sln

pause
