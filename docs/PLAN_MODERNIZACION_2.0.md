# Plan de Modernización wMule 2.0 — Seguridad, Robustez y Escalabilidad

## 0. Contexto y Alcance

- **Base actual**: CMake + vcpkg + MSVC 2022, Windows x64 only, binarios `wmule.exe` (14 MB) y `wmulecmd.exe` (1.8 MB) funcionando, 12/12 tests verdes.
- **Estado**: Modernización sintáctica (TRUE/FALSE, NULL, Types.h), ThreadPool + AsyncSocket en progreso, pero vulnerabilidades críticas pendientes.
- **Objetivo 2.0**: Convertir wMule en una base segura, robusta y mantenible antes de añadir arquitectura REST o migrar a C#.
- **Principios**: Seguridad > estabilidad > arquitectura > nuevas features. Cambios incrementales, compatibilidad de protocolo, pruebas obligatorias.

## Metodología de Ejecución y Seguimiento

### Reglas generales
- No esperar al final de la fase para validar todo: cada cambio relevante debe compilarse y probarse.
- Las fases se cierran solo con evidencias de build/test y actualización documental.

### Validación durante cada fase
- Tras cada cambio relevante: compilar la parte afectada (o todo el proyecto si aplica) y ejecutar tests focalizados.
- Tras cerrar un subbloque: build del proyecto + tests relacionados + anotar incidencias.

### Validación obligatoria al cerrar una fase
- `cmake --build . --config Debug`
- `ctest --output-on-failure -C Debug`
- Verificar arranque/uso básico de `wmule.exe` y `wmulecmd.exe`
- Actualizar este documento con estado real (checkboxes, fechas, notas).

### Convención de estado
- `[ ]` Pendiente
- `[~]` En progreso
- `[x]` Completado
- `[!]` Bloqueado/incidencia

### Documentación mínima por fase
- Registrar fecha de cierre parcial/completo.
- Enumerar validaciones ejecutadas.
- Anotar incidencias abiertas si las hay.

---

## 1. Hoja de Ruta Fases 2.0

| Fase | Objetivo | Resultado esperado |
|------|----------|--------------------|
| 0 | Baseline verificable *(ver anexo de fases completadas)* | Build & tests reproducibles, documentación alineada |
| 1 | Seguridad crítica *(ver anexo de fases completadas)* | Parsing de red endurecido, defaults seguros |
| 2 | Rutas y configuración *(ver anexo de fases completadas)* | Sanitización de archivos y EC endurecido |
| 3 | Robustez x64 | Sin truncaciones ni desbordes dependientes de plataforma |
| 4 | Concurrencia segura | Threading estable, ownership claro |
| 5 | Async incremental | Migración controlada a AsyncSocket con métricas |
| 6 | Refactor arquitectónico | Core desacoplado de GUI/wxWidgets |
| 7 | Calidad y CI | Pipeline Windows estable, cobertura ampliada |
| 8 | Evolución futura | Base lista para REST, GUI nueva o migración parcial |

---

> **Nota**: Las fases completadas (Fase 0, Fase 1 y Fase 2) se trasladaron a `docs/PLAN_MODERNIZACION_COMPLETADO.md` para mantener este documento enfocado en el trabajo activo.

## Fase 2 – Rutas, Archivos y Configuración Remota *(completada)*

**Estado**: `[x] Completada (2026-04-02)`

**Resumen**: Se cerró el hardening de paths internos/externos y ahora los directorios Incoming/Temp/OS pueden configurarse fuera del `ConfigDir` siempre que superen la normalización (`NormalizeAbsolutePath`, `NormalizeSharedPath`, `NormalizeInternalDir`). Las rutas provenientes de GUI, CLI, EC y tooling legacy pasan por los mismos guards y generan mensajes consistentes cuando se rechazan. La UX documenta las restricciones y explica por qué se produce cada fallback. EC conserva el almacenamiento PBKDF2 y los conectores Web/CLI migran secretos legacy; UPnP se mantiene operativo con `wmule_upnp_sdk` (miniupnpc) y feedback visible en la UI.

**Validaciones obligatorias ejecutadas (02/04/2026)**
- [x] `cmake --build . --config Debug` (matriz ENABLE_UPNP=ON con pruebas MuleUnit nuevas)
- [x] `ctest --output-on-failure -C Debug` (incluyendo suites de traversal, EC y UPnP)
- [x] Verificación básica de `wmule.exe`
- [x] Verificación básica de `wmulecmd.exe`
- [x] Documentación actualizada (`PLAN_MODERNIZACION_2.0.md`, `PLAN_MODERNIZACION_COMPLETADO.md`, `BUILD_MEMORY.md`)

Detalle completo del trabajo y de las incidencias resueltas en `docs/PLAN_MODERNIZACION_COMPLETADO.md`.

### Deuda técnica derivada

- [ ] Auditoría de cobertura i18n en la GUI: tras habilitar `ENABLE_NLS` y regenerar catálogos, quedan textos visibles en inglés (toolbar principal, etiquetas de preferencias heredadas, mensajes legacy). Necesita un barrido de recursos `muuli_wdr.cpp` + `.po`/`.mo` para alinear `msgid`/`msgstr` y actualizar traducciones faltantes.

---

## Fase 3 – Robustez x64 y Memoria

**Objetivo**: Eliminar truncaciones y asegurar serialización correcta en x64.

### Tareas
#### 3.1 Casts y punteros
- [ ] Sustituir `int` usado como puntero por `intptr_t/uintptr_t`.
- [ ] Revisar `Packet.h`, `Tag.h`, `MuleThread.h`, `NetworkFunctions.cpp` y módulos relacionados.

#### 3.2 Alineación y serialización
- [ ] Declarar estructuras con `#pragma pack` o `static_assert(sizeof)` según protocolo.
- [ ] Documentar layout binario y añadir tests que validen tamaños/alineación.

#### 3.3 Buffers y tamaños
- [ ] Cambiar operaciones `sizeof(int)` por `sizeof(type)` apropiado.
- [ ] Añadir `static_assert` para garantizar tamaños esperados en x64.
- [ ] Revisar `memcpy`/`memmove`/`Read` con tamaños dinámicos.

### Validación durante la fase
- Compilación tras toques en tipos/estructuras.
- Tests específicos de serialización y módulos afectados.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe`
- [ ] Verificación básica de `wmulecmd.exe`
- [ ] Actualizar estado/documentación

### Exit criteria
- Cero truncaciones conocidas.
- Build x64 limpia (sin warnings críticos).
- Tests de serialización/regresión pasando.

### Estado
- Estado actual: `[ ] Pendiente`

### Notas / incidencias
- Ninguna.

---

## Fase 4 – Concurrencia Segura

**Objetivo**: Estabilizar threading, ownership y apagado.

### Tareas
#### 4.1 Auditoría de data races
- [ ] Revisar `DownloadQueue`, `PartFile`, `ThreadScheduler`, `ClientTCPSocket`.
- [ ] Introducir `std::mutex`/`std::shared_mutex` donde corresponda.
- [ ] Documentar ownership de objetos compartidos.

#### 4.2 Ciclo de vida de threads
- [ ] Documentar `ThreadPool` (enqueue, cancel, shutdown, WaitAll).
- [ ] Garantizar apagado limpio en salida de la app (sin hilos colgando).
- [ ] Añadir mecanismos de cancelación/timeouts donde falten.

#### 4.3 Instrumentación
- [ ] Añadir contadores de tareas activas, máximos, tiempos promedio.
- [ ] Añadir tracing básico para depurar contención.

### Validación durante la fase
- Ejecutar tests de estrés y benchmarks tras cambios (ThreadPoolBenchmark, DownloadBenchmark).
- Revisar manualmente el apagado y tiempos de cierre.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe`
- [ ] Verificación básica de `wmulecmd.exe`
- [ ] Actualizar estado/documentación

### Exit criteria
- Sin data races conocidas.
- Tests de estrés/benchmarks pasando sin regresiones.
- Apagado limpio y documentado.

### Estado
- Estado actual: `[ ] Pendiente`

### Notas / incidencias
- Ninguna.

---

## Fase 5 – Async Incremental

**Objetivo**: Migrar gradualmente flujos a AsyncSocket con métricas y control.

### Tareas
- [ ] Elegir flujo crítico acotado (p.ej. lecturas en `ClientTCPSocket`).
- [ ] Integrar `AsyncSocket` con límites, timeouts y backpressure.
- [ ] Añadir telemetría/métricas para latencia y throughput.
- [ ] Comparar resultados con benchmarks existentes (`ThreadPoolBenchmark`, `DownloadBenchmark`).
- [ ] Documentar fallback/no regression path.

### Validación durante la fase
- Ejecutar benchmarks tras cada iteración significativa.
- Tests funcionales del flujo migrado.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe`
- [ ] Verificación básica de `wmulecmd.exe`
- [ ] Actualizar estado/documentación

### Exit criteria
- 1-2 rutas migradas y estables.
- Métricas objetivas antes/después.
- Sin regresiones funcionales.

### Estado
- Estado actual: `[ ] Pendiente`

### Notas / incidencias
- Ninguna.

---

## Fase 6 – Refactor Arquitectónico

**Objetivo**: Reorganizar el código para reducir acoplamiento y preparar evoluciones.

### Tareas
#### 6.1 Modularización
- [ ] Reorganizar en `core/`, `protocol/`, `network/`, `storage/`, `gui/`.
- [ ] Definir interfaces para config, logging, sockets, filesystem.

#### 6.2 Desacoplar GUI/Core
- [ ] Evitar incluir wxWidgets en el core salvo adaptadores específicos.
- [ ] Preparar core para futuras GUIs/servicios.

#### 6.3 Deuda técnica
- [ ] Eliminar duplicados (ej. `SortProc`).
- [ ] Unificar utilidades comunes.
- [ ] Documentar dependencias cruzadas.

### Validación durante la fase
- Compilar targets modulares tras cada reorganización.
- Ejecutar tests relevantes para módulos tocados.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe`
- [ ] Verificación básica de `wmulecmd.exe`
- [ ] Actualizar estado/documentación

### Exit criteria
- Core compilable con dependencias mínimas.
- Límites claros entre módulos.
- Documentación y diagramas actualizados.

### Estado
- Estado actual: `[ ] Pendiente`

### Notas / incidencias
- Ninguna.

---

## Fase 7 – Calidad, Tests y CI

**Objetivo**: Aumentar cobertura de pruebas y automatizar validaciones.

### Tareas
- [ ] Extender MuleUnit con casos de seguridad, rutas y concurrencia.
- [ ] Añadir tests de integración para networking y descarga.
- [ ] Configurar CI Windows (GitHub Actions o AzDO) con `cmake --build` + `ctest`.
- [ ] Añadir análisis estático (clang-tidy/MSVC /analyze) y tratar warnings críticos.
- [ ] Documentar políticas de calidad (p.ej. no merges sin CI verde).

### Validación durante la fase
- Ejecutar nuevos tests localmente antes de subir cambios.
- Validar pipeline CI al menos en un entorno de staging.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe`
- [ ] Verificación básica de `wmulecmd.exe`
- [ ] CI Windows ejecutándose en cada PR/merge
- [ ] Actualizar estado/documentación

### Exit criteria
- Pipeline estable.
- Cobertura significativa de módulos críticos.
- Análisis estático integrado en el flujo.

### Estado
- Estado actual: `[ ] Pendiente`

### Notas / incidencias
- Ninguna.

---

## Fase 8 – Evolución Futura

**Objetivo**: Explorar nuevas arquitecturas y tecnologías sobre un core ya estable.

### Opciones a evaluar
- [ ] API REST (core como servicio HTTP, clientes GUI/CLI/Web).
- [ ] Separación cliente-servidor real con protocolos modernos.
- [ ] Nueva GUI (WPF/WinUI) sobre core existente.
- [ ] Migración progresiva a .NET para módulos específicos.

### Condición de inicio
- Solo iniciar esta fase cuando las fases 1-7 estén cerradas y el core sea estable.

### Validación durante la fase
- Prototipos controlados, con medición de impacto.
- Evaluaciones de compatibilidad y esfuerzo.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe` / `wmulecmd.exe`
- [ ] Documentación actualizada describiendo la nueva arquitectura.

### Estado
- Estado actual: `[ ] Pendiente`

### Notas / incidencias
- Ninguna.

---

## Riesgos y Mitigaciones

| Riesgo | Mitigación |
|--------|------------|
| Regresiones al endurecer parsing | Tests unitarios + fuzzing + métricas binarias |
| Romper compatibilidad del protocolo | Documentar cambios, mantener wire format, usar flags |
| Mezcla de cambios grandes en paralelo | Gobernanza clara, ramas por fase, revisiones enfocadas |
| Migración async antes de tiempo | Limitar alcance, medir y comparar con baseline |

---

## Próximos Pasos Inmediatos

# 1. Completar subbloques críticos de **Fase 2** (normalización de rutas externas, endurecimiento de EC y feedback visible de UPnP) aplicando `NormalizeAbsolutePath`/helpers similares en todos los puntos de entrada.
2. Integrar `ClientUDPTest`, `KadHandlerFuzzTest`, `KadPacketGuardsTest`, `UPnPUrlUtilsTest` y `UPnPXmlSafetyTest` en `unittests/tests/CMakeLists.txt` para que `ctest` vuelva a reflejar la cobertura real.
3. Reejecutar builds/tests completos en matrices `ENABLE_UPNP=OFF/ON` tras cada bloque para garantizar que los fixes de CMake/libupnp se mantienen.
4. Actualizar `docs/BUILD_MEMORY.md` y este plan al cerrar cada hito parcial para dejar constancia de incidencias y validaciones.

---

## Anexos y Referencias

- `docs/history/Auditoria_Seguridad_aMule.md` – lista de vulnerabilidades y soluciones sugeridas.
- `docs/history/PLAN_MODERNIZACION_WINDOWS.md` – estado de fases 1-4.
- `AGENTS.md` – comandos de build/test y estilo de código.
- `CHANGELOG.md` – historial de versiones y tareas futuras (incluye header simplificado pendiente).
