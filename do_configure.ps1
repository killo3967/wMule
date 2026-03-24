$ErrorActionPreference = 'Continue'

$msvcLib = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64"
$ucrtLib = "C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\ucrt\x64"
$umLib = "C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\um\x64"
$msvcBin = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
$vcAux = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build"
$sdkBin = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"

$env:Path = "$msvcBin;$vcAux;$sdkBin;$env:Path"
$env:LIB = "$msvcLib;$ucrtLib;$umLib"

Write-Host "Environment configured"

Set-Location K:\amule\build
Remove-Item -Force -Recurse -ErrorAction SilentlyContinue * 

Write-Host "Running cmake..."

# Write CMake command to batch file to avoid quoting issues
$batContent = @"
cmake -S K:\amule -B K:\amule\build -G "NMake Makefiles" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DBUILD_MONOLITHIC=ON -DBUILD_DAEMON=OFF -DBUILD_REMOTEGUI=OFF -DBUILD_AMULECMD=ON -DBUILD_TESTING=ON -DENABLE_UPNP=OFF -DENABLE_IP2COUNTRY=OFF -DENABLE_NLS=OFF -DCRYPTOPP_INCLUDE_DIR="C:\vcpkg\installed\x64-windows\include\cryptopp" -DCRYPTOPP_LIB_SEARCH_PATH="C:\vcpkg\installed\x64-windows\lib" -DCMAKE_BUILD_TYPE=Release
"@

Set-Content -Path "K:\amule\run_cmake.bat" -Value $batContent -Encoding ASCII
& "K:\amule\run_cmake.bat"

Write-Host "CMake exit code: $LASTEXITCODE"
