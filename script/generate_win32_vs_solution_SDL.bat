@echo off

cd ..
REM Get project directory name
for %%a in (.) do set projectDir=%%~na

cd ..
mkdir build-jsm-win32-sdl
cd build-jsm-win32-sdl
cmake ../%projectDir% -A Win32 -DBUILD_SHARED_LIBS=1 -DSDL=1

echo Open the generated Visual Studio solution located at: %cd%\JoyShockMapper.sln

pause
