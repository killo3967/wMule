<#
.SYNOPSIS
    Sincroniza la biblioteca de conocimientos (Knowledge) clonando/procesando repositorios de skills vía Repomix.
    
.DESCRIPTION
    Este script descarga via archive los repositorios de GitHub especificados, genera un archivo consolidado en MD
    y luego reconstruye el SKILL_INDEX.md para que el Orquestador pueda navegar la biblioteca.
#>

$KnowledgeDir = "$PSScriptRoot\..\docs\knowledge"
$IndexFile = "$KnowledgeDir\SKILL_INDEX.md"

$RepoMap = @{
    "https://github.com/anthropics/skills"                  = "anthopics-skills.md"
    "https://github.com/sickn33/antigravity-awesome-skills" = "antigravity-skills.md"
    "https://github.com/bobmatnyc/claude-mpm-skills"        = "claude-skills.md"
    "https://github.com/openai/skills"                      = "openai-skills.md"
    "https://github.com/VoltAgent/awesome-agent-skills"     = "voltagent-skills.md"
}

if (-not (Test-Path $KnowledgeDir)) {
    New-Item -ItemType Directory -Path $KnowledgeDir -Force | Out-Null
}

Write-Host "`n[1/2] Sincronizando Repositorios via Repomix...`n" -ForegroundColor Cyan

foreach ($Repo in $RepoMap.GetEnumerator()) {
    $Url = $Repo.Key
    $OutputFile = Join-Path $KnowledgeDir $Repo.Value
    
    Write-Host "Procesando: $Url -> $($Repo.Value)" -ForegroundColor Gray
    
    # Ejecuta Repomix directamente sobre la URL
    # --no-security-check para acelerar el proceso
    # --include restringe a solo los archivos SKILL.md para ahorrar tokens
    npx repomix $Url --output $OutputFile --no-security-check --include "**/*SKILL.md"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [OK] Guardado en $($Repo.Value)" -ForegroundColor Green
    }
    else {
        Write-Host "  [FALLO] Error al procesar $Url" -ForegroundColor Red
    }
}

Write-Host "`n[2/2] Regenerando SKILL_INDEX.md...`n" -ForegroundColor Cyan

$Data = New-Object System.Collections.Generic.List[PSObject]

Get-ChildItem -Path $KnowledgeDir -Filter "*.md" | Where-Object { $_.Name -ne "SKILL_INDEX.md" } | ForEach-Object {
    $CurrentFile = $_.Name
    $FullPath = $_.FullName
    
    # Patrón compatible con el formato de Repomix: <file path="xxxx/SKILL.md">
    Select-String -Path $FullPath -Pattern '<file path=".*SKILL\.md">' -Context 0, 15 | ForEach-Object {
        $RawLine = $_.Line 
        $SkillName = ""
        
        # Extrae la ruta del atributo path
        $FilePathAttr = ""
        if ($RawLine -match 'path="([^"]+)"') {
            $FilePathAttr = $Matches[1]
        }

        # Busca la línea de 'name:' en el contexto posterior (el contenido del archivo)
        foreach ($ContextLine in $_.Context.PostContext) {
            if ($ContextLine -match "name:\s*(.*)") {
                $SkillName = $Matches[1].Trim().Replace('"', '').Replace("'", "")
                # Limpiar posibles restos de markdown
                $SkillName = $SkillName -replace '^[-]+', ''
                break
            }
        }
        
        if (-not [string]::IsNullOrWhiteSpace($SkillName)) {
            $Data.Add([PSCustomObject]@{
                    Name   = $SkillName
                    File   = $CurrentFile
                    Header = $RawLine.Trim()
                    Path   = $FilePathAttr
                })
        }
    }
}

# Generar el Markdown del Índice
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("# INDEX: AGENT SKILLS LIBRARY")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("Este indice se genera automaticamente. Permite al Orquestador localizar capacidades rapido.")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("| Skill Name | Source File | Internal Path |")
[void]$sb.AppendLine("| :--- | :--- | :--- |")

foreach ($row in ($Data | Sort-Object Name)) {
    [void]$sb.AppendLine("| **$($row.Name)** | $($row.File) | ``$($row.Path)`` |")
}

$sb.ToString() | Out-File -FilePath $IndexFile -Encoding utf8

Write-Host "  [OK] Indice regenerado con $($Data.Count) skills." -ForegroundColor Green
Write-Host "`nSincronizacion finalizada correctamente." -ForegroundColor Cyan
