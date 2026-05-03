#=============================================================================
# VALIDACIÓN LOCAL REPRODUCIBLE PARA wMule
#=============================================================================
# Ejecuta build Debug, CTest y un smoke básico de wmulecmd.exe.
# Compatible con PowerShell en Windows 11.
#=============================================================================

[CmdletBinding()]
param(
	[string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
	[string]$BuildDir = (Join-Path $RepoRoot "build"),
	[ValidateSet("Debug", "Release")]
	[string]$Configuration = "Debug",
	[switch]$SkipSmoke
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-CheckedStep {
	param(
		[string]$Name,
		[scriptblock]$Action
	)

	Write-Host "[INFO] $Name" -ForegroundColor Cyan
	& $Action
	if ($LASTEXITCODE -ne 0) {
		throw "$Name falló con código de salida $LASTEXITCODE"
	}
}

Write-Host "[INFO] Repositorio: $RepoRoot" -ForegroundColor Cyan
Write-Host "[INFO] BuildDir:    $BuildDir" -ForegroundColor Cyan
Write-Host "[INFO] Configuración: $Configuration" -ForegroundColor Cyan

if (-not (Test-Path $BuildDir)) {
	throw "No existe el directorio de build: $BuildDir"
}

Push-Location $BuildDir
try {
	Invoke-CheckedStep -Name "Compilando Debug" -Action { & cmake --build . --config $Configuration }
	Invoke-CheckedStep -Name "Ejecutando CTest" -Action { & ctest --output-on-failure -C $Configuration }

	if (-not $SkipSmoke) {
		$wmulecmdExe = Join-Path $BuildDir "src\$Configuration\wmulecmd.exe"
		if (-not (Test-Path $wmulecmdExe)) {
			throw "No existe el ejecutable de smoke: $wmulecmdExe"
		}

		Invoke-CheckedStep -Name "Smoke wmulecmd.exe -c help" -Action { & $wmulecmdExe -c help }
	}

	Write-Host "[OK] Validación local completada correctamente." -ForegroundColor Green
}
finally {
	Pop-Location
}
