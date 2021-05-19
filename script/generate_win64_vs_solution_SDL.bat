@echo off

cd ..
rmdir /S /Q build-jsm-win64-sdl
mkdir build-jsm-win64-sdl
cmake . -A x64 -B build-jsm-win64-sdl -DBUILD_SHARED_LIBS=1 -DSDL=1

echo Open the generated Visual Studio solution located at: build-jsm-win64-sdl\JoyShockMapper.sln

pause
