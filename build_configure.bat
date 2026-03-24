@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

cd /d K:\amule\build

cmake -S K:\amule -B K:\amule\build -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DBUILD_MONOLITHIC=ON -DBUILD_AMULECMD=ON -DBUILD_TESTING=OFF -DENABLE_UPNP=OFF -DENABLE_IP2COUNTRY=OFF -DENABLE_NLS=OFF -DCRYPTOPP_INCLUDE_DIR="C:\vcpkg\installed\x64-windows\include\cryptopp" -DCRYPTOPP_LIB_SEARCH_PATH="C:\vcpkg\installed\x64-windows\lib" -DCMAKE_BUILD_TYPE=Release
