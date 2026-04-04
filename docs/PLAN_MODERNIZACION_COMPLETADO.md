# Plan de Modernización wMule 2.0 — Fases completadas

Este anexo recoge únicamente las fases cerradas del plan vigente. El documento activo (`docs/PLAN_MODERNIZACION_2.0.md`) mantiene las fases en curso y futuras.

## Fase 0 – Baseline y Gobierno

**Objetivo**: Establecer baseline reproducible y reglas mínimas de contribución.

### Tareas
- [x] Congelar estado actual (`wMule 1.0.0`) como baseline funcional (build Debug 2026-03-24, `wmule.exe` + `wmulecmd.exe` verificados).
- [x] Documentar comandos canónicos de build/test (Debug/Release) y dependencias (ver `AGENTS.md`).
- [x] Revisar documentación (`AGENTS.md`, `README.md`, `PLAN_MODERNIZACION_WINDOWS.md`) para evitar inconsistencias aMule/wMule.
- [x] Activar `do_build_ninja.bat` + `compile_commands.json` (nuevo directorio `build-ninja`, clangd apuntando ahí).
- [x] Definir política de ramas y aprobación para cambios críticos de seguridad (`main` solo vía PR con build + tests verdes, revisiones obligatorias).

### Validación durante la fase
- Compilar tras cambios en scripts/documentación crítica.
- Verificar que `do_build_ninja.bat` genera `compile_commands.json` y que clangd ve los headers.

### Validación obligatoria al cierre
- [x] `cmake --build . --config Debug` (24/03/2026)
- [x] `ctest --output-on-failure -C Debug` (12/12 tests verdes)
- [x] Verificación básica de `wmule.exe` (tamaño 13.9 MB en `src/Debug`)
- [x] Verificación básica de `wmulecmd.exe` (tamaño 1.8 MB en `src/Debug`)
- [x] Documento actualizado (Plan 2.0 + política ramas + notas fase)

### Exit criteria
- Build reproducible en Debug/Release.
- Documentación alineada con estado real.
- Reglas básicas de contribución y seguridad definidas.

### Estado
- Estado actual: `[x] Completada (2026-03-24)`

### Notas / incidencias
- `compile_commands.json` vive en `build-ninja/`; `clangd` configurado vía `.vscode/settings.json`.
- Política de ramas: trabajo en ramas feature, PR obligatorio con `cmake --build` + `ctest` y revisión antes de merge a `main`.

---

## Fase 1 – Seguridad Crítica (Parsing Red/UPnP)

**Objetivo**: Endurecer parsing de red y flujos UPnP sin perder conectividad.

### Tareas
#### 1.1 Parsing de paquetes eD2K/Kad
- [x] **Bloque A (UDP inmediato)**: reforzar `ClientUDPSocket.cpp` para: (a) no leer protocolo/opcode si `packetLen < 2`, (b) rechazar paquetes comprimidos cuya descompresión supere el límite aceptado y (c) aplicar heurísticas claras de logging en `EncryptedDatagramSocket.cpp` sin depender de lecturas fuera de rango. *(24/03/2026: a-b implementados en `ClientUDPSocket.cpp`; `EncryptedDatagramSocket.cpp` ahora descarta y loguea padding/magic/value inválidos sin entregar el paquete aguas arriba. El límite global vive en `ClientUDPGuards.h` → `MAX_KAD_UNCOMPRESSED_PACKET = 128 KiB`.)*
- [x] **Bloque B (Handlers Kad)**: revisar `KademliaUDPListener.cpp` empezando por `AddContact2`, `Process2BootstrapResponse`, `Process2PublishKeyRequest`, `Process2PublishSourceRequest`, `ProcessCallbackRequest` y `ProcessFirewalled2Request`, añadiendo prechecks de tamaño, límites de conteos remotos y rechazo explícito de respuestas no solicitadas. *(24/03/2026: `EnsureKadPayload` + límites de contactos/tags/audio aplicados a todos los handlers verificados; `ProcessSearchResponse`/`Process2SearchResponse` validan tracklist y limitan resultados, `Process2Search*` y `Process2PublishNotesRequest` endurecidos.)*
- [x] Validar longitudes antes de `memcpy`/`Read` en todos los puntos identificados; cuando aplique, reemplazar buffers estáticos por `std::vector<uint8_t>` o `CMemFile` seguro. *(Implementado en todos los handlers Kad.)*
- [x] Rechazar opcodes desconocidos o tamaños fuera de rango con mensajes en logs en lugar de silencios. *(ProcessPacket ahora loguea y descarta opcodes no soportados con contexto de IP/len; las macros de tamaño continúan arrojando excepciones con logs previos.)*
- [x] **Política Kad aceptada**: el tope de descompresión se fija en `MAX_KAD_UNCOMPRESSED_PACKET (128 KiB)` y cualquier paquete que lo supere será descartado.

#### 1.2 Protección contra SSRF / Reflection en UPnP
- [x] Endurecer helpers UPnP para tratar nulos y respuestas malformadas sin crashes. *(24/03/2026: `AssignLanUrlOrClear` valida URLs y `CUPnPService::Execute`, `GetStateVariable`, `Subscribe`, `Unsubscribe` y `OnEventReceived` ahora validan docs/SIDs/URLs; 29/03/2026: eventing/XML legacy retirado al migrar a miniupnpc, manteniendo las mismas validaciones LAN/SSRF.)*
- [x] Validar `LOCATION`, `URLBase`, `SCPDURL`, `controlURL` y `eventSubURL` antes de aceptar una respuesta: solo se admitirán esquemas `http/https`, hosts pertenecientes a la LAN local y puertos válidos. *(24/03/2026: `UPnPUrlUtils` + `AssignLanUrlOrClear` y verificaciones en `AddRootDevice`, `CUPnPService`, `CUPnPDevice`.)*
- [x] Limitar quién puede anunciarse (solo red local), añadir timeouts y máximo de intentos tanto en descubrimiento como en port mapping. *(25/03/2026: añadida lógica de retry con backoff exponencial en `AddPortMappings`, `m_defaultMaxRetries=3`, `m_initialRetryDelayMs=500`, logging de intentos y resultados.)*
- [x] Registrar intentos sospechosos y exponer estado de mapping en UI/logs, garantizando fallback claro si UPnP falla. *(25/03/2026: logs de reintentos y fallos en `AddPortMappings`.)*
- [x] **Política UPnP aceptada**: ignorar cualquier respuesta cuya URL u origen no sea LAN/local incluso si actualmente funciona en routers “permisivos”.

#### 1.3 Tests y métricas
- [x] Crear suite unitaria de payloads válidos/inválidos cubriendo: truncamientos UDP, Kad comprimido sobredimensionado/fallido, handlers Kad con longitudes erróneas y XML/URLs UPnP hostiles. *(24/03/2026: `ClientUDPTest`, `KadPacketGuardsTest`, `KadHandlerFuzzTest` y `UPnPXmlSafetyTest` ejercitan límites de descompresión, ventanas deslizantes y formateo/log para UPnP.)*
- [x] Añadir fuzzing ligero (inputs truncados, extendidos, repetidos) y documentar los casos reproducibles. *(24/03/2026: `KadHandlerFuzzTest` barre tags/ventanas.)*

### Validación durante la fase
- Compilar tras cada cambio en parsing/UPnP.
- Ejecutar tests focalizados de red (p. ej. `ctest -R NetworkFunctionsTest`).
- Verificar manualmente conectividad básica tras cambios grandes.

### Validación obligatoria al cierre
- [x] `cmake --build . --config Debug`
- [x] `ctest --output-on-failure -C Debug`
- [x] Verificación básica de `wmule.exe`
- [x] Verificación básica de `wmulecmd.exe`
- [x] Actualizar estado/documentación

### Exit criteria
- Sin lecturas fuera de rango conocidas.
- Suite de tests de parsing reproducible (incluye casos inválidos).
- UPnP endurecido, con logging/feedback claro y sin pérdida de función.

### Estado
- Estado actual: `[x] Completada (25/03/2026)`

### Notas / incidencias
- 2026-03-24: `ClientUDPSocket.cpp` aplicó el límite de descompresión (128 KiB) y `KademliaUDPListener.cpp` añadió `EnsureKadPayload`, topes de contactos (200), entradas publicadas (64) y tags (64) en múltiples handlers. Build/tests tras esta tanda: `cmake --build . --config Debug` + `ctest -C Debug` (12/12 verdes).
- 2026-03-24: Se factorizaron los guards Kad en `src/kademlia/net/KadPacketGuards.h`, se añadieron pruebas MuleUnit y se reconstruyó toda la matriz de tests (`ctest -C Debug`, 13/13).
- 2026-03-24: Se crearon `UPnPUrlUtils` y `AssignLanUrlOrClear`, se endurecieron los métodos de `CUPnPService` y se añadieron pruebas dedicadas (`UPnPXmlSafetyTest`).
- 2026-03-25: Lógica de retry con backoff exponencial en `UPnPBase.cpp`, límites de descompresión activos y tests pasando (16/16).
- 2026-03-28: Wrapper `wmule_upnp_sdk` con miniupnpc habilitado, telemetría `CUPnPOperationReport` y pruebas `UPnPFeedbackPolicyTest` superadas.

---

## Fase 2 – Rutas, Archivos y Configuración Remota (trabajo completado)

**Estado**: `[x] Completada (2026-04-02)`

**Resumen**: Se cerró el hardening de paths internos/externos y ahora los directorios Incoming/Temp/OS pueden configurarse fuera del `ConfigDir` siempre que superen la normalización (`NormalizeAbsolutePath`, `NormalizeSharedPath`, `NormalizeInternalDir`). Las rutas provenientes de GUI, CLI, EC y tooling legacy pasan por los mismos guards y generan mensajes consistentes cuando se rechazan. La UX documenta las restricciones y explica por qué se produce cada fallback. EC conserva el almacenamiento PBKDF2 y los conectores Web/CLI migran secretos legacy; UPnP se mantiene operativo con `wmule_upnp_sdk` (miniupnpc) y feedback visible en la UI.

**Validaciones obligatorias ejecutadas (02/04/2026)**
- [x] `cmake --build . --config Debug` (matriz ENABLE_UPNP=ON con pruebas MuleUnit nuevas)
- [x] `ctest --output-on-failure -C Debug` (incluyendo suites de traversal, EC y UPnP)
- [x] Verificación básica de `wmule.exe`
- [x] Verificación básica de `wmulecmd.exe`
- [x] Documentación actualizada (`PLAN_MODERNIZACION_2.0.md`, `PLAN_MODERNIZACION_COMPLETADO.md`, `BUILD_MEMORY.md`)

### Deuda técnica derivada

- [ ] Auditoría de cobertura i18n en la GUI: tras habilitar `ENABLE_NLS` y regenerar catálogos, quedan textos visibles en inglés (toolbar principal, etiquetas de preferencias heredadas, mensajes legacy). Necesita un barrido de recursos `muuli_wdr.cpp` + `.po`/`.mo` para alinear `msgid`/`msgstr` y actualizar traducciones faltantes.

### 2.1 Path Traversal & FS Safety
- [x] Crear helper común (p. ej. `NormalizeSharedPath`) que use `wxFileName::Normalize(wxPATH_NORM_ALL | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE)` y verifique `IsRelative` antes de aceptar rutas externas.
- [x] Aplicar el helper a los puntos de entrada ya saneados:
  - [x] `PrefsUnifiedDlg.cpp`: selectores de Temp/Incoming/OS ya normalizan y alertan.
  - [x] `CatDialog.cpp`: selector + validación en OK.
  - [x] `PartFileConvertDlg.cpp`: `wxDirSelector` validado antes de importar.
  - [x] `wxcasprefs.cpp` / `wxcasframe.cpp`: selectores de directorios/ficheros normalizados.
  - [x] `aLinkCreator (alcframe.cpp)`: selectores de fichero normalizados.
  - [x] `PrefsUnifiedDlg.cpp`: selectores de ejecutables (`OnButtonBrowseApplication`).
  - [x] `DownloadQueue.cpp`: (no aplica) no consume rutas compartidas; validaciones resueltas en Prefs/EC/SharedFileList.
  - [x] `SharedFileList.cpp`: defensas al consumir rutas compartidas (descarta entradas inválidas o no absolutas).
  - [x] Web/EC: sanitizar rutas que lleguen desde conectores remotos.
  - [x] Categorías (GUI + EC/Web) solo aceptan rutas bajo Incoming o raíces compartidas declaradas (fallback automático a Incoming si son inválidas).
  - [x] Renombrado (GUI/CLI) restringido a nombres base y `SharedFileList` bloquea movimientos fuera de directorios autorizados.
  - [x] `PartFileConvert` valida directorios a importar y solo permite `deleteSource` dentro de Temp/Incoming.
  - [x] `ED2KLinkParser` y `--config-dir` normalizan rutas absolutas antes de leer/escribir `ED2KLinks` o colecciones.
  - [x] `shareddir_list` se sanitiza al guardarse desde la GUI/EC y solo persiste rutas normalizadas (sin duplicados ni carpetas prohibidas).
- [x] Rechazar explícitamente rutas absolutas que salgan de `ConfigDir/Temp/Incoming` cuando el campo represente ubicaciones internas (p. ej. `.met`, directorios de trabajo) y documentar el límite. *(Helpers `NormalizeInternalDir` + validaciones en UI/CLI/EC; fallback seguro y logs de rechazo.)*
- [x] Añadir MuleUnit tests (`PathTraversalTest`) que cubran `..`, rutas UNC y absolutas Windows/Linux.

### 2.2 Configuración EC / Remota
- [x] Mantener EC desactivado por defecto (`thePrefs::EnableExternalConnections(false)`). *(Implementado en `Preferences.cpp`: EC sólo se activa manualmente o desde `ec-config`, y la UI vuelve a desactivar si no hay password.)*
- [x] Diseñar el formato de credencial seguro + estrategia de migración. *(Ver bloque “Diseño PBKDF2 + migración”.)*
- [x] Reemplazar el almacenamiento legacy (`Cfg_Str_Encrypted`/hash MD5) por el formato PBKDF2 definido.
- [x] Limpiar logs de credenciales/hashes y sustituirlos por mensajes neutros (véase `ExternalConn.cpp:496-499`).
- [x] Añadir aviso y confirmación explícita cuando EC se active manualmente (Prefs + CLI), registrando quién lo activa y por qué. *(`PrefsUnifiedDlg.cpp` y `amule.cpp` muestran advertencia, solicitan motivo y registran usuario + razón antes de habilitar EC.)*
- [x] Actualizar `ExternalConn.cpp`, `ExternalConnector.cpp`, `RemoteConnect.cpp` y `WebInterface.cpp` para que hablen el nuevo formato versionado sin romper compatibilidad (detectar hashes legacy y migrarlos al vuelo).
  - [x] `Preferences.cpp` / `Cfg_Str_Encrypted`: persistir PBKDF2 y migrar entradas legacy al guardar.
  - [x] `ExternalConn.cpp`: validar secretos nuevos, añadir tag `EC_TAG_PBKDF2_PARAMS` y limpiar logs sensibles.
  - [x] `ExternalConnector.cpp`: permitir `--password` plano y recalcular PBKDF2 con los parámetros recibidos; actualizar `remote.conf`.
  - [x] `RemoteConnect.cpp`: consumir `EC_TAG_PBKDF2_PARAMS` y derivar el secreto en memoria antes del handshake.
  - [x] `WebInterface.cpp` / `WebServer.cpp`: adoptar el mismo formato para Admin/Guest, exponer migraciones en log neutro.

#### Diseño PBKDF2 + migración (2026-03-26)
- **Formato almacenado**: `pbkdf2-sha256$<iteraciones>$<salt_hex>$<dk_hex>`.
  - `iteraciones`: entero decimal, valor inicial recomendado `150000`.
  - `salt_hex`: 16 bytes aleatorios codificados en hex (32 caracteres).
  - `dk_hex`: resultado de PBKDF2-HMAC-SHA256 de longitud 32 bytes (64 hex).
- **Compatibilidad**:
  - Cualquier valor que no empiece por `pbkdf2-sha256$` se trata como hash legacy (MD5 hex). Durante carga se marca como `legacy` y se reescribe al nuevo formato en cuanto el usuario confirme preferencias o ejecute `amulecmd --write-config`.
  - Autenticación: el servidor comparará contra `dk_hex`. El handshake EC seguirá enviando un salt aleatorio (`m_passwd_salt`), pero se añaden parámetros PBKDF2 (iteraciones + `salt_hex`) al paquete `EC_OP_AUTH_SALT` para que clientes CLI/Web puedan derivar `dk_hex` en tiempo real. Posteriormente ambos lados harán `MD5(dk_hex.lower() + challengeSalt)` para mantener compatibilidad.
  - Webserver comparte el mismo formato; `WebInterface.cpp` leerá y escribirá los nuevos secretos tal cual.
- **Migración automática**:
  1. Detectar hashes legacy (`^[0-9a-fA-F]{32}$`).
  2. Generar salt PBKDF2 (16 bytes), aplicar PBKDF2 al hash legacy (se usa como “password” temporal) y almacenar el resultado en el nuevo formato.
  3. Cuando el usuario edite la contraseña desde la UI o CLI, repetir PBKDF2 usando el password plano y sobrescribir el archivo de config.
  4. Registrar en log un mensaje neutro “EC password migrated al nuevo formato” sin exponer hashes.
- **Biblioteca**: Crypto++ (PKCS5_PBKDF2_HMAC<SHA256>).
- **Handshake remoto**:
  - `ExternalConn.cpp`: añade `EC_TAG_PBKDF2_PARAMS` con iteraciones y `salt_hex` a la respuesta de `EC_OP_AUTH_SALT`.
  - `RemoteConnect.cpp` / `ExternalConnector.cpp`: si el paquete trae los parámetros, derivan `dk_hex` del password plano y continúan el handshake; si no, retrocompatibilidad total.
- **Persistencia**: `PrefsUnifiedDlg`, `ec-config`, `remote.conf` y `amuleweb.conf` escriben siempre el nuevo formato; el legacy solo se usa para lectura + migración.

### 2.3 UPnP/Servicios externos
- [x] Mantener UPnP habilitado y soportado para conectividad entrante (wrapper `wmule_upnp_sdk` resuelve `miniupnpc` vía vcpkg; `ENABLE_UPNP=ON` documentado).
- [x] Implementar timeouts y retries limitados (backoff exponencial en `CUPnPControlPoint::AddPortMappings`).
- [x] Mostrar en UI/logs el estado real del mapeo (éxito/fallo/pendiente) y métricas de reintentos (resumen `CUPnPOperationReport` + bloque “Último estado UPnP” en Preferencias con botón de reintento).
- [x] Proveer fallback claro cuando no se logra el mapping (mensaje y acción en preferencias + log; supresión automática con política de 15 minutos y opción de forzar reintento).
- [x] Persistir el último resultado de UPnP (timestamp, router, puertos) para evitar repetir la misma operación si sabemos que fallará.

### Notas / incidencias (histórico Fase 2)
- 2026-03-26: `NormalizeSharedPath` se aplica en Prefs (Temp/Incoming/OS), categorías, el conversor de partes, wxCas y aLinkCreator; rutas no absolutas muestran error antes de guardarse.
- 2026-03-26: los conectores EC/Web normalizan Temp/Incoming/Shared y descartan rutas inválidas, dejando registro neutro y manteniendo los valores previos.
- 2026-04-01: nuevas claves `/Logging/LogFilePath` y `/Logging/LogFileSeparator` en `amule.conf`. Defaults `%APPDATA%\wMule\wmule.log` y `;`, con validación absoluta, creación del directorio padre y fallback al default + warning si falla el path configurado.
- [resuelto] 2026-03-30: deuda crítica de build cuando `ENABLE_UPNP=ON` + `BUILD_TESTING=ON`. Se corrigió en el mismo día ajustando `UPnPBase.cpp`, `NormalizeInternalDir` y los tests MuleUnit; `scripts/build-wmule.ps1` volvió a compilar todos los targets con tests.
- 2026-03-31: la preferencia “Enable UPnP for router port forwarding” se inicializa en `ON` y queda documentado que el script oficial también lo fuerza; usuario puede desactivarlo manualmente.
- 2026-03-30 (noche): `ENABLE_UPNP=ON` vuelve a ser default en `scripts/build-wmule.ps1`; miniupnpc recompila y linkea sin errores.
- 2026-03-31 (validación real): `wmule.exe` abre automáticamente los puertos 11122/11125/11137 vía UPnP en router doméstico (HighID/Kad OK, `discovered=4 filtered=0`).
- 2026-03-27: Hardening de rutas integrado y publicado como **wMule 1.0.1** (build + `PathTraversalTest` + smoke test completados antes de etiquetar).

---

## Fase 3 – Robustez x64 y Memoria

**Objetivo**: Eliminar truncaciones y asegurar serialización correcta en x64.

### Tareas
#### 3.1 Casts y punteros
- [x] Sustituir `int` usado como puntero por `intptr_t/uintptr_t`.
- [x] Revisar `Packet.h`, `Tag.h`, `MuleThread.h`, `NetworkFunctions.cpp` y módulos relacionados.

#### 3.2 Alineación y serialización
- [x] Declarar estructuras con `#pragma pack` o `static_assert(sizeof)` según protocolo.
- [x] Documentar layout binario y añadir tests que validen tamaños/alineación.

#### 3.3 Buffers y tamaños
- [x] Cambiar operaciones `sizeof(int)` por `sizeof(type)` apropiado.
- [x] Añadir `static_assert` para garantizar tamaños esperados en x64.
- [x] Revisar `memcpy`/`memmove`/`Read` con tamaños dinámicos.

### Validación durante la fase
- Compilación tras toques en tipos/estructuras.
- Tests específicos de serialización y módulos afectados.

### Validación obligatoria al cierre
- [x] `cmake --build . --config Debug`
- [x] `ctest --output-on-failure -C Debug`
- [x] Verificación básica de `wmule.exe`
- [x] Verificación básica de `wmulecmd.exe`
- [x] Actualizar estado/documentación

### Exit criteria
- Cero truncaciones conocidas.
- Build x64 limpia (sin warnings críticos).
- Tests de serialización/regresión pasando.

### Estado
- Estado actual: `[x] Completada (2026-04-04)`

### Notas / incidencias
- `SearchList`, `amulecmd` y la API EC ya no dependen de `long` para IDs; los headers empaquetados (`Header_Struct`, `ServerMet_Struct`, etc.) tienen `static_assert` que rompen la build si cambia el layout.
- Se documentaron los layouts binarios y se incorporaron `static_assert` + tests para validarlos en `docs/PLAN_MODERNIZACION_2.0.md`.
- El barrido de `memcpy`/`memmove`/`Read` quedó cubierto y se corrigió la única llamada con `sizeof(int)` que faltaba.
- Build Debug + ctest + smoke manual (`wmule.exe`, `wmulecmd.exe`) completados en `K:\wMule\build-ninja` (04/04/2026).
- [Deuda estética] Ajustar la pantalla de búsqueda con parámetros adicionales: el cuadro de “Extensión” queda demasiado alto y necesita corrección visual.

---
