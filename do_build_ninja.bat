@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist K:\wMule\build-ninja mkdir K:\wMule\build-ninja
cd /d K:\wMule\build-ninja
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_UPNP=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
