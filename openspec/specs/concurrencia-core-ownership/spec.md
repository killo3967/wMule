# concurrencia-core-ownership Specification

## Purpose
Alinear el contrato de ownership entre GUI, scheduler y throttler, registrando métricas mínimas que permitan detectar contención sin introducir regresiones de compatibilidad eD2K/Kad/EC.

## Functional Requirements

### Requirement: Ownership declarativo y métricas activables
El sistema MUST exponer contadores atómicos (`pendingTasks`, `throttlerDebt`, `wakeups`) y el hilo que posee cada recurso compartido a través de `CamuleApp::OnCoreTimer`, emitiendo trazas únicamente bajo `DBG_THREAD` o `thePrefs::GetVerboseThreading()`.

#### Scenario: Métricas visibles bajo demanda
- GIVEN `thePrefs::GetVerboseThreading()` está activo
- WHEN `OnCoreTimer` se ejecuta mientras `CThreadScheduler` tiene tareas pendientes
- THEN se publica una traza `DBG_THREAD` con los contadores y el owner actual de DownloadQueue y throttler
- AND la traza incluye identificadores de hilo y números monotónicos de wakeups

#### Scenario: Ruido contenido en builds normales
- GIVEN `thePrefs::GetVerboseThreading()` está desactivado
- WHEN `UploadBandwidthThrottler` aumenta `throttlerDebt`
- THEN no se escribe log adicional y los contadores sólo quedan disponibles vía MuleUnit

### Requirement: DownloadQueue bajo lock consistente
El sistema MUST procesar `DownloadQueue::Process`, `ReinsertItem` y `RemoveSource` bajo un `std::mutex` compartido declarado en la cola, manteniendo iteradores estables mediante snapshots internos.

#### Scenario: Scheduler protegido frente a escritores concurrentes
- GIVEN el scheduler intenta iterar la cola
- WHEN otro hilo quiere eliminar un paquete
- THEN el segundo hilo espera al mutex, el scheduler ve una vista consistente y ambos hilos actualizan métricas de espera

### Requirement: Integridad de tareas en CPartFile
El sistema MUST registrar cada tarea asíncrona (`PartFile::StartAsyncTask`) y MUST impedir destruir `CPartFile` hasta que `taskCount` llegue a cero o expire un timeout reportado.

#### Scenario: PartFile no se destruye con tareas activas
- GIVEN una descarga mantiene dos tareas de hash activas
- WHEN `CPartFile` recibe `EndDownload` y solicita destrucción
- THEN bloquea hasta que ambas tareas llamen `EndAsyncTask`
- AND se emite una traza única si el bloqueo supera el timeout configurado

### Requirement: Contadores globales coherentes
El sistema MUST actualizar `theStats` mediante atomics o un mutex dedicado y MUST exponer snapshots inmutables de `thePrefs` a `UploadBandwidthThrottler` antes de cada ciclo.

#### Scenario: Ingresos concurrentes sin pérdida
- GIVEN dos hilos reportan bytes descargados simultáneamente
- WHEN incrementan `theStats::sessionDownloadedBytes`
- THEN el contador resultante es la suma exacta y se registra una única métrica de contención si hubo espera

#### Scenario: Lecturas consistentes en throttler
- GIVEN `UploadBandwidthThrottler` inicia un ciclo
- WHEN solicita límites de ancho de banda a `thePrefs`
- THEN obtiene un snapshot inmutable hasta finalizar el ciclo y todas las lecturas usan el mismo valor

## Non-Functional Requirements
- El coste agregado de las métricas MUST ser <1 % de CPU en `ThreadPoolBenchmark`.
- Las trazas SHOULD poder habilitarse/inhabilitarse en tiempo de ejecución sin reiniciar la app.
- Todo el locking nuevo MUST preservar compatibilidad binaria de colas y archivos parciales.

## Test Cases
- MuleUnit `DownloadQueueRaceTest`: fuerza escrituras simultáneas y valida snapshots/contadores.
- MuleUnit `PartFileAsyncTaskTest`: garantiza bloqueo en destrucción y trazas de timeout.
- MuleUnit `ThreadPoolBenchmark` extendido: compara overhead de métricas contra baseline.
- Manual: habilitar `DBG_THREAD`, descargar un conjunto de archivos y revisar que las trazas sólo aparezcan con la preferencia activa.
