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
| 3 | Robustez x64 *(ver anexo de fases completadas)* | Sin truncaciones ni desbordes dependientes de plataforma |
| 4 | Concurrencia segura | Threading estable, ownership claro |
| 5 | Async incremental | Migración controlada a AsyncSocket con métricas |
| 6 | Refactor arquitectónico | Core desacoplado de GUI/wxWidgets |
| 7 | Calidad y CI | Pipeline Windows estable, cobertura ampliada |
| 8 | Evolución futura | Base lista para REST, GUI nueva o migración parcial |

---

## Bloque transversal priorizado – Evolución del protocolo de control remoto

- **Identificador**: `BT-EC-PBgRPC`
- **Estado**: `[ ] Pendiente`
- **Ventana recomendada**: iniciar auditoría y modelado durante Fase 3; ejecutar extracción de seams, infraestructura mínima y validación paralela en coordinación con Fase 6 y Fase 7.
- **Resultado esperado**: inventario técnico/funcional completo de EC v2 basado en código real, matriz de equivalencia EC v2 → PBgRPC, prototipo funcional de `PBgRPC` coexistiendo con EC v2 y cliente `wmulecmdv2` para validación paralela sin romper `wmulecmd.exe`, `amuleweb` ni integraciones existentes.

### Título

**Auditoría EC v2 y transición conservadora a PBgRPC**

### Objetivo

Auditar en profundidad el protocolo External Connect (EC) heredado de aMule/wMule, preservar **EC v2** plenamente funcional durante toda la transición y diseñar/validar una nueva línea de control remoto basada en **Protocol Buffers + gRPC** (**PBgRPC**) que reproduzca las capacidades reales del protocolo actual, con un cliente de pruebas separado (`wmulecmdv2`) para comparar comportamiento sin contaminar el canal legacy.

### Justificación técnica

- El protocolo EC actual sigue siendo una pieza crítica de operación remota: `wmulecmd`, `amuleweb` y cualquier cliente enlazado con `src/libs/ec/cpp` dependen del mismo modelo de opcodes, tags y notificaciones (`src/ExternalConn.cpp`, `src/ExternalConnector.cpp`, `src/TextClient.cpp`, `src/webserver/src/WebServer.cpp`).
- La implementación real está **fuertemente acoplada** al core monolítico mediante `theApp`, `thePrefs`, colas, búsquedas, Kad y logging; el switch central de `CECServerSocket::ProcessRequest2` expone directamente operaciones del núcleo (`src/ExternalConn.cpp`).
- El framing, serialización y handshake actuales están definidos en código propio (`src/libs/ec/cpp/ECSocket.cpp`, `ECPacket.h`, `ECTag.h`, `ECTag.cpp`) y no deben reinterpretarse a partir de documentación histórica si ésta diverge del comportamiento real.
- La autenticación sigue descansando en reto/respuesta MD5 sobre TCP sin cifrado del canal; wMule endureció el **almacenamiento** del secreto con PBKDF2, pero el intercambio de autenticación y las capacidades expuestas siguen siendo esencialmente las del stack legacy. Si la documentación afirma algo más fuerte que eso, **prevalece el código** y esa discrepancia debe quedar documentada explícitamente.
- Antes de aspirar a una GUI externa, un core separado o un adaptador MCP, hace falta una API remota moderna con contrato formal, versionado y streaming bien definidos, pero **sin retirar ni degradar** EC v2 hasta demostrar equivalencia funcional suficiente.

### Alcance

Incluye:

- Auditoría funcional y técnica del protocolo EC v2 basada en el código real heredado de aMule y adaptado en wMule.
- Inventario de opcodes, tags, detalle de paquetes, flujo de autenticación, notificaciones, errores, capacidades cliente y puntos de acoplamiento con el core.
- Identificación explícita de divergencias entre documentación y código, priorizando el comportamiento observado en código fuente.
- Definición de una matriz de equivalencia **EC v2 → PBgRPC** que cubra operaciones síncronas, eventos asíncronos y propiedades del modelo actual.
- Diseño de una API PBgRPC formal (servicios, mensajes, eventos/streams, versionado, autenticación, compatibilidad evolutiva).
- Implementación incremental de infraestructura mínima para ejecutar PBgRPC **en paralelo** a EC v2.
- Clonado controlado de `wmulecmd` a `wmulecmdv2` para validar el nuevo protocolo sin tocar el cliente legacy.
- Estrategia de retirada futura de EC v2 **solo** cuando exista cobertura funcional demostrada, telemetría y pruebas de no regresión suficientes.

No incluye en este bloque:

- Eliminación inmediata de `ExternalConn` o de `src/libs/ec/cpp`.
- Sustitución masiva de clientes existentes en una sola iteración.
- Cambio del wire format de EC v2.
- Reescritura completa del core antes de completar la auditoría y la matriz de equivalencias.

### Subfases

#### BT-EC.1 – Auditoría profunda del protocolo EC v2
- [ ] Inventariar la arquitectura real del stack EC a partir de `src/ExternalConn.cpp`, `src/libs/ec/cpp/*`, `src/ExternalConnector.cpp`, `src/TextClient.cpp`, `src/webserver/src/WebServer.cpp`, `src/amule.cpp`, `src/Preferences*` y cualquier módulo acoplado relevante.
- [ ] Documentar framing, cabecera, flags, compresión, detalle de paquetes, serialización de `CECPacket`/`CECTag`, tipos soportados, jerarquías de tags y límites binarios efectivos.
- [ ] Documentar handshake, autenticación, gestión de sesión, negociación de capacidades (`ZLIB`, `UTF8 numbers`, `notify`) y manejo de errores según código real.
- [ ] Levantar inventario de comandos, respuestas y eventos asíncronos realmente implementados, con trazabilidad a opcodes/tags.
- [ ] Identificar semántica síncrona/asíncrona, correlación implícita entre petición y respuesta, FIFO de peticiones y colas de notificación por cliente.
- [ ] Catalogar capacidades consumidas por `wmulecmd`, `amuleweb` y demás clientes EC existentes.
- [ ] Identificar puntos de acoplamiento con el core, deuda técnica, riesgos de seguridad, límites de compatibilidad y huecos de observabilidad.
- [ ] Generar un inventario funcional/técnico de EC v2 **basado en código real**, con nota explícita de discrepancias entre documentación y comportamiento observado.

#### BT-EC.2 – Preservación explícita y pruebas de no regresión sobre EC v2
- [ ] Declarar EC v2 como protocolo soportado durante toda la transición; no se desmonta, no se elimina y no se sustituye prematuramente.
- [ ] Definir una batería de regresión específica para `wmulecmd.exe`, `amuleweb` y flujos EC críticos antes de introducir PBgRPC.
- [ ] Aislar cambios en `ExternalConn`, `RemoteConnect`, `libec` y adaptadores para evitar contaminación cruzada con el nuevo stack.
- [ ] Introducir instrumentación/telemetría mínima de uso de opcodes y rutas de notificación, sin alterar el wire format existente.
- [ ] Definir el criterio exacto de retirada futura de EC v2 (ver condición de salida al final del bloque) y prohibir el deprecation por intuición o por documentación incompleta.

#### BT-EC.3 – Modelado de equivalencias EC v2 → PBgRPC
- [ ] Construir una matriz de equivalencia que relacione opcodes, tags, eventos y semánticas de EC v2 con servicios, RPCs, mensajes y streams de PBgRPC.
- [ ] Marcar explícitamente gaps funcionales, ambigüedades semánticas, dependencias internas y decisiones de mapeo que requieran extracción de seams en el core.
- [ ] Separar en la matriz: operaciones request/response, eventos push, suscripciones, cambios de estado, errores y metadatos de compatibilidad/versionado.
- [ ] Identificar capacidades mínimas obligatorias para la primera iteración de PBgRPC y capacidades diferidas para iteraciones posteriores.

#### BT-EC.4 – Diseño formal de PBgRPC
- [ ] Definir el contrato formal de `proto` para servicios, mensajes, enums, errores y metadatos de versión.
- [ ] Diseñar RPCs síncronas equivalentes a las operaciones EC críticas y streams de servidor equivalentes a los eventos/notificaciones asíncronas.
- [ ] Establecer separación limpia entre servicios de control, estado, colas, búsquedas, servidores/redes, preferencias y eventos.
- [ ] Diseñar estrategia de autenticación/autorización y transporte seguro apropiada para Windows 11 y despliegue local/LAN, evitando repetir las debilidades del canal EC legacy.
- [ ] Definir versionado de contrato, compatibilidad evolutiva, manejo de campos opcionales y política de backward/forward compatibility.
- [ ] Diseñar PBgRPC orientado a que wMule pueda evolucionar hacia un core desacoplado con GUI externa y a que exista un adaptador MCP por encima del nuevo protocolo sin exponer internals legacy.

#### BT-EC.5 – Infraestructura mínima en paralelo
- [ ] Introducir seams internos para reutilizar operaciones del core sin pasar por el switch monolítico de EC en cada nueva integración.
- [ ] Implementar un listener/host PBgRPC **en paralelo**, con puerto/configuración independientes de EC v2.
- [ ] Mantener el listener EC actual intacto salvo cambios defensivos, métricas o adaptaciones internas estrictamente necesarias para compartir servicios.
- [ ] Añadir trazas y métricas para comparar cobertura funcional, latencia, errores y uso de ambos canales durante la convivencia.

#### BT-EC.6 – Clonado de `wmulecmd` a `wmulecmdv2`
- [ ] Clonar la base de `wmulecmd` a un nuevo binario/target `wmulecmdv2` sin modificar el comportamiento del cliente legacy.
- [ ] Separar el código común reutilizable (parsing CLI, formato de salida, utilidades) del transporte/protocolo para evitar contaminación entre EC v2 y PBgRPC.
- [ ] Implementar una batería mínima inicial de comandos sobre PBgRPC: estado básico, conectar/desconectar, cola de descargas, cola de subidas, pausa/reanudación/cancelación, añadir enlace, búsqueda básica/resultados y logging/diagnóstico mínimo.
- [ ] Añadir comparativas funcionales EC v2 vs PBgRPC para cada comando soportado por `wmulecmdv2` antes de ampliar cobertura.
- [ ] Definir cobertura mínima obligatoria antes de crecer el cliente: no ampliar comandos sin haber cerrado equivalencia y pruebas del subconjunto ya soportado.

#### BT-EC.7 – Validación paralela y decisión de migración
- [ ] Ejecutar pruebas paralelas `wmulecmd` (EC v2) vs `wmulecmdv2` (PBgRPC) sobre escenarios equivalentes y datasets comparables.
- [ ] Validar que los eventos y estados observables entregados por PBgRPC reproducen correctamente el comportamiento funcional de EC v2 o lo mejoran sin pérdida de información necesaria.
- [ ] Documentar diferencias aceptadas, gaps pendientes y bloqueos que impidan migraciones mayores.
- [ ] Mantener cualquier decisión de migración amplia fuera de este bloque hasta disponer de evidencias de equivalencia suficientes.

### Dependencias

- **Entrada mínima**: Fase 2 completada, porque el endurecimiento previo de configuración remota y credenciales EC deja una base más segura para auditar el comportamiento actual.
- **Dependencia fuerte para diseño/implementación**: avances de Fase 6 para extraer seams internos y reducir el acoplamiento directo del protocolo remoto con `theApp`, `thePrefs` y estructuras monolíticas.
- **Dependencia fuerte para validación sostenida**: Fase 7 o, como mínimo, un subconjunto operativo de su infraestructura de pruebas/automatización para ejecutar regresión paralela EC v2 vs PBgRPC.
- **Dependencia técnica transversal**: cualquier cambio de serialización, tipos o tamaños derivado de Fase 3 debe reflejarse tanto en la auditoría de EC v2 como en los contratos iniciales de PBgRPC.

### Riesgos

- **Riesgo de falsa equivalencia**: diseñar PBgRPC solo a partir de documentación histórica y omitir capacidades reales que sí usa el código actual.
- **Riesgo de regresión en clientes existentes**: romper `wmulecmd`, `amuleweb` o integraciones de terceros al tocar `ExternalConn`, `RemoteConnect` o tablas de tags/opcodes.
- **Riesgo de acoplamiento oculto**: descubrir tarde dependencias del protocolo con globals, IDs internos, rutas o estructuras del core no preparadas para exposición moderna.
- **Riesgo criptográfico/de transporte**: repetir en PBgRPC un modelo insuficiente de autenticación o dejar el nuevo canal sin una estrategia clara de seguridad y versionado.
- **Riesgo de alcance**: intentar cubrir el 100 % del protocolo en una sola iteración y bloquear el avance por exceso de ambición.
- **Riesgo documental**: mantener afirmaciones optimistas en el plan si el código revela que el endurecimiento efectivo es menor del esperado.
- **Riesgo de contaminación del cliente**: reutilizar `wmulecmd` sin separación suficiente y acabar rompiendo el binario original o mezclando UX/transporte.

### Mitigaciones

- Usar **el código** como fuente primaria y registrar explícitamente cada discrepancia relevante con la documentación.
- Mantener EC v2 congelado a nivel de wire format y encapsular cambios en seams internos o adaptadores, no en el contrato legacy.
- Introducir telemetría de uso de opcodes/eventos antes de planificar retirada o priorización de cobertura.
- Priorizar un subconjunto funcional crítico para la primera iteración de PBgRPC y cerrar equivalencia antes de ampliar alcance.
- Separar `wmulecmdv2` como target y código de transporte independientes desde el día 1.
- Requerir pruebas de no regresión sobre EC v2 en cada cambio que toque el plano remoto.
- Exigir revisión técnica específica de seguridad y compatibilidad antes de abrir el nuevo listener a más escenarios que localhost/LAN controlada.

### Entregables

- Documento de auditoría técnica/funcional de EC v2 basado en código real, con referencias a archivos, opcodes, tags, flujos y clientes afectados.
- Inventario de capacidades reales de `wmulecmd`, `amuleweb` y otros consumidores EC relevantes.
- Matriz de equivalencia **EC v2 → PBgRPC** con gaps, decisiones de mapeo y prioridades.
- Definición inicial de servicios, mensajes y streams de PBgRPC (`.proto` + notas de compatibilidad/versionado).
- Diseño de seguridad/autenticación/transporte para PBgRPC.
- Target `wmulecmdv2` funcional sobre PBgRPC con batería mínima de comandos y comparativas paralelas frente a `wmulecmd`.
- Plan de convivencia, métricas y criterio formal de retirada futura de EC v2.

### Criterios de aceptación

- Existe una auditoría reproducible de EC v2 que cubre, como mínimo: arquitectura, framing, handshake, autenticación, sesión, comandos, respuestas, eventos, semántica síncrona/asíncrona, serialización, errores, compatibilidad entre versiones, capacidades reales de clientes, acoplamientos y riesgos.
- La auditoría identifica expresamente cualquier discrepancia entre documentación y código, dejando constancia de que el código prevalece para la planificación.
- EC v2 permanece operativo durante toda la ejecución del bloque; `wmulecmd.exe` y `amuleweb` siguen funcionando sobre el protocolo actual.
- Existe una matriz EC v2 → PBgRPC suficientemente detallada para justificar que el nuevo protocolo reproduce las capacidades funcionales del actual.
- El diseño de PBgRPC incluye servicios, mensajes, streams, versionado, autenticación y estrategia de compatibilidad evolutiva.
- `wmulecmdv2` puede ejecutar la batería mínima acordada exclusivamente sobre PBgRPC y compararse de forma objetiva con `wmulecmd`.
- No se autoriza ninguna retirada de EC v2 sin cumplir la condición de salida definida en este bloque.

### Estrategia de pruebas

- **Pruebas de auditoría**: construir fixtures/capturas o tests de integración que validen handshake, flags, opcodes críticos, tags y eventos sobre el EC real implementado.
- **Pruebas de no regresión EC v2**: ejercitar `wmulecmd.exe`, `amuleweb` y operaciones críticas (estado, colas, búsquedas, servidores, preferencias, logs) tras cada cambio en el plano remoto.
- **Pruebas de equivalencia**: ejecutar el mismo escenario funcional mediante EC v2 y PBgRPC y comparar respuesta observable, errores, latencia básica y eventos emitidos.
- **Pruebas de streaming**: validar suscripción, reconexión, orden relativo razonable, backpressure y cancelación de eventos en PBgRPC.
- **Pruebas de compatibilidad evolutiva**: verificar que el contrato protobuf acepta ampliaciones compatibles sin romper clientes anteriores del nuevo protocolo.
- **Pruebas de seguridad**: validar autenticación, rechazos, timeouts, límites de tamaño, exposición LAN/localhost y endurecimiento del listener nuevo.
- **Pruebas de cliente**: comparar salida funcional de `wmulecmd` y `wmulecmdv2` para el subconjunto cubierto antes de ampliar comandos.

### Impacto en arquitectura

- Obliga a introducir un seam explícito entre el core y el plano de control remoto, reduciendo dependencia directa del protocolo con globals y handlers monolíticos.
- Empuja la evolución hacia servicios reutilizables por GUI, CLI, web y futuros adaptadores externos sin forzar todavía una reescritura completa del core.
- Sienta la base para una futura separación core/GUI y para un adaptador MCP sobre PBgRPC, siempre que el nuevo contrato no filtre detalles internos innecesarios del modelo legacy.

### Impacto en compatibilidad

- **Compatibilidad obligatoria durante la transición**: EC v2 se mantiene soportado y funcional; no se rompe wire format ni se elimina soporte para `wmulecmd`, `amuleweb` o integraciones existentes.
- PBgRPC nace como canal adicional, no como reemplazo inmediato.
- Cualquier diferencia funcional detectada entre EC v2 y PBgRPC debe registrarse y tratarse como gap, limitación conocida o mejora explícitamente aprobada; no se asume equivalencia por similitud superficial.

### Condición de salida para futura retirada de EC v2

EC v2 **solo podrá plantearse como retirado o deprecado** cuando se cumplan simultáneamente todas estas condiciones:

- [ ] Existe auditoría cerrada y actualizada de EC v2 basada en código real.
- [ ] La matriz de equivalencia EC v2 → PBgRPC cubre el 100 % de las capacidades funcionales consideradas obligatorias para clientes actuales.
- [ ] `wmulecmdv2` demuestra equivalencia funcional suficiente frente a `wmulecmd` en la batería mínima y en las ampliaciones aprobadas.
- [ ] Hay evidencia de no regresión en `wmulecmd`, `amuleweb` y demás integraciones críticas durante la convivencia.
- [ ] La telemetría confirma qué partes de EC v2 siguen usándose realmente y existe plan de migración para esos consumidores.
- [ ] El nuevo canal PBgRPC dispone de autenticación, versionado, streaming y estrategia de compatibilidad validados.
- [ ] La decisión de retirada queda documentada en el plan, con riesgos residuales, ventana de transición y fallback explícito.

### Notas / cuestiones abiertas

- El plan actual ya afirma que EC quedó endurecido en Fase 2, pero el código real muestra que el canal sigue usando handshake legacy con reto MD5 y TCP sin cifrado; PBKDF2 protege el secreto almacenado, no transforma por sí mismo el modelo de transporte/autenticación. Esta discrepancia debe seguir visible hasta que se corrija documentalmente y/o se implante el nuevo canal.
- La cobertura real necesaria para declarar equivalencia no debe decidirse por intuición: debe salir del inventario de opcodes/tags usados por código y por clientes reales.

---

> **Nota**: Las fases completadas (Fase 0, Fase 1 y Fase 2) se trasladaron a `docs/PLAN_MODERNIZACION_COMPLETADO.md` para mantener este documento enfocado en el trabajo activo.

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
- El último build MSVC (04/04/2026) arrojó un bloque consistente de warnings que quedan como deuda de la fase:
  - `wxPATH_NORM_ALL` está deprecado; nuestros helpers en `src/libs/common/Path.cpp` deben reemplazarlo por la combinación explícita de flags (`wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | …`).
  - `src/libs/common/StringFunctions.h` sigue usando `strcpy`/`strncpy` y dispara C4996 cada vez que se incluye. Hay que migrar a las variantes `_s`, encapsular en helpers o desactivar el warning localmente.
  - `BTList` se declara como `struct` en `unittests/muleunit/test.h` pero los `.cpp` la vuelven a declarar como `class`, generando C4099 en todos los tests.
  - `FormatTest.cpp` usa literales narrow para `wxChar` y provoca warnings de truncamiento (C4305/C4309); también hay variables `e/err` sin usar en `SafeFile.cpp` y `LibSocketAsio.cpp` (C4101) que conviene limpiar.
  - `LibSocketAsio.cpp` depende de APIs de Boost.Asio marcadas como deprecadas (`deadline_timer`, `null_buffers`, `io_context::strand::wrap`). La migración a `basic_waitable_timer`/`bind_executor` requiere un subtask dedicado (probable alcance Fase 5 o 6).
- 04/04/2026 — Se eliminó el lote de conversiones inseguras (`uint32→uint8/uint16`) en `BaseClient`, `ClientList`, `ClientTCPSocket`, `DownloadClient`, `PartFile`, `Preferences`, `Server`, `ServerSocket`, `UploadClient` y helpers asociados. Los contadores clave (`GetValidSourcesCount`, `SetRemoteQueueRank`, `m_nSumForAvgUpDataRate`, puertos, etc.) ahora operan con tipos consistentes y clamps explícitos; build/tests completaron sin nuevos warnings propios.
- 04/04/2026 — Se congelaron los lexers generados por flex (`Scanner.cpp`, `IPFilterScanner.cpp`) y se introdujo la opción `WMULE_USE_FLEX` para regenerarlos solo bajo demanda. Esto evita que MSVC vuelva a introducir warnings por `register`/`strdup` cuando se construye el proyecto por defecto.

---

## Fase 5 – Async Incremental

**Objetivo**: Migrar gradualmente flujos a AsyncSocket con métricas y control.

### Tareas
- [ ] Diseñar la migración de `LibSocketAsio` fuera de las APIs de Boost.Asio deprecadas (`deadline_timer`, `null_buffers`, `io_context::strand::wrap`), definiendo cómo y cuándo adoptar `basic_waitable_timer`/`bind_executor` dentro del alcance de esta fase.
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

1. Iniciar `BT-EC-PBgRPC` por **BT-EC.1**: auditar el stack EC v2 a partir de `src/ExternalConn.cpp`, `src/libs/ec/cpp/*`, `src/ExternalConnector.cpp`, `src/TextClient.cpp` y `src/webserver/src/WebServer.cpp`, dejando constancia explícita de divergencias entre documentación y código.
2. Preparar la matriz de equivalencia EC v2 → PBgRPC antes de tocar wire formats, listeners o clientes existentes.
3. Definir el clonado controlado de `wmulecmd` a `wmulecmdv2` como target independiente para validar PBgRPC sin riesgo sobre `wmulecmd.exe`.
4. Mantener la disciplina de validación del plan: actualizar este documento y la documentación complementaria al cerrar cada subfase parcial con evidencias de pruebas y compatibilidad.

---

## Anexos y Referencias

- `docs/history/Auditoria_Seguridad_aMule.md` – lista de vulnerabilidades y soluciones sugeridas.
- `docs/history/PLAN_MODERNIZACION_WINDOWS.md` – estado de fases 1-4.
- `AGENTS.md` – comandos de build/test y estilo de código.
- `CHANGELOG.md` – historial de versiones y tareas futuras (incluye header simplificado pendiente).
