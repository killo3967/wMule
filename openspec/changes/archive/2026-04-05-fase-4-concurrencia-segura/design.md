# Design: Fase 4 – Concurrencia Segura

## Enfoque técnico
Aplicamos una capa de sincronización declarativa encima de los componentes críticos (DownloadQueue, CPartFile, ThreadScheduler/ThreadPool, UploadBandwidthThrottler y los contadores globales) sin alterar los contratos públicos ni el protocolo eD2K/Kad/EC. Cada subsistema obtiene un guardián RAII (`Threading::ScopedQueueLock`, `PartFileAsyncGate`, `ThreadPoolShutdownGuard`) que centraliza el bloqueo y reporta métricas mediante `Threading::ThreadMetrics`. `CamuleApp::OnCoreTimer` pasa de ejecutar lógica "a ciegas" a publicar snapshots (`ThreadOwnershipSnapshot`) sólo cuando `DBG_THREAD` o la nueva preferencia `thePrefs::GetVerboseThreading()` están activos; fuera de ese modo los contadores permanecen accesibles únicamente para MuleUnit. Para shutdown coordinamos un token global `ThreadShutdownToken` que rechaza nuevas tareas, drenamos scheduler/throttler/pool con timeouts configurables y exponemos el resultado como parte del bloque de log único exigido por la especificación.

## Decisiones de arquitectura
| Opción | Tradeoff | Decisión |
| --- | --- | --- |
| Migrar todos los `wxMutex` existentes a `std::mutex` directamente | Riesgo elevado de regresión binaria en GUI/MuleUnit y coste amplio de refactor | Mantener `wxMutex` para compatibilidad y añadir un `std::mutex m_queueLock` exclusivo para `DownloadQueue::Process/ReinsertItem/RemoveSource`, protegido vía `Threading::ScopedQueueLock` que también instrumenta esperas. |
| Tomar preferencias dinámicamente en cada lectura | Más simple pero multiplica reads sin sincronización y puede mezclar valores de ciclos distintos | Introducir `ThreadPrefsSnapshot` (captura atómica de límites de ancho de banda y flags) y pasarlo por valor a `UploadBandwidthThrottler` y scheduler en cada iteración. |
| Forzar shutdown deteniendo hilos a la fuerza | Menos código pero deja trabajo incompleto y rompe compatibilidad con sockets activos | Implementar `Drain(timeout)` en ThreadPool y UploadBandwidthThrottler con retorno `enum class DrainResult { Completed, TimedOut }`, usando timeout configurable (`thePrefs::GetThreadDrainTimeoutMs()`). |
| Activar logs de contención siempre | Garantiza visibilidad pero añade ruido y coste >1% CPU | Encapsular todo el tracing tras `DBG_THREAD` + preferencia runtime `VerboseThreading`; fuera de ese modo sólo almacenamos contadores atómicos. |

## Flujo de datos
1. `CamuleApp::OnCoreTimer` obtiene `ThreadOwnershipSnapshot` (lectura atómica de `pendingTasks`, `throttlerDebt`, `wakeups`, `DownloadQueueOwnerId`, `ThrottlerOwnerId`). Si `VerboseThreading` está activo emite `AddDebugLogLineC(logThreading, ...)` con la instantánea y la duración de cada lock.
2. `DownloadQueue::Process` entra con `ScopedQueueLock`. El guard serializa escritores, captura tiempo de espera y actualiza `ThreadMetrics.pendingTasks`. Lectura/escritura real sigue usando `wxMutexLocker` para las colecciones internas, preservando compatibilidad.
3. `CPartFile::BeginAsyncTask/EndAsyncTask` notifican al nuevo `PartFileAsyncGate`, que publica `PartFileAsyncSnapshot` (conteo y timeout) accesible por tests.
4. `UploadBandwidthThrottler::Entry` toma `ThreadPrefsSnapshot`, ejecuta un ciclo y coloca `throttlerDebt` en métricas; al finalizar su `Drain()` espera a que todos los sockets confirmados hayan sido despachados.
5. `ThreadPool::Drain` bloquea `m_mutex`, pide a los workers finalizar tras agotar la cola y espera con `std::condition_variable` manteniendo `DrainResult` para el log final.

```
CamuleApp::ShutDown
   │
   ├─ ThreadShutdownToken::Activate()
   ├─ downloadqueue->ForEachPartFile(PartFileAsyncGate::RequestShutdown)
   │      └─ Scoped waits sobre m_asyncTaskMutex
   ├─ CThreadScheduler::Drain(timeout)
   ├─ CThreadPool::Instance().Drain(timeout)
   ├─ uploadBandwidthThrottler->Drain(timeout)
   └─ Cierra sockets / guarda estado
```

## Secuencia de shutdown limpio
1. `CamuleApp::ShutDown` obtiene `ThreadShutdownToken` (atomic_bool). Primer lock: `Threading::ShutdownMutex` para evitar reentrada.
2. `RequestAsyncTaskShutdown` sobre cada `CPartFile`: bloquea `m_asyncTaskMutex`, marca `m_asyncTasksBlocked`, espera hasta `m_asyncTaskCount==0` o `timeout`, actualiza `ThreadMetrics.partFileWaitMs` y, si expira, emite alerta.
3. `CThreadScheduler::Drain`: toma `schedulerMutex`, marca `IsTerminating`, despierta `m_completeCV` y espera `timeout`. Mantiene `ScopedThreadOwner` con ID del hilo scheduler para usarlo en OnCoreTimer.
4. `ThreadPool::Drain`: bajo `ThreadPoolShutdownGuard` deja de aceptar `Enqueue`, espera a que `m_tasks` quede vacía y que `m_activeTasks==0`. Retorna `DrainResult`.
5. `UploadBandwidthThrottler::Drain`: reutiliza `m_runMutex` + `m_runCond` para esperar a que `m_ControlQueue_list` y `m_StandardOrder_list` queden vacías, midiendo `throttlerDebt` restante.
6. Sólo después liberamos sockets, guardamos estado y escribimos el bloque único `ShutdownSummary{pendingTasks, throttlerDebt, DrainResult}` en log + MuleUnit.

## Instrumentación y logging
- Nuevo log-category `logThreading` (habilitado con `DBG_THREAD`).
- Preferencia `VerboseThreading` en `Preferences.ini`, superficie API `bool CPreferences::GetVerboseThreading() const` que se puede cambiar vía GUI/EC sin reiniciar.
- `Threading::ThreadMetrics` expone atómicos (`pendingTasks`, `throttlerDebt`, `wakeups`, `schedulerWaitMs`, `timedOutPartFiles`). `OnCoreTimer` y MuleUnit acceden vía `ThreadMetrics::Snapshot()`.
- Cada guard RAII reporta `WaitStart/End` en microsegundos; sólo se escribe en log cuando `VerboseThreading` es true.
- Flags de compilación: ninguno adicional; dependemos del existente `DBG_THREAD` y de `ENABLE_DEBUG_LOG`. En release, el coste se limita a incrementos atómicos (<1% CPU medido en `ThreadPoolBenchmark`).

## Cambios de ficheros
| Fichero | Acción | Descripción |
| --- | --- | --- |
| `src/libs/common/threading/ThreadGuards.h/cpp` | Nuevo | Define `ScopedQueueLock`, `ScopedThreadOwner`, `ThreadShutdownToken` y `ThreadMetrics`. |
| `src/DownloadQueue.{h,cpp}` | Modificar | Añadir `std::mutex m_queueLock`, snapshots internos, métricas de espera y rechazo de writers cuando `ThreadShutdownToken` está activo. |
| `src/PartFile.{h,cpp}` | Modificar | Extender `Begin/End/RequestAsyncTaskShutdown` con timeout (`std::chrono`), logging y exposición de `PartFileAsyncSnapshot`. |
| `src/amule.cpp` | Modificar | Instrumentar `OnCoreTimer` para publicar snapshots, reorganizar `ShutDown` siguiendo la secuencia y generar el bloque `ShutdownSummary`. |
| `src/UploadBandwidthThrottler.{h,cpp}` | Modificar | Añadir `Drain(timeout)`, snapshot de pref y actualización de `throttlerDebt`. |
| `src/ThreadScheduler.{h,cpp}` | Modificar | Introducir `Drain`, ownership tracking y uso de guards RAII. |
| `src/libs/common/ThreadPool.{h,cpp}` | Modificar | Añadir `DrainResult`, `Drain(std::chrono::milliseconds)`, `BeginShutdown()` y métricas `wakeups`. |
| `src/Statistics.{h,cpp}` | Modificar | Migrar contadores globales críticos a `std::atomic<uint64_t>` o protegerlos con `std::mutex s_statsMutex` + exponer `StatsSnapshot` para tests. |
| `src/Preferences.{h,cpp}` + GUI/EC | Modificar | Persistir `VerboseThreading` y `ThreadDrainTimeoutMs`, exponer getters/setters y sincronizar con EC commands. |
| `unittests/tests/{DownloadQueueRaceTest,PartFileAsyncTaskTest,ThreadPoolLifecycleTest}.cpp` + `unittests/tests/CMakeLists.txt` | Nuevo/Actualizar | Harness MuleUnit específicos que ejercitan colisiones de hilos y shutdown. |

## Interfaces / Contratos
- `struct ThreadOwnershipSnapshot { uint64 wakeups; uint32 pendingTasks; uint32 throttlerDebt; wxThreadIdType downloadQueueOwner; wxThreadIdType throttlerOwner; DrainResult schedulerDrain; };`
- `class ScopedQueueLock` (ctor toma `std::mutex&`, `ThreadMetrics&`, `ThreadOwnerTag`) adquiere el lock y actualiza `pendingTasks`, `waitNs`; destructor lo decrementa.
- `struct ThreadPrefsSnapshot { uint32 downLimit; uint32 upLimit; bool verboseThreading; std::chrono::milliseconds drainTimeout; };` obtenido vía `ThreadPrefsSnapshot CaptureThreadPrefs(const CPreferences& thePrefs);`.
- `enum class DrainResult { Completed, TimedOut };` usado por scheduler, ThreadPool y throttler. MuleUnit lee el último valor vía `Threading::ThreadMetrics::LastDrainResult()`.
- `class ThreadShutdownToken` ofrece `Activate()`, `IsActive()`, `GuardEnqueueOrErr(const char* origin)` devolviendo `ERR_SHUTTING_DOWN` y registrando el intento según la especificación.

## Estrategia de pruebas
| Capa | Qué probar | Enfoque |
| --- | --- | --- |
| Unit | `ScopedQueueLock`, `ThreadShutdownToken`, `ThreadPrefsSnapshot`, `DrainResult` | Nuevos tests en `ThreadingGuardsTest` con hilos sintéticos verificando métricas y rechazos. |
| Unit (MuleUnit) | `DownloadQueueRaceTest`, `PartFileAsyncTaskTest` | Simulan scheduler + escritor concurrente, verifican que `pendingTasks` y snapshots coinciden y que el timeout dispara trazas. |
| Integración | `ThreadPoolLifecycleTest`, `ShutdownSmokeTest` | Arrancan ThreadPool + throttler reales, ejecutan `CamuleApp::ShutDown` stub y comprueban `DrainResult`, rechazo de `QueueJob` y log consolidado. |
| Benchmark | `ThreadPoolBenchmark` extendido | Ejecutar antes/después para asegurar <1% overhead de métricas. |
| Manual | Activar `VerboseThreading`, iniciar descargas reales, observar bloque de log y ausencia de hilos colgados tras cerrar wmule.exe. |

## Migración / despliegue
No requiere migraciones de datos. El fichero de preferencias se amplía con dos claves nuevas (default `VerboseThreading=false`, `ThreadDrainTimeoutMs=5000`). Al arrancar, si las claves no existen se usan los valores por defecto y se guardan al cerrar.

## Preguntas abiertas
- ¿Debemos exponer `VerboseThreading` también vía línea de comandos para `wmulecmd.exe`? (por defecto sólo GUI/EC.)
- ¿Hay escenarios en los que `UploadBandwidthThrottler::Drain` necesite priorizar sockets críticos (EC) antes de rechazar nuevos clientes? Necesitamos confirmación funcional.
