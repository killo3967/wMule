# Proposal: Fase 4 – Concurrencia Segura

## Intent

- Documentar y hacer cumplir el ownership entre GUI/core, scheduler y throttler para evitar contratos implícitos.
- Garantizar apagado ordenado: ninguna tarea pendiente en `CThreadScheduler`, `ThreadPool` ni `UploadBandwidthThrottler` tras `CamuleApp::ShutDown`.
- Endurecer colas y contadores (`DownloadQueue`, `PartFile`, `theStats`, cuotas EC/Kad) sin romper compatibilidad eD2K/Kad/EC.

## Scope

### In Scope
- Mapa de ownership + instrumentación (counters, tracing acotado) en `CamuleApp::OnCoreTimer`, scheduler y throttler.
- Lifecycle gating: `RequestAsyncTaskShutdown`, `ThreadScheduler::Terminate`, `ThreadPool` (enqueue/cancel/wait) con asserts MuleUnit.
- Refuerzo de locking y validaciones agresivas en `DownloadQueue`, `PartFile`, `UploadBandwidthThrottler`, `Statistics` + stress loops (`ThreadPoolBenchmark`, `DownloadBenchmark`).

### Out of Scope
- Migrar sockets (`ClientTCPSocket`, AsyncSocket) → Fase 5.
- Cambiar protocolos, wire formats o contratos EC/eD2K/Kad.
- Rediseñar UI/logging global; solo se añaden contadores mínimos necesarios.

## Capabilities

### New Capabilities
- `concurrencia-core-ownership`: describe el contrato de ownership/locking entre hilos (GUI, scheduler, throttler) y métricas asociadas.
- `shutdown-lifecycle-threads`: cubre cancelación explícita, tiempos máximos y evidencias de apagado limpio en scheduler/threadpool/PartFile tasks.

### Modified Capabilities
- Ninguna.

## Approach

1. **Instrumentación mínima**: contadores atómicos + `wxCondition` wake metrics para `CThreadScheduler`, `ThreadPool`, `UploadBandwidthThrottler`, `CamuleApp::OnCoreTimer`; limitar a `DBG_THREAD` para no inundar logs.
2. **Lifecycle gating**: obligar a que `CamuleApp::ShutDown` itere colas activas y llame `RequestAsyncTaskShutdown` antes de terminar scheduler; exponer `ThreadPool::Drain(timeout)` y MuleUnit `ThreadPoolLifecycleTest` + `ShutdownSmokeTest` que verifique cero hilos vivos.
3. **Locks/ownership**: reencolar `DownloadQueue::Process` bajo secciones críticas cortas con iteradores estables; `CPartFile` registra tareas vivas y bloquea destructores hasta `EndAsyncTask`; `theStats` pasa a atomics/mutex dedicados. Crear `DownloadQueueRaceTest`, `PartFileAsyncTaskTest` y extender `ThreadPoolBenchmark` con aserciones.
4. **Validación continua**: ejecutar benchmarks tras cada bloque y revisar manualmente throughput; mantener compatibilidad de `OnCoreTimer` y mantener `UploadBandwidthThrottler` sin modificar wire-level.

## Affected Areas

| Área | Impacto | Descripción |
|------|---------|-------------|
| `docs/PLAN_MODERNIZACION_2.0.md` | Modificado | Actualizar estado, tareas y métricas de la fase. |
| `src/amule.cpp` (`OnCoreTimer`, `ShutDown`) | Modificado | Hook de métricas y secuencia de apagado con drain explícito. |
| `src/DownloadQueue.cpp` | Modificado | Locking granular, invariantes y tracing. |
| `src/PartFile.cpp` + `src/ThreadTasks.cpp` | Modificado | Registro de tareas asincrónicas, shutdown seguro. |
| `src/ThreadScheduler.cpp` + `src/libs/common/ThreadPool.cpp` | Modificado | Contadores, cancelación, `Drain`. |
| `src/UploadBandwidthThrottler.cpp` / `src/UploadQueue.cpp` | Modificado | Protección al acceder a `thePrefs`/`theStats` y métricas de deuda. |
| `src/Statistics.cpp` | Modificado | Atomics/mutex para contadores globales. |
| `unittests/tests/*Benchmark.cpp` + nuevos tests | Nuevo/Modificado | Stress + MuleUnit de lifecycle y races. |

## Risks

| Riesgo | Probabilidad | Mitigación |
|--------|--------------|------------|
| Bloqueo perceptible en GUI por locks más largos | Media | Mantener secciones críticas cortas, perfilar con contadores añadidos. |
| Pérdida de throughput al tocar `UploadBandwidthThrottler` | Media | Ejecutar benchmarks antes/después y habilitar rollback parcial de cuotas. |
| Logs excesivos que oculten señales | Baja | Encapsular tracing tras flag `thePrefs::GetVerboseThreading()`. |
| Atomics/mutex globales degradan CPU | Media | Medir en `ThreadPoolBenchmark` y, si es necesario, usar caches locales + `std::memory_order_relaxed`. |

## Rollback Plan

- Revertir commits que añaden contadores/locking y restaurar `ThreadPool` previo (sin afectar files persistentes).
- Desactivar tracing vía flag y recompilar para volver al comportamiento anterior sin borrar código nuevo.
- Mantener tests nuevos pero marcarlos `SKIP_CONCURRENCY` si bloquean releases.

## Dependencies

- Harness MuleUnit existente (`ThreadPoolBenchmark`, `DownloadBenchmark`) para validar sin CI adicional.
- Canal de logging `DBG_THREAD` disponible en builds actuales.
- Coordinación con Fase 5 para no duplicar trabajo sobre sockets.

## Success Criteria

- [ ] Ownership documentado + métricas activables (`concurrencia-core-ownership`).
- [ ] `ThreadPoolLifecycleTest`, `DownloadQueueRaceTest`, `PartFileAsyncTaskTest` y benchmarks pasando reproduciblemente.
- [ ] Logs de apagado muestran 0 tareas pendientes y throttler termina antes de cerrar sockets.
