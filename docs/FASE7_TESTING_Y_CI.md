# Fase 7 — Calidad, Tests y CI

## Estado inicial

- Fase 6 cerrada documentalmente.
- Fase 7 iniciada como prioridad activa.
- Línea base validada: Debug 28/28, `wmule.exe` arranca y `wmulecmd.exe -c help` responde.

## Fase 7.1

- Iniciada la primera tanda crítica sobre EC auth/handshake.
- Tests añadidos: `ECAuthHandshakeTest` (helpers `SecretHash`: derivación PBKDF2, formato, migración legacy y normalización de secretos).
- Deuda detectada: la validación semántica de `SecretHash` sigue siendo blanda en varios bordes; se ha caracterizado sin endurecer. No se ha cubierto el packet contract EC completo.

## Fase 7.2

- Iniciada la segunda tanda crítica sobre parsing hostil Kad/eD2K.
- Seam elegido: `KadPacketGuards` (`KadBytesAvailable`, `EnsureKadPayload`, `EnsureKadTagCount`).
- Tests ampliados en `KadPacketGuardsTest` para buffer vacío, buffer corto, payload exacto, payload vacío válido y límites de tags.
- Límite real: aún no hay seam limpio para packet-level Kad/eD2K completo sin tocar producción; esa cobertura queda pendiente y caracterizada como deuda.

## Fase 7.3

- Iniciada la tercera tanda crítica sobre rutas y configuración corrupta/peligrosa.
- Seam elegido: `Path.*` (`NormalizeAbsolutePath*`, `NormalizeSharedPath`, `NormalizePreferencePathForLoad`) + `ResolveInternalPathKind`.
- Tests añadidos en `PathConfigHardeningTest` para entrada vacía, relativa, traversal, UNC, separadores mixtos, ficheros en lugar de directorios y fallback de clave de configuración desconocida.
- Deuda detectada: la configuración real extremo a extremo sigue acoplada a `wxConfig`/globals; este lote ha caracterizado solo los helpers de normalización y la validación de contexto.

## Fase 7.4

- Evaluado el lote de concurrencia/shutdown: sí hay seam limpio en `PartFileAsyncGate` + `ThreadShutdownToken` + `ThreadMetrics`.
- Tests añadidos: `ThreadingShutdownTest` para cierre ordenado, rechazo tras shutdown y token de apagado.
- Límite real: `CThreadPool` sigue siendo singleton global y no es un buen candidato para tests deterministas sin tocar producción.

## Objetivo

Consolidar validación local reproducible, CI Windows inicial y cobertura incremental de tests sin tocar código productivo.

## Validación local

### Script

- `scripts/validate-local.ps1`

### Qué hace

- compila Debug;
- ejecuta CTest;
- lanza `wmulecmd.exe -c help` como smoke no destructivo.

### Uso

```powershell
Set-Location K:\wMule
.\scripts\validate-local.ps1
```

### Nota

El script ayuda a repetir la validación, pero no sustituye el criterio de cierre de fase.

## Estado del CI Windows

- Workflow inicial creado en `.github/workflows/windows-ci.yml`.
- Cobertura mínima: Windows + MSVC + vcpkg + build Debug + CTest + smoke CLI.
- No cubre todavía Release, análisis estático ni sanitizers.
- Aún no hay ejecución confirmada en GitHub Actions; la viabilidad se ha validado localmente con el mismo flujo.
- En esta sesión no se pudo consultar el estado remoto porque `gh` requiere autenticación en este entorno.
- El workflow necesita clonar `vcpkg` en el runner porque `deps/vcpkg` es un gitlink local no utilizable tal cual en Actions.

## Cobertura actual conocida

- 31 tests pasando en CTest.
- Cobertura ya existente en:
  - parsing de red y guards Kad;
  - rutas/configuración;
  - threading/shutdown;
  - helpers comunes de Fase 6;
  - smoke básico de CLI.

## Huecos prioritarios de cobertura

### Seguridad / parsing

- autenticación/handshake EC;
- validación estricta futura de `SecretHash`;
- paquetes eD2K/Kad hostiles;
- límites de tamaño y truncaciones.

### Rutas / configuración

- path traversal;
- rutas absolutas;
- rutas UNC;
- normalización;
- configuración corrupta o incompleta.

### Concurrencia

- ownership de ThreadPool;
- shutdown ordenado;
- cancelación de tareas;
- tareas pendientes;
- carreras históricas o conocidas.

### Red / compatibilidad

- smoke EC;
- smoke Kad/eD2K no destructivo;
- `wmulecmd` básico;
- compatibilidad de wire format.

### CI

- Build Debug;
- CTest;
- smoke CLI;
- futuro Release;
- futuro análisis estático;
- futuro sanitizers o equivalentes disponibles en MSVC/Windows.

## Prioridad recomendada

1. Consolidar CI Windows estable.
2. Mantener la validación local reproducible.
3. Añadir tests de caracterización para huecos críticos.
4. No iniciar Fase 8.
