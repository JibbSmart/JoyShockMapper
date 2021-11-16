@echo off

cd ..
rmdir /S /Q  build-jsm-win64-jsl
mkdir build-jsm-win64-jsl
cmake . -B build-jsm-win64-jsl -A x64 -DBUILD_SHARED_LIBS=1 -DSDL=0

echo Open the generated Visual Studio solution located at: build-jsm-win64-jsl\JoyShockMapper.sln

pause
