@echo off

echo -- Building for 32bit Windows
mkdir JSM_x86
cd JSM_x86
cmake ../.. -G "Visual Studio 16 2019" -A Win32 -DBUILD_SHARED_LIBS=1 -DCMAKE_INSTALL_PREFIX=d

cmake --build . --config Release
cmake --install .

cd d
rename bin JSM
mkdir JSM\Autoload
cd ../..
powershell.exe "Compress-Archive JSM_x86/d/JSM/ JSM_x86.zip"

rd /Q /S JSM_x86

echo -- Building for 64bit Windows
mkdir JSM_x64
cd JSM_x64
cmake ../.. -G "Visual Studio 16 2019" -A x64 -DBUILD_SHARED_LIBS=1 -DCMAKE_INSTALL_PREFIX=d

cmake --build . --config Release
cmake --install .

cd d
rename bin JSM
mkdir JSM\Autoload
cd ../..
powershell.exe "Compress-Archive JSM_x64/d/JSM/ JSM_x64.zip"

rd /Q /S JSM_x64

echo Done!
pause