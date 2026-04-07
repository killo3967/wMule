# Logging reclassification debt

## Alcance actual
- Solo afecta a `wmule.log`.
- El objetivo inmediato es que los niveles reflejen la intención real del mensaje.

## Reglas aplicadas
- `INFO`: estado normal, arranque y transiciones esperadas.
- `WARNING`: validaciones que caen a fallback, rutas fuera de base, UPnP con problemas parciales.
- `ERROR`: fallos reales de operación.
- `CRITICAL`: reservado para errores graves o crashes.

## Clasificación a revisar
- `Kad started.` → `INFO`
- `Kad stopped.` → `INFO`
- `Connected to Kad (ok)` → `INFO`
- `Connected to Kad (firewalled)` → `WARNING`
- `Disconnected from Kad` → `INFO`
- `Connected to eD2k ...` / `Disconnected from eD2k` → `INFO`
- `Thread shutdown token activated` / `ShutdownSummary ...` → `INFO`
- `Protocol not supported for HTTP download: https` → `WARNING`
- `Failed to download the version check file` → `WARNING`
- `Connecting` (IPFilter startup) → `INFO`
- `7 servers in server.met found` → `INFO`
- `Destroy() already dying socket ...` → `INFO`/`DEBUG` (candidate for future cleanup)
- `UPnP` summary con `fail/suppressed` → `WARNING`

## Deuda técnica futura
- Unificar todo el sistema de logging.
- Pasar todos los mensajes de `wmule.log` a inglés de forma consistente.
- Revisar categorías/módulos para reducir ruido y falsos `CRITICAL`.
