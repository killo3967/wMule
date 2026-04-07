# Threading diagnostics

## Preferencias

- **VerboseThreading**: GUI → Preferencias → Tweaks → *Threading diagnostics*. Habilitar esta casilla activa el log `logThreading` sin recompilar.
- **ThreadDrainTimeoutMs**: mismo panel, valores aceptados `1000-600000 ms`. El timeout se reutiliza en `CPartFile::RequestAsyncTaskShutdown`, `CThreadScheduler::Drain`, `CThreadPool::Drain` y `UploadBandwidthThrottler::Drain`.
- **CLI / fallback**: mientras la API EC no exponga los nuevos tags, los toggles sólo pueden cambiarse en GUI o editando `preferences.ini` (sección `[Threading]`, claves `VerboseThreading=0/1`, `ThreadDrainTimeoutMs=<ms>`). Tras modificar el fichero hay que reiniciar wMule.
- **Stub EC**: `wmulecmd` muestra un mensaje explícito al intentar `Set Threading ...` hasta que se regeneren los ficheros `ECCodes`. No se silencian los intentos remotos.

## Logging

- `CamuleApp::OnCoreTimer` sólo emite métricas cuando `VerboseThreading` o `DBG_THREAD` están activos.
- `CamuleApp::ShutDown` siempre produce `ShutdownSummary pending=... drainTimeoutMs=...` tras drenar scheduler/pool/throttler. Buscar esa línea en el log (`logfile` configurado en GUI) para verificar que no quedó deuda (`throttlerDebt=0`).

## Script manual

- `manual-tests/threading/VerboseThreading.ps1` recuerda los pasos para activar los toggles y, con `-Watch`, monitorea el log filtrando `ShutdownSummary`.
- Ejemplo: `pwsh .\manual-tests\threading\VerboseThreading.ps1 -LogPath C:\wMule\logs\amule.log -DrainTimeoutMs 8000 -Watch`

## Cobertura MuleUnit

- `DownloadQueueRaceTest`: dos escritores compiten por `ScopedQueueLock` y verifican owner/pendingTasks.
- `PartFileAsyncTaskTest`: valida que `PartFileAsyncGate` espera y registra timeouts.
- `ThreadPoolLifecycleTest`: cubre `CThreadPool::Drain` para casos `Completed/TimedOut`.
- `ShutdownSmokeTest`: asegura que `ThreadMetrics::Snapshot()` y `DrainResult` reflejan el estado publicado en los logs.

## Limitaciones actuales de validación

- Por política operativa **"Never build after changes"**, los agentes de implementación no pueden ejecutar `cmake --build` ni `ctest` inmediatamente después de tocar código.
- QA/verificación debe programar esos comandos en un pipeline autorizado antes de cerrar la fase; hasta entonces, los CR asociados se reportan como WARNING documentado.
- Cuando se levante la restricción, ejecutar `cmake --build . --config Debug` seguido de `ctest --output-on-failure -C Debug` para sellar los refactors descritos en este documento.
