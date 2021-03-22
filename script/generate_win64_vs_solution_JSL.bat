@echo off

cd ..
REM Get project directory name
for %%a in (.) do set projectDir=%%~na

cd ..
mkdir build-jsm-win64-jsl
cd build-jsm-win64-jsl
cmake ../%projectDir% -A x64 -DBUILD_SHARED_LIBS=1

echo Open the generated Visual Studio solution located at: %cd%\JoyShockMapper.sln

pause
