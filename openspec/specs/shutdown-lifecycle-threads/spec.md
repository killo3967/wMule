# shutdown-lifecycle-threads Specification

## Purpose
Definir el ciclo de vida seguro de scheduler, throttler y ThreadPool para que `CamuleApp::ShutDown` termine sin hilos colgando ni trabajo en vuelo.

## Functional Requirements

### Requirement: Secuencia de apagado orquestada
El sistema MUST invocar `RequestAsyncTaskShutdown`, detener `OnCoreTimer`, drenar `CThreadScheduler` y `UploadBandwidthThrottler` en ese orden antes de liberar sockets.

#### Scenario: Apagado limpio del scheduler y throttler
- GIVEN `CamuleApp::ShutDown` ha comenzado
- WHEN existen tareas pendientes en scheduler o deuda en throttler
- THEN `OnCoreTimer` deja de rearmarse, ambos componentes reciben `Drain(timeout=5s)` y confirman cero tareas/bytes pendientes antes de cerrar

### Requirement: ThreadPool Drain API con verificación
El sistema MUST exponer `ThreadPool::Drain(std::chrono::milliseconds timeout)` que bloquea hasta que no haya trabajadores ocupados y retorna un `enum class DrainResult`.

#### Scenario: Drain exitoso
- GIVEN la cola tiene trabajo ligero
- WHEN `CamuleApp::ShutDown` llama `Drain(2000ms)`
- THEN el resultado es `Completed`, se añade una traza `DBG_THREAD` y `ThreadPool` queda en estado `Stopped`

#### Scenario: Drain excede timeout
- GIVEN una tarea se bloquea más de 2 segundos
- WHEN expira el timeout de `Drain`
- THEN el resultado es `TimedOut`, se registra el identificador de la tarea y se emite una advertencia para rollback manual

### Requirement: Guardas frente a nuevo trabajo durante shutdown
El sistema SHALL rechazar nuevas tareas (`DownloadQueue::Enqueue`, `ThreadPool::QueueJob`, `UploadBandwidthThrottler::AddClient`) una vez que `RequestAsyncTaskShutdown` marque el flag global.

#### Scenario: Enqueue bloqueado al cerrar
- GIVEN el flag `IsShuttingDown` está activo
- WHEN un subsistema intenta encolar descarga
- THEN recibe un error `ERR_SHUTTING_DOWN` y una traza única describe el origen

### Requirement: Evidencia de finalización
El sistema SHOULD escribir un bloque de log consolidado con contadores finales (`pendingTasks=0`, `bandwidthDebt=0`) y SHOULD exponerlos vía MuleUnit para verificación automática.

#### Scenario: Registro único de cierre
- GIVEN `CamuleApp::ShutDown` finaliza sin tareas
- WHEN la app se cierra
- THEN se escribe una traza `DBG_THREAD` única con los contadores en cero y MuleUnit puede leerlos mediante `ThreadPoolLifecycleTest`

## Non-Functional Requirements
- `Drain` MUST respetar el timeout configurado por `thePrefs` sin bloquear la GUI.
- El apagado MUST mantener compatibilidad de protocolo (las conexiones activas se cierran tras drenar, no antes).
- Las nuevas trazas SHOULD agruparse en un único bloque para facilitar auditoría.

## Test Cases
- MuleUnit `ThreadPoolLifecycleTest`: arranca, encola trabajo y valida resultados `Completed/TimedOut`.
- MuleUnit `ShutdownSmokeTest`: simula `CamuleApp::ShutDown`, verifica flags `IsShuttingDown` y rechazos de enqueue.
- Benchmarks `ThreadPoolBenchmark` y `DownloadBenchmark`: ejecutar con y sin shutdown para medir impacto temporal.
- Manual: cerrar `wmule.exe` tras descargas activas y revisar logs para confirmar bloque único de métricas y ausencia de hilos persistentes via Process Explorer.
