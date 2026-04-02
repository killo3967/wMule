<#!
.SYNOPSIS
    Compila los catálogos .po en .mo y los sincroniza con la carpeta assets/locale.

.DESCRIPTION
    Utiliza el script Python `po2mo.py` incluido en este directorio para convertir
    cada archivo .po bajo `po/` en un `amule.mo`. Los resultados se guardan en
    `assets/locale/<lang>/LC_MESSAGES/amule.mo`. Opcionalmente copia los catálogos
    al directorio de build para que `wmule.exe`/`wmulecmd.exe` puedan usarlos sin pasos
    manuales.

.PARAMETER ProjectRoot
    Ruta raíz del proyecto. Por defecto es el padre del directorio del script.

.PARAMETER BuildDir
    Directorio de build (default: <ProjectRoot>/build). Solo se usa si -CopyToBuild está presente.

.PARAMETER Configs
    Lista de configuraciones (Debug, Release, etc.) a sincronizar dentro del directorio de build.

.PARAMETER PythonExe
    Ejecutable de Python a utilizar. Por defecto "python".

.PARAMETER CopyToBuild
    Si se especifica, los catálogos compilados bajo assets/locale se copian a cada
    `build/src/<Config>/locale` existente.
#>
[CmdletBinding()]
param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$BuildDir = "",
    [string[]]$Configs = @("Debug", "Release"),
    [string]$PythonExe = "python",
    [string[]]$Domains = @("wmule", "amule"),
    [switch]$CopyToBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $BuildDir) {
    $BuildDir = Join-Path $ProjectRoot "build"
}

$poDir = Join-Path $ProjectRoot "po"
if (-not (Test-Path $poDir)) {
    throw "No se encontró el directorio de traducciones: $poDir"
}

$assetsLocaleDir = Join-Path $ProjectRoot "assets\locale"
if (-not (Test-Path $assetsLocaleDir)) {
    New-Item -Path $assetsLocaleDir -ItemType Directory | Out-Null
}

$compilerScript = Join-Path $PSScriptRoot "po2mo.py"
if (-not (Test-Path $compilerScript)) {
    throw "No se encontró el script de compilación: $compilerScript"
}

$poFiles = Get-ChildItem -Path $poDir -Filter '*.po' | Sort-Object Name
if ($poFiles.Count -eq 0) {
    Write-Warning "No se encontraron archivos .po en $poDir"
    return
}

$uniqueDomains = $Domains | Where-Object { $_ } | Select-Object -Unique
if ($uniqueDomains.Count -eq 0) {
    throw "Debe especificarse al menos un dominio"
}

$compiledLangs = [System.Collections.Generic.HashSet[string]]::new()
foreach ($po in $poFiles) {
    $lang = [System.IO.Path]::GetFileNameWithoutExtension($po.Name)
    $targetDir = Join-Path $assetsLocaleDir (Join-Path $lang 'LC_MESSAGES')
    if (-not (Test-Path $targetDir)) {
        New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
    }

    foreach ($domain in $uniqueDomains) {
        $targetFile = Join-Path $targetDir ("{0}.mo" -f $domain)
        Write-Host "[i18n] Compilando $($po.Name) -> $targetFile" -ForegroundColor Cyan
        & $PythonExe $compilerScript $po.FullName $targetFile
        if ($LASTEXITCODE -ne 0) {
            throw "Falló la compilación de $($po.Name) para el dominio $domain"
        }
    }

    $compiledLangs.Add($lang) | Out-Null
}

Write-Host "[i18n] Catálogos compilados para $($compiledLangs.Count) idiomas. Dominios: $($uniqueDomains -join ', ')" -ForegroundColor Green

if ($CopyToBuild) {
    foreach ($cfg in $Configs) {
        $localeTarget = Join-Path $BuildDir "src\$cfg\locale"
        if (-not (Test-Path $localeTarget)) {
            $parent = Split-Path $localeTarget -Parent
            if (Test-Path $parent) {
                New-Item -Path $localeTarget -ItemType Directory -Force | Out-Null
            } else {
                continue
            }
        }
        Write-Host "[i18n] Sincronizando catálogos hacia $localeTarget" -ForegroundColor Yellow
        Copy-Item -Path (Join-Path $assetsLocaleDir '*') -Destination $localeTarget -Recurse -Force
    }
}
