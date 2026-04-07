# Tasks: Fase 4 – Concurrencia Segura

## Phase 1: Infra y snapshots

- [x] 1.1 Guardas y métricas base — Crear `src/libs/common/threading/ThreadGuards.{h,cpp}` con `ThreadMetrics`, `ScopedQueueLock`, `ScopedThreadOwner` y `ThreadShutdownToken`, registrar `logThreading` y exponer `ThreadOwnershipSnapshot`/`ThreadPrefsSnapshot`. (Ficheros: nuevos ThreadGuards + `src/Logger.cpp`; Dependencias: -; Tipo: código)
- [x] 1.2 Preferencias VerboseThreading/DrainTimeout — Añadir claves y getters/setters en `src/Preferences.{h,cpp}`, persistir en GUI/EC (`src/amuleDlg.*, src/ExternalConn.cpp`) y exponer `CaptureThreadPrefs(const CPreferences&)`. (Ficheros: `src/Preferences.*`, UI/EC wiring; Dependencias: 1.1; Tipo: código)
- [x] 1.3 Atomizar theStats y snapshot de contadores — Migrar campos críticos de `src/Statistics.{h,cpp}` a `std::atomic` o `StatsSnapshot`, vincularlos con `ThreadMetrics::Snapshot()` accesible para MuleUnit y OnCoreTimer. (Ficheros: `src/Statistics.*`; Dependencias: 1.1-1.2; Tipo: código)

## Phase 2: Núcleo de concurrencia e instrumentación

- [x] 2.1 Refactor de `DownloadQueue` — Introducir `std::mutex m_queueLock`, envolver `Process/ReinsertItem/RemoveSource` con `ScopedQueueLock`, capturar esperas y rechazar writers cuando `ThreadShutdownToken` esté activo. (Ficheros: `src/DownloadQueue.{h,cpp}`; Dependencias: 1.1-1.3; Tipo: código)
- [x] 2.2 `CPartFile` async gate — Extender `Begin/End/RequestAsyncTaskShutdown` con `PartFileAsyncGate`, timeouts configurables y métricas (`PartFileAsyncSnapshot`) visibles vía tests/logs. (Ficheros: `src/PartFile.{h,cpp}`; Dependencias: 1.1-1.3; Tipo: código)
- [x] 2.3 Throttler con snapshots de prefs — Consumir `ThreadPrefsSnapshot` en `UploadBandwidthThrottler::Entry`, actualizar `throttlerDebt` atómico y preparar `Drain(timeout)` almacenando `DrainResult`. (Ficheros: `src/UploadBandwidthThrottler.{h,cpp}`; Dependencias: 1.2-1.3; Tipo: código)
- [x] 2.4 Scheduler + OnCoreTimer instrumentados — Añadir ownership tracking en `src/ThreadScheduler.{h,cpp}` y reescribir `CamuleApp::OnCoreTimer` para publicar y loguear `ThreadOwnershipSnapshot` sólo si `VerboseThreading` o `DBG_THREAD` están activos. (Ficheros: `src/ThreadScheduler.*`, `src/amule.cpp`; Dependencias: 1.x-2.3; Tipo: código)

## Phase 3: Secuencia de apagado y rechazos

- [x] 3.1 Integrar `ThreadShutdownToken` en flujo de cierre — Activar el token desde `CamuleApp::ShutDown`, coordinar `RequestAsyncTaskShutdown` en todas las `CPartFile` y evitar que `OnCoreTimer` reprograme ticks tras la activación. (Ficheros: `src/amule.cpp`, `src/PartFile.*`; Dependencias: 2.1-2.4; Tipo: código)
- [x] 3.2 `Drain()` coordinado — Implementar `Drain(std::chrono::milliseconds)` con `DrainResult` en `ThreadScheduler`, `ThreadPool` y `UploadBandwidthThrottler`, reutilizando `ThreadPoolShutdownGuard`/condvars y reportando métricas. (Ficheros: `src/ThreadScheduler.*`, `src/libs/common/ThreadPool.*`, `src/UploadBandwidthThrottler.*`; Dependencias: 2.1-3.1; Tipo: código)
- [x] 3.3 Bloqueo de nuevas tareas y resumen de cierre — Hacer que `DownloadQueue::Enqueue`, `ThreadPool::QueueJob` y `UploadBandwidthThrottler::AddClient` verifiquen `ThreadShutdownToken`, registren `ERR_SHUTTING_DOWN`, y producir el bloque `ShutdownSummary` con los snapshots finales en `CamuleApp::ShutDown`. (Ficheros: `src/DownloadQueue.*`, `src/libs/common/ThreadPool.*`, `src/UploadBandwidthThrottler.*`, `src/amule.cpp`; Dependencias: 3.1-3.2; Tipo: código)

## Phase 4: Verificación automatizada y manual

- [x] 4.1 MuleUnit `ThreadingGuardsTest` — Crear `unittests/tests/ThreadingGuardsTest.cpp` + entradas en `CMakeLists.txt` para validar `ScopedQueueLock`, `ThreadShutdownToken` y snapshots sintéticos con hilos de prueba. (Ficheros: `unittests/tests/*.cpp`, `unittests/tests/CMakeLists.txt`; Dependencias: 1.x; Tipo: test)
- [x] 4.2 `DownloadQueueRaceTest` y `PartFileAsyncTaskTest` — Implementar fixtures MuleUnit que simulen scheduler/escritor concurrente y verifiquen contadores, timeouts y trazas, incluyendo harness para `PartFileAsyncSnapshot`. (Ficheros: `unittests/tests/DownloadQueueRaceTest.cpp`, `unittests/tests/PartFileAsyncTaskTest.cpp`; Dependencias: 2.x; Tipo: test)
- [x] 4.3 `ThreadPoolLifecycleTest` + `ShutdownSmokeTest` + script manual — Añadir MuleUnit para `ThreadPool::Drain`/`ShutdownSummary` y un script PowerShell (`manual-tests/threading/VerboseThreading.ps1`) que active la preferencia, dispare descargas y verifique logs. (Ficheros: `unittests/tests/ThreadPoolLifecycleTest.cpp`, `unittests/tests/ShutdownSmokeTest.cpp`, `manual-tests/threading/VerboseThreading.ps1`; Dependencias: 3.x; Tipo: test)

## Phase 5: Documentación y toggles

- [x] 5.1 Actualizar `docs/PLAN_MODERNIZACION_2.0.md` — Registrar el progreso de Fase 4, los nuevos pasos de validación (tests ejecutados, script manual) y cualquier impacto en roadmap. (Ficheros: `docs/PLAN_MODERNIZACION_2.0.md`; Dependencias: 1-4; Tipo: docs)
- [x] 5.2 Notas de depuración y ayuda de usuario — Documentar `VerboseThreading`, `ThreadDrainTimeoutMs` y el flujo `ShutdownSummary` en `docs/debug/threading.md` (o similar) y en el texto de ayuda/UI correspondiente. (Ficheros: `docs/debug/threading.md`, `src/amuleDlg.*` strings; Dependencias: 1-4; Tipo: docs)
