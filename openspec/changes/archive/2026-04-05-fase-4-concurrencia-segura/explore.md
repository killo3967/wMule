## Exploration: Fase 4 – Concurrencia Segura

### Current State
- `docs/PLAN_MODERNIZACION_2.0.md` (líneas 240-278) define la fase como una auditoría completa de races en `DownloadQueue`, `PartFile`, `ThreadScheduler` y `ClientTCPSocket`, más métricas de estrés (`ThreadPoolBenchmark`, `DownloadBenchmark`). Sigue marcada como `[ ] Pendiente`, así que no hay evidencias de que exista un inventario previo.
- El bucle principal (`CamuleApp::OnCoreTimer`, `src/amule.cpp` 1336-1450) ejecuta `uploadqueue->Process()`, `downloadqueue->Process()`, `clientcredits->Process()` y `sharedfiles->Process()` en el hilo GUI sin protecciones adicionales. En paralelo viven `UploadBandwidthThrottler` (wxThread en `UploadBandwidthThrottler.cpp` 46-400), `CHTTPDownloadThread` y los trabajos del `CThreadScheduler` que se despachan sobre `CThreadPool` (`ThreadScheduler.cpp` 74-360 + `libs/common/ThreadPool.cpp` 24-97). No hay un punto único que coordine ownership entre todos esos hilos.
- `CDownloadQueue::Process` (`src/DownloadQueue.cpp` 413-475) recorre `m_filelist` con un `wxMutexLocker`, pero libera el cerrojo en cada iteración mediante `CMutexUnlocker` (`OtherFunctions.h` 391-406) para llamar a `CPartFile::Process`. Mientras el lock está liberado otros hilos (GUI, tareas de hashing, eventos EC) pueden eliminar o insertar elementos en `m_filelist`, dejando punteros colgantes durante `file->Process`.
- `CPartFile` delega trabajos pesados (hashing, preasignación, copia) en `CThreadScheduler` usando `BeginAsyncTask/EndAsyncTask` (`PartFile.cpp` 263-321 y 3727-3763). Solo el destructor llama a `RequestAsyncTaskShutdown`, pero `CamuleApp::ShutDown` (`src/amule.cpp` 1563-1609) nunca recorre las descargas activas para bloquear tareas antes de `CThreadScheduler::Terminate`, así que es posible terminar con escrituras pendientes mientras el hilo scheduler se cierra.
- `UploadBandwidthThrottler::Entry` (`UploadBandwidthThrottler.cpp` 277-400) calcula cuotas y toca `thePrefs`, `theStats` y `CUpDownClient`/`ThrottledFileSocket` sin sincronización global. Únicamente protege las listas internas con `m_sendLocker`, pero el resto del estado (estadísticas, preferencias, sockets de clientes) no emplea mutexes compartidos, de modo que depende de un “contracto social” implícito que hoy no está documentado.
- El subsistema de métricas (`src/Statistics.cpp`) solo usa `wxMutex` dentro de `CPreciseRateCounter::CalculateRate` (línea 69). Métodos como `theStats::AddSentBytes()` o `GetUploadRate()` se leen/escriben desde múltiples hilos (GUI, `UploadBandwidthThrottler`, tareas del scheduler), por lo que los contadores son propensos a races y valores incoherentes bajo carga.
- La cobertura actual se limita a benchmarks impresos (`unittests/tests/ThreadPoolBenchmark.cpp` y `DownloadBenchmark.cpp`), sin aserciones ni tests de sincronización. No existe validación automatizada de apagado limpio, ownership de colas ni condiciones de carrera sobre sockets.

### Affected Areas
- `docs/PLAN_MODERNIZACION_2.0.md` — referencia del alcance esperado y criterios de aceptación de la fase.
- `src/DownloadQueue.cpp` — locking granular con `CMutexUnlocker`, posible uso de punteros sobre datos borrados.
- `src/PartFile.cpp` + `src/ThreadTasks.cpp` — coordinación de tareas asincrónicas y su shutdown durante salida de la app.
- `src/ThreadScheduler.cpp` y `src/libs/common/ThreadPool.cpp` — único punto de entrada para trabajos en background, carente de métricas/cancelación.
- `src/UploadBandwidthThrottler.cpp` y `src/UploadQueue.cpp` — único hilo real de red, sin contrato de sincronización documentado frente al resto del core.
- `src/Statistics.cpp` y `theStats::*` — contadores globales accesibles desde todos los hilos sin atomics.
- `unittests/tests/ThreadPoolBenchmark.cpp` + `DownloadBenchmark.cpp` — únicas “validaciones” de concurrencia disponibles, pero solo imprimen números.

### Approaches
1. **Refuerzo de ownership en colas y ficheros** — Revisar `DownloadQueue`, `UploadQueue` y `CPartFile` para que ninguna estructura se use sin un lock claro o sin traspaso explícito de ownership al hilo worker.
   - Pros: elimina las races más peligrosas (punteros colgantes, escrituras simultáneas) y deja invariantes claros para el resto de la fase.
   - Contras: requiere tocar bucles calientes (`Process`, `SendBlockData`, `PartFile::Process`) y puede degradar throughput si no se diseña un esquema de locks por niveles.
   - Esfuerzo: Alto (varios módulos + validaciones manuales de GUI/core/red).

2. **Instrumentación y lifecycle gating** — Añadir métricas y controles cruzados en `CThreadScheduler`/`ThreadPool`, `UploadBandwidthThrottler` y `CamuleApp::ShutDown` para detectar tareas vivas, colas saturadas y falta de `EndThread()` antes de apagar.
   - Pros: impacto incremental, facilita observar si realmente quedan hilos vivos o colas colgando antes de tocar lógica sensible; habilita alertas tempranas sobre deadlocks.
   - Contras: no resuelve por sí mismo las races conocidas; necesita disciplina para no inundar los logs.
   - Esfuerzo: Medio (se concentra en puntos únicos de orquestación).

### Recommendation
Aplicar primero la instrumentación/lifecycle gating (Approach 2) para tener visibilidad de cuántos trabajos quedan activos en `CThreadScheduler`, qué sockets siguen vivos cuando el throttler termina y cuántas veces `DownloadQueue::Process` suelta el lock mientras se borran archivos. Con esos datos, atacar después las secciones críticas del Approach 1 (especialmente el bucle de `DownloadQueue` y la sincronización `PartFile` ↔ scheduler) con refactors acotados.

**Inputs sugeridos para la propuesta**
- **Objetivos:** documentar ownership por hilo (GUI/core, scheduler, throttler), garantizar apagado limpio sin tareas pendientes, eliminar races evidentes en `DownloadQueue` y en las tareas asincrónicas de `PartFile`.
- **Alcance mínimo:** `DownloadQueue::Process`, `PartFile::Begin/EndAsyncTask` + `ThreadTasks`, ganchos de `CThreadScheduler`/`ThreadPool`, `UploadBandwidthThrottler::Entry/EndThread`, contadores `theStats` y chequeos en `CamuleApp::ShutDown`.
- **Dependencias:** mantener compatibilidad con el ciclo principal (`OnCoreTimer`), respetar el uso actual del scheduler por hashing/completion, y coordinar con futuros cambios de Fase 5 (AsyncSocket) para no duplicar esfuerzos en sockets.

### Risks
- Bloquear secciones amplias (por ejemplo `DownloadQueue::Process`) puede introducir jank visible en la GUI si no se afinan los scopes de locking.
- Cambios en `UploadBandwidthThrottler` afectan al throughput de subida; sin benchmarks automáticos es fácil introducir regresiones silenciosas.
- Añadir instrumentation al scheduler sin limitarla podría generar logs demasiado verbosos y ocultar señales relevantes.
- Ajustar `theStats`/`thePrefs` para ser thread-safe puede requerir atomics o nuevos mutexes a nivel global, con impacto en rendimiento si no se perfila.

### Ready for Proposal
Sí. Con la información anterior el siguiente paso lógico es un `sdd-propose` que puntúe las tareas de instrumentación + refuerzo de locking, defina qué métricas/contadores hay que capturar y describa pruebas (stress loops + apagado limpio) antes de tocar la lógica central.
