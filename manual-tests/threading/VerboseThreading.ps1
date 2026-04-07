param(
    [string]$LogPath = "$env:APPDATA\wMule\logs\wMule.log",
    [int]$DrainTimeoutMs = 5000,
    [switch]$Watch
)

Write-Host "=== Diagnóstico VerboseThreading ===" -ForegroundColor Cyan
Write-Host "1. Abrí wMule y ve a Preferencias -> Tweaks -> Threading diagnostics." -ForegroundColor Yellow
Write-Host "2. Activá \"Enable verbose threading logging\" y ajustá \"Thread drain timeout (ms)\" a $DrainTimeoutMs." -ForegroundColor Yellow
Write-Host "3. Repite la prueba de apagado: iniciá descargas, luego cerrá wMule normalmente." -ForegroundColor Yellow
Write-Host "   Las entradas 'ShutdownSummary' aparecerán sólo si VerboseThreading está activo o DBG_THREAD está habilitado." -ForegroundColor Yellow
Write-Host "" 
Write-Host "Si necesitás automatizar el cambio sin GUI, editá manualmente preferences.ini (sección [Threading]) y reiniciá wMule." -ForegroundColor DarkYellow
Write-Host "La API EC actual no expone aún estos toggles: cualquier comando remoto mostrará un stub y no modificará el estado." -ForegroundColor DarkYellow

if ($Watch) {
    if (-Not (Test-Path -LiteralPath $LogPath)) {
        Write-Warning "No se encontró el log '$LogPath'. Ajustá el parámetro -LogPath." 
        return
    }

    Write-Host "Monitoreando '$LogPath' para entradas ShutdownSummary... (Ctrl+C para salir)" -ForegroundColor Green
    Get-Content -LiteralPath $LogPath -Wait |
        Where-Object { $_ -like '*ShutdownSummary*' } |
        ForEach-Object { Write-Host $_ -ForegroundColor Magenta }
} else {
    Write-Host "Sugerencia: ejecutá con -Watch para seguir el log en tiempo real." -ForegroundColor Cyan
}
