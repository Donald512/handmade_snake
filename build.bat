@echo off 

echo ____________________________________________________________________
mkdir Build
pushd Build
cl -Zi -W4 ..\win32_handmade.c user32.lib gdi32.lib
popd
echo ____________________________________________________________________