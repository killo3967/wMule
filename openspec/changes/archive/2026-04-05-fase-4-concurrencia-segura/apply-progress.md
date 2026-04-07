# Apply Progress – Fase 4 – Concurrencia Segura

## Estado general
- [x] 2.x Núcleo endurecido — `DownloadQueue`, `CPartFile`, `ThreadScheduler`, `ThreadPool` y `UploadBandwidthThrottler` usan guards RAII (`ScopedQueueLock`, `PartFileAsyncGate`) y publican `Drain(timeout)`/`ThreadPrefsSnapshot`.
- [x] 2.1 DownloadQueue bajo lock consistente — `Process()` y las ordenaciones internas se ejecutan bajo `Threading::ScopedQueueLock(m_queueLock, DownloadQueue)` para sincronizar iteraciones y métricas.
- [x] 3.x Secuencia de apagado — `CamuleApp::ShutDown` activa `ThreadShutdownToken`, drena scheduler/pool/throttler y emite `ShutdownSummary` con las métricas finales.
- [x] 4.x Validación — Nuevos MuleUnit (`DownloadQueueRaceTest`, `PartFileAsyncTaskTest`, `ThreadPoolLifecycleTest`, `ShutdownSmokeTest`) + script manual `manual-tests/threading/VerboseThreading.ps1`.
- [x] 5.x Documentación — Se actualizó `docs/PLAN_MODERNIZACION_2.0.md` y se creó `docs/debug/threading.md` describiendo toggles, logs y validación.
- [ ] Builds/tests no ejecutados en esta fase debido a la política "Never build after changes". Documentado en `docs/debug/threading.md`; queda pendiente que verificación ejecute `cmake --build` + `ctest`.

## Riesgos / Bloqueos
- La API EC sigue sin exponer los toggles; se dejó stub en CLI/web y se documentó el workaround (GUI / edición manual). Para habilitarlo realmente habrá que regenerar `ECCodes` e incrementar versión de protocolo.
- A falta de builds/tests (bloqueados explícitamente por "Never build after changes"), la verificación queda aplazada hasta que un pipeline autorizado ejecute `cmake --build` + `ctest` y confirme el refactor de concurrencia.

## Próximos pasos
1. Ejecutar build + suite MuleUnit completa en cuanto esté permitido para validar el refactor de concurrencia.
2. Planificar regeneración de `ECCodes` (o nuevo contrato EC) para exponer `VerboseThreading`/`ThreadDrainTimeoutMs` remotamente sin romper wire format.
3. Dar seguimiento a la ejecución del script manual en QA (ver `manual-tests/threading/VerboseThreading.ps1`) y recopilar feedback sobre `ShutdownSummary`.
