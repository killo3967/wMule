## Verification Report

**Cambio**: fase-4-concurrencia-segura  
**Versión**: N/A  
**Modo**: Standard (Strict TDD no aplica; no existe configuración en `openspec/config.yaml` ni memorias previas)

---

### Completeness
| Métrica | Valor |
|---------|-------|
| Tareas totales | 15 |
| Tareas completas | 15 |
| Tareas incompletas | 0 |

Todas las tareas en `openspec/changes/fase-4-concurrencia-segura/tasks.md` están marcadas con `[x]`, incluidos los hitos 2.1 (DownloadQueue bajo lock) y 4.x (suites MuleUnit + script manual).

---

### Build & Tests Execution

**Build**: ⚠️ No ejecutado (bloqueado por política)  
```
La política global “Never build after changes” sigue vigente y `apply-progress.md`/`docs/debug/threading.md` indican que la verificación debe delegar `cmake --build . --config Debug` en un pipeline autorizado. No se pudo producir binario actualizado.
```

**Tests**: ⚠️ No ejecutados (bloqueado por política)  
```
La misma política impide lanzar `ctest --output-on-failure -C Debug`. Las suites MuleUnit recientemente añadidas (ThreadingGuardsTest, DownloadQueueRaceTest, PartFileAsyncTaskTest, ThreadPoolLifecycleTest, ShutdownSmokeTest) permanecen sin evidencia runtime.
```

**Cobertura**: ➖ No disponible — no se ejecutaron pruebas, por lo que no se generan reportes de coverage.

---

### Spec Compliance Matrix
| Requisito | Escenario | Test / Evidencia esperada | Resultado |
|-----------|-----------|---------------------------|-----------|
| Ownership declarativo y métricas activables | Métricas visibles bajo demanda | `ThreadingGuardsTest > ScopedQueueLockUpdatesPendingTasks` | ⚠️ BLOQUEADO — no se ejecutó MuleUnit por la política “Never build after changes”. |
| Ownership declarativo y métricas activables | Ruido contenido en builds normales | `manual-tests/threading/VerboseThreading.ps1` (observación de logs con VerboseThreading desactivado) | ⚠️ BLOQUEADO — el script manual quedó pendiente de QA. |
| DownloadQueue bajo lock consistente | Scheduler protegido frente a escritores concurrentes | `DownloadQueueRaceTest > WritersSerializeAccess` | ⚠️ BLOQUEADO — no hay ejecución de tests aún. |
| Integridad de tareas en CPartFile | PartFile no se destruye con tareas activas | `PartFileAsyncTaskTest > RequestShutdownWaitsForTasks` | ⚠️ BLOQUEADO — pendiente por la misma restricción de build/test. |
| Contadores globales coherentes | Ingresos concurrentes sin pérdida | (sin prueba automatizada; depende de validación posterior sobre `theStats`) | ⚠️ UNTESTED — falta suite específica. |
| Contadores globales coherentes | Lecturas consistentes en throttler | (sin prueba automatizada; se esperaba MuleUnit dedicado) | ⚠️ UNTESTED — falta suite específica. |
| Secuencia de apagado orquestada | Apagado limpio del scheduler y throttler | `ShutdownSmokeTest` + inspección de logs | ⚠️ BLOQUEADO — sin ejecución y, además, el timer central sigue sin detenerse explícitamente. |
| ThreadPool Drain API con verificación | Drain exitoso | `ThreadPoolLifecycleTest > DrainCompletesWithinTimeout` | ⚠️ BLOQUEADO — MuleUnit no ejecutado. |
| ThreadPool Drain API con verificación | Drain excede timeout | `ThreadPoolLifecycleTest > DrainTimesOutWhenWorkStalls` | ⚠️ BLOQUEADO — MuleUnit no ejecutado. |
| Guardas frente a nuevo trabajo durante shutdown | Enqueue bloqueado al cerrar | `ThreadingGuardsTest > ShutdownTokenRejectsNewWorkAfterActivation` | ⚠️ BLOQUEADO — MuleUnit no ejecutado. |
| Evidencia de finalización | Registro único de cierre | `manual-tests/threading/VerboseThreading.ps1` monitoreando `ShutdownSummary` | ⚠️ BLOQUEADO — script manual pendiente. |

**Resumen de cumplimiento**: 0/11 escenarios con evidencia runtime (todos bloqueados por política y 2 carecen aún de test dedicado).

---

### Correctness (Static — Evidencia estructural)
| Requisito | Estado | Notas |
|-----------|--------|-------|
| Ownership declarativo y métricas activables | ✅ Implementado | `Threading::ThreadMetrics` + `CamuleApp::OnCoreTimer` (src/amule.cpp 1338-1396) sólo publican snapshots bajo `thePrefs::GetVerboseThreading()` o `DBG_THREAD`, tal como exige la spec.
| DownloadQueue bajo lock consistente | ✅ Implementado | `DownloadQueue::Process()` protege todo el recorrido con `Threading::ScopedQueueLock queueGuard(m_queueLock, ...)` y `wxMutexLocker` (src/DownloadQueue.cpp 425-552). `RemoveSource()` también entra con el mismo guard (línea 776).
| Integridad de tareas en CPartFile | ✅ Implementado | `CPartFile::Begin/End/RequestAsyncTaskShutdown` delegan en `Threading::PartFileAsyncGate` con timeouts configurables (src/PartFile.cpp 3717-3736).
| Contadores globales coherentes | ✅ Implementado | `Statistics.cpp` migró contadores críticos a `std::atomic` y `UploadBandwidthThrottler::Entry()` consume `ThreadPrefsSnapshot` consistente (Statistics.cpp 148-383; UploadBandwidthThrottler.cpp 303-475).
| Secuencia de apagado orquestada | ⚠️ Parcial | `CamuleApp::ShutDown()` drena scheduler/pool/throttler pero no detiene explícitamente `core_timer`; depende de que `OnCoreTimer` haga early-return al ver `Threading::ShutdownToken()` (src/amule.cpp 1338-1653).
| ThreadPool Drain API con verificación | ✅ Implementado | `CThreadScheduler::Drain`, `CThreadPool::Drain` y `UploadBandwidthThrottler::Drain` usan `Threading::DrainResult` y publican métricas (ThreadScheduler.cpp 426-462; ThreadPool.cpp 90-104; UploadBandwidthThrottler.cpp 509-541).
| Guardas frente a nuevo trabajo durante shutdown | ✅ Implementado | Las rutas `DownloadQueue::AddDownload`, `CThreadPool::Enqueue`, `UploadBandwidthThrottler::AddTo*` llaman `ThreadShutdownToken::GuardEnqueueOrErr` y emiten `ERR_SHUTTING_DOWN`.
| Evidencia de finalización | ✅ Implementado | `CamuleApp::ShutDown()` escribe `ShutdownSummary` y `docs/debug/threading.md` documenta cómo consultarlo.

---

### Coherence (Design)
| Decisión | ¿Se siguió? | Observaciones |
|----------|-------------|---------------|
| Añadir `std::mutex m_queueLock` sólo para secciones críticas de DownloadQueue | ✅ Sí | `m_queueLock` existe y `Threading::ScopedQueueLock` envuelve `AddDownload`, `Process` y `RemoveSource`, cumpliendo el tradeoff del diseño.
| Pasar preferencias por snapshot (`ThreadPrefsSnapshot`) | ✅ Sí | `Threading::CaptureThreadPrefs` y sus consumidores (throttler, PartFile) usan snapshots por valor.
| Implementar `Drain(timeout)` con `DrainResult` en scheduler/pool/throttler | ✅ Sí | Los tres drenan con el mismo enum y publican el resultado en `ShutdownSummary`.
| Encapsular tracing bajo `VerboseThreading`/`DBG_THREAD` | ✅ Sí | Tanto `OnCoreTimer` como `ShutdownSummary` revisan la preferencia/categoría antes de escribir logs.

---

### Issues Found

**CRITICAL**
- Ninguno. El locking pendiente en `DownloadQueue::Process()` quedó resuelto.

**WARNING**
1. **Builds/tests bloqueados por política** — Sigue sin ejecutarse `cmake --build`/`ctest`; la propia documentación (`docs/debug/threading.md`, sección “Limitaciones”) exige mantener este riesgo como WARNING hasta que un pipeline autorizado ejecute la validación runtime.
2. **`CamuleApp::ShutDown()` no detiene `core_timer`** — Aunque `Threading::ShutdownToken()` evita recursión, la spec requiere parar el timer antes de drenar. Hay que decidir si se detiene explícitamente o se actualiza la spec.
3. **Escenarios de contadores globales sin test dedicado** — No existe MuleUnit que verifique `theStats::sessionDownloadedBytes` ni las lecturas snapshot del throttler; dependen de logs/manual.
4. **Script manual sin ejecutar** — `manual-tests/threading/VerboseThreading.ps1` forma parte de la validación planificada pero aún no se ha corrido (QA pendiente).

**SUGGESTION**
1. Extender las suites MuleUnit para cubrir `VerboseThreading` (logs ON/OFF) y así reducir dependencia de pasos manuales.

---

### Verdict
**PASS WITH WARNINGS** — El refactor de locking de `DownloadQueue::Process()` cumple lo exigido, pero la validación runtime permanece bloqueada por la política “Never build after changes” y hay advertencias pendientes sobre shutdown y cobertura de contadores.
