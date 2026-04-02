#=============================================================================
# SCRIPT DE COMPILACIÓN PARA wMule (Windows Mule)
#=============================================================================
# Este script automatiza el proceso completo de configuración y compilación
# del proyecto wMule utilizando CMake y vcpkg en Windows 11 con MSVC 2022.
#
# Requisitos previos:
#   - vcpkg instalado en C:\vcpkg
#   - MSVC 2022 Build Tools instalado
#   - CMake instalado y disponible en PATH
#
# Uso:
#   .\build-wmule.ps1 [opción]
#
# Opciones:
#   -configurar   : Solo ejecuta la fase de configuración CMake
#   -compilar     : Solo ejecuta la fase de compilación (asume configuración previa)
#   -reconstruir  : Limpia, reconfigura y recompila desde cero
#   -limpiar      : Elimina el directorio de build
#   (sin opción)  : Ejecuta configuración + compilación (build completo)
#
# Salida:
#   El ejecutable wmule.exe se generará en:
#   K:\wMule\build\src\Debug\wmule.exe
#
#=============================================================================

#=============================================================================
# DECLARACIÓN DE VARIABLES DE ENTORNO Y CONFIGURACIÓN
#=============================================================================
# Todas las variables se declaran aquí al comienzo para facilitar su modificación
# y comprensión. Cada variable incluye un comentario explicando su propósito.

# Ruta raíz del proyecto wMule
# Debe apuntar al directorio donde se encuentra el código fuente de wMule
$ProjectRoot = "K:\wMule"

# Directorio donde se almacenarán los archivos intermedios de build
# Se recomienda mantenerlo fuera del código fuente para facilitar limpieza
$BuildDir = Join-Path $ProjectRoot "build"

# Tipo de configuración de build
# Debug: Incluye símbolos de depuración, desactiva optimizaciones
# Release: Optimiza para rendimiento, elimina símbolos de depuración
$BuildType = "Debug"

# Ruta al archivo de toolchain de vcpkg
# Este archivo permite a CMake encontrar las dependencias instaladas vía vcpkg
$VcpkgToolchain = "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Flag para habilitar/deshabilitar el soporte UPnP
# UPnP permite el descubrimiento automático de dispositivos en la red local
# Requiere la instalación previa de miniupnpc vía vcpkg.
$EnableUpnp = $true   # Cambiar a $false solo si se desea desactivarlo explícitamente

# Habilita la internacionalización gettext. Si se desactiva, el selector de idioma
# sigue visible pero los catálogos no se cargan en runtime.
$EnableNls = $true

# Flag para decidir si se construyen los tests MuleUnit; ON garantiza que `ctest`
# tenga todos los binarios disponibles
$BuildTesting = $true

# Valores que CMake entiende para las opciones booleanas
$EnableUpnpValue   = if ($EnableUpnp) { "ON" } else { "OFF" }
$EnableNlsValue    = if ($EnableNls) { "ON" } else { "OFF" }
$BuildTestingValue = if ($BuildTesting) { "ON" } else { "OFF" }

# Generador de CMake a utilizar
# Options: "Visual Studio 17 2022", "NMake Makefiles", "Ninja"
# Ninja es generalmente más rápido para builds incrementales
$CMakeGenerator = "Visual Studio 17 2022"

#=============================================================================
# GENERAR ARCHIVO DE LOG CON TIMESTAMP
#=============================================================================
# Genera un nombre de archivo de log con timestamp para capturar detalladamente
# la salida del proceso de build
$scriptDir = Split-Path -Parent $PSCommandPath
$logTimestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$logFile = Join-Path $scriptDir "build-wmule_$logTimestamp.log"

# Escribir encabezado al archivo de log
"=== wMule Build Log ===" | Out-File -FilePath $logFile -Encoding UTF8
"Timestamp: $logTimestamp" | Out-File -FilePath $logFile -Append -Encoding UTF8
"Script: $PSCommandPath" | Out-File -FilePath $logFile -Append -Encoding UTF8
"Directorio de trabajo: $PWD" | Out-File -FilePath $logFile -Append -Encoding UTF8
"Parámetros recibidos: $args" | Out-File -FilePath $logFile -Append -Encoding UTF8
"`n" | Out-File -FilePath $logFile -Append -Encoding UTF8

#=============================================================================
# FUNCIÓN: Configurar el entorno de MSVC y Windows SDK
#=============================================================================
function Setup-BuildEnvironment {
    Write-Host "[INFO] Configurando entorno de compilación MSVC..." -ForegroundColor Cyan
    
    # Rutas a las bibliotecas y herramientas de MSVC 2022
    # Estas variables se añaden al PATH y LIB para que las herramientas las encuentren
    $MsvcLib = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64"
    $UcrtLib = "C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\ucrt\x64"
    $UmLib   = "C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\um\x64"
    $MsvcBin = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
    $VcAux   = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build"
    $SdkBin  = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
    
    # Configurar variables de entorno para el proceso actual
    # PATH: Para que el sistema encuentre los ejecutables (cl.exe, link.exe, etc.)
    $env:Path = "$MsvcBin;$VcAux;$SdkBin;$env:Path"
    
    # LIB: Para que el enlazador encuentre las bibliotecas de MSVC y Windows SDK
    $env:LIB  = "$MsvcLib;$UcrtLib;$UmLib"
    
    Write-Host "[INFO] Entorno configurado correctamente." -ForegroundColor Green
}

#=============================================================================
# FUNCIÓN: Ejecutar la configuración CMake
#=============================================================================
function Run-CmakeConfiguration {
    Write-Host "[INFO] Iniciando configuración CMake..." -ForegroundColor Cyan
    
    # Asegurarse de que el directorio de build existe
    if (-not (Test-Path $BuildDir)) {
        Write-Host "[INFO] Creando directorio de build: $BuildDir" -ForegroundColor Yellow
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }
    
    # Cambiar al directorio de build
    Push-Location $BuildDir
    
    try {
        # Construir el comando CMake con todos los parámetros necesarios
        $cmakeArgs = @(
            ".."                                    # Directorio fuente (un nivel arriba)
            "-G"                                    # Indicamos el generador explícitamente
            $CMakeGenerator                         # Nombre del generador (sin comillas incrustadas)
            "-A"                                    # Plataforma objetivo
            "x64"
            "-DCMAKE_BUILD_TYPE=$BuildType"         # Tipo de build (Debug/Release)
            "-DCMAKE_TOOLCHAIN_FILE=$VcpkgToolchain" # Toolchain de vcpkg para dependencias
            "-DENABLE_UPNP=$EnableUpnpValue"        # Habilitar UPnP (ON/OFF)
            "-DENABLE_NLS=$EnableNlsValue"          # Habilitar i18n/gettext (ON/OFF)
            "-DBUILD_MONOLITHIC=ON"                 # Construir wmule.exe (GUI principal)
            "-DBUILD_AMULECMD=ON"                   # Construir wmulecmd.exe (interfaz CLI)
            "-DBUILD_TESTING=$BuildTestingValue"    # Construir pruebas unitarias
        )
        
        $cmakeTimestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
        $cmakeLogFile = Join-Path $scriptDir "cmake_$cmakeTimestamp.log"
        
        "=== CMake Configuration Log ===" | Out-File -FilePath $cmakeLogFile -Encoding UTF8
        "Timestamp: $cmakeTimestamp" | Out-File -FilePath $cmakeLogFile -Append -Encoding UTF8
        "Command: cmake $cmakeArgs" | Out-File -FilePath $cmakeLogFile -Append -Encoding UTF8
        "`n" | Out-File -FilePath $cmakeLogFile -Append -Encoding UTF8
        
        Write-Host "[DEBUG] Ejecutando: cmake $cmakeArgs" -ForegroundColor DarkGray
        
        & cmake @cmakeArgs *>&1 | Tee-Object -FilePath $cmakeLogFile
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[INFO] Configuración CMake completada exitosamente." -ForegroundColor Green
            "CMake configuration succeeded." | Out-File -FilePath $logFile -Append -Encoding UTF8
        } else {
            Write-Host "[ERROR] Falló la configuración CMake. Código de salida: $LASTEXITCODE" -ForegroundColor Red
            "CMake configuration FAILED (exit code $LASTEXITCODE)." | Out-File -FilePath $logFile -Append -Encoding UTF8
            throw "Configuración CMake fallida"
        }
    }
    finally {
        Pop-Location
    }
}

#=============================================================================
# FUNCIÓN: Ejecutar la compilación
#=============================================================================
function Run-Build {
    Write-Host "[INFO] Iniciando proceso de compilación..." -ForegroundColor Cyan
    
    # Cambiar al directorio de build
    Push-Location $BuildDir
    
    try {
        Write-Host "[DEBUG] Ejecutando: cmake --build . --config $BuildType" -ForegroundColor DarkGray
        & cmake --build . --config $BuildType

        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] Falló la compilación. Código de salida: $LASTEXITCODE" -ForegroundColor Red
            throw "Compilación fallida"
        }

        Write-Host "[INFO] Compilación completada exitosamente." -ForegroundColor Green

        # Mostrar ubicación de los ejecutables generados
        $wmuleExe = Join-Path $BuildDir "src\$BuildType\wmule.exe"
        $wmulecmdExe = Join-Path $BuildDir "src\$BuildType\wmulecmd.exe"

        if (Test-Path $wmuleExe) {
            Write-Host "[INFO] Ejecutable principal generado: $wmuleExe" -ForegroundColor Yellow
        }
        if (Test-Path $wmulecmdExe) {
            Write-Host "[INFO] Ejecutable CLI generado: $wmulecmdExe" -ForegroundColor Yellow
        }

        $translationScript = Join-Path $scriptDir "update-translations.ps1"
        if (Test-Path $translationScript) {
            try {
                Write-Host "[INFO] Actualizando catálogos de idioma (.mo)" -ForegroundColor Cyan
                & $translationScript -ProjectRoot $ProjectRoot -BuildDir $BuildDir -Configs @($BuildType) -CopyToBuild
                if ($LASTEXITCODE -ne 0) {
                    throw "update-translations.ps1 devolvió $LASTEXITCODE"
                }
            }
            catch {
                Write-Warning "No se pudieron sincronizar las traducciones: $($_.Exception.Message)"
            }
        }
    }
    finally {
        # Regresar al directorio original
        Pop-Location
    }
}

#=============================================================================
# FUNCIÓN: Limpiar el directorio de build
#=============================================================================
function Clean-BuildDir {
    Write-Host "[INFO] Limpiando directorio de build: $BuildDir" -ForegroundColor Yellow
    
    if (Test-Path $BuildDir) {
        # Eliminar recursivamente todo el contenido
        Remove-Item -Path $BuildDir -Recurse -Force
        Write-Host "[INFO] Directorio de build eliminado." -ForegroundColor Green
    } else {
        Write-Host "[INFO] El directorio de build no existe, nada que limpiar." -ForegroundColor DarkGray
    }
}

#=============================================================================
# FUNCIÓN: Mostrar ayuda/uso del script
#=============================================================================
function Show-Help {
    Write-Host @"
USO:
    .\build-wmule.ps1 [opción]

OPCIONES:
    -configurar   : Solo ejecuta la fase de configuración CMake
    -compilar     : Solo ejecuta la fase de compilación (asume configuración previa)
    -reconstruir  : Limpia, reconfigura y recompila desde cero
    -limpiar      : Elimina el directorio de build
    -help, /?     : Muestra esta ayuda
    (sin opción)  : Ejecuta configuración + compilación (build completo)

EJEMPLOS:
    .\build-wmule.ps1              # Build completo (configurar + compilar)
    .\build-wmule.ps1 -configurar  # Solo configurar
    .\build-wmule.ps1 -compilar    # Solo compilar (requiere configuración previa)
    .\build-wmule.ps1 -reconstruir # Build limpio desde cero
    .\build-wmule.ps1 -limpiar     # Eliminar archivos de build
"@ -ForegroundColor Cyan
}

#=============================================================================
# MAIN: Procesar argumentos y ejecutar acciones (con transcript)
#=============================================================================
$transcriptDir = Split-Path -Parent $PSCommandPath   # mismo directorio que el script
$transcriptTimestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$transcriptPath = Join-Path $transcriptDir "build-wmule_transcript_$transcriptTimestamp.log"

try {
    # Iniciar el transcript (captura TODO lo que aparece en la consola)
    Start-Transcript -Path $transcriptPath -Append -IncludeInvocationHeader

    # Escribimos una cabecera adicional para que quede claro en el log
    Write-Host "=== INICIO DEL BUILD ===" -ForegroundColor Cyan
    Write-Host "Timestamp: $transcriptTimestamp"
    Write-Host "Argumentos recibidos: $args"
    Write-Host ""

    # Configurar entorno
    Setup-BuildEnvironment

    # Obtener el primer argumento (si existe)
    $argument = if ($args.Count -gt 0) { $args[0].ToLower() } else { "" }

    switch ($argument) {
        "-configurar" {
            Run-CmakeConfiguration
            break
        }
        "-compilar" {
            Run-Build
            break
        }
        "-reconstruir" {
            Clean-BuildDir
            Run-CmakeConfiguration
            Run-Build
            break
        }
        "-limpiar" {
            Clean-BuildDir
            break
        }
        "-help" {
            Show-Help
            break
        }
        "/?" {
            Show-Help
            break
        }
        "" {  # Build completo por defecto
            Run-CmakeConfiguration
            Run-Build
            break
        }
        default {
            Write-Host "[ERROR] Argumento desconocido: $argument" -ForegroundColor Red
            Show-Help
            exit 1
        }
    }

    Write-Host "[INFO] Operación completada." -ForegroundColor Green
    exit 0
}
catch {
    Write-Host "[ERROR] Excepción no controlada: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
finally {
    # Asegurarnos de que el transcript se cierre aunque haya error
    Stop-Transcript
}
