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
| 0 | Baseline verificable | Build & tests reproducibles, documentación alineada |
| 1 | Seguridad crítica | Parsing de red endurecido, defaults seguros |
| 2 | Rutas y configuración | Sanitización de archivos y EC endurecido |
| 3 | Robustez x64 | Sin truncaciones ni desbordes dependientes de plataforma |
| 4 | Concurrencia segura | Threading estable, ownership claro |
| 5 | Async incremental | Migración controlada a AsyncSocket con métricas |
| 6 | Refactor arquitectónico | Core desacoplado de GUI/wxWidgets |
| 7 | Calidad y CI | Pipeline Windows estable, cobertura ampliada |
| 8 | Evolución futura | Base lista para REST, GUI nueva o migración parcial |

---

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
- Fase 0 completada (2026-03-24):
  - `cmake --build . --config Debug` recompiló todo y generó `src/Debug/wmule.exe` (13.9 MB) y `wmulecmd.exe` (1.8 MB).
  - `ctest --output-on-failure -C Debug` pasó 12/12 tests.
  - Documentación actualizada (`PLAN_MODERNIZACION_2.0.md`, `AGENTS.md`) con metodología, checkboxes y política de ramas.
  - `do_build_ninja.bat` crea `build-ninja/` y genera `compile_commands.json`; `.vscode/settings.json` apunta a ese directorio para clangd.
- 2026-03-24: `ClientUDPSocket.cpp` aplicó el límite de descompresión (128 KiB) y `KademliaUDPListener.cpp` añadió `EnsureKadPayload`, topes de contactos (200), entradas publicadas (64) y tags (64) en `AddContact2`, `Process2BootstrapResponse`, `Process2Publish*`, `ProcessCallbackRequest` y `ProcessFirewalled*`. `EncryptedDatagramSocket.cpp` descarta paquetes ofuscados con magic/padding inválidos y `ProcessSearchResponse`/`Process2SearchResponse` (más `Process2Search*`) validan tracklist y límites de resultados etiquetados. Build/tests tras esta tanda: `cmake --build . --config Debug` + `ctest -C Debug` (24/03/2026, 12/12 verdes).
- 2026-03-24 (tarde): Se factorizaron los guards Kad en `src/kademlia/net/KadPacketGuards.h`, se añadieron pruebas MuleUnit (`KadPacketGuardsTest`) para `EnsureKadPayload`/`EnsureKadTagCount`, y se reconstruyeron `wmule`, `wmulecmd` y toda la matriz de tests (`cmake --build . --config Debug`, `ctest -C Debug`, 13/13). `PLAN_MODERNIZACION_2.0.md` actualizado con el nuevo alcance del Bloque B.
- 2026-03-24 (ClientUDPGuards): se movieron los límites de payload comprimido/descomprimido y los helpers de validación a `src/ClientUDPGuards.h` para compartir política entre `ClientUDPSocket` y MuleUnit (`ClientUDPTest`).
- 2026-03-24 (noche): `Process2HelloResponseAck`, `ProcessKademlia2Request`, `ProcessKademlia2Response`, `ProcessFindBuddyRequest/Response` y `Process2Search*` reciben `KadPacketGuards`, se añadieron más casos en `KadPacketGuardsTest` y se validó con build + `ctest -C Debug` (13/13).
- 2026-03-24 (UPnP SSRF): se crearon `UPnPUrlUtils` y `AssignLanUrlOrClear`, se validó URLBase/SCPD/control/event/presentation antes de registrar dispositivos/servicios, y se endurecieron `CUPnPService::Execute`, `GetStateVariable`, `Subscribe`/`Unsubscribe`, `OnEventReceived` y `AddRootDevice`. Build/tests: `cmake --build . --config Debug`, `ctest -C Debug` (14/14).
- 2026-03-24 (Tests parsing): se añadieron `ClientUDPTest`, `KadHandlerFuzzTest` y `UPnPXmlSafetyTest` (este último solo se compila con `ENABLE_UPNP=ON`) para cubrir límites de descompresión, ventanas de payloads y feedback UPnP. Build: `cmake --build . --config Debug` (ctest parcial: `KadHandlerFuzzTest`, `ClientUDPTest`).
- 2026-03-24 (UPnP ENABLE_UPNP=ON): se filtraron las rutas erróneas que `UPNP::Shared`/`IXML::Shared` exportaban (`.../COMPONENT`, `.../UPNP_Development`), se añadió la dependencia explícita a `PThreads4W` y se reconstruyó con `cmake .. -DENABLE_UPNP=ON` + `cmake --build . --config Debug`. Los tests focalizados `ctest -R "ClientUDPTest|KadHandlerFuzzTest|KadPacketGuardsTest|UPnPUrlUtilsTest|UPnPXmlSafetyTest" -C Debug` quedaron en verde.
- 2026-03-29 (UPnP miniupnpc): migrado el backend a `miniupnpc` con seam `IUPnPClient`, eliminado eventing/XML (`UPnPEventLog`, callbacks libupnp) y conservada la política LAN/SSRF. Tests MuleUnit actualizados (`UPnPXmlSafetyTest`, `UPnPUrlUtilsTest`).
- 2026-03-26: los nuevos binarios MuleUnit (`ClientUDPTest`, `KadHandlerFuzzTest`, `KadPacketGuardsTest`, `UPnPUrlUtilsTest`, `UPnPXmlSafetyTest`) se añadieron a `unittests/tests/CMakeLists.txt`. `ctest` los ejecuta (salvo `UPnPXmlSafetyTest`, que solo se compila con `ENABLE_UPNP=ON`).
- 2026-03-25: Fase 1 completada. Añadida lógica de retry con backoff exponencial en `UPnPBase.cpp` (`m_defaultMaxRetries=3`, `m_initialRetryDelayMs=500`), validación de payloads en todos los handlers Kad verificada, límites de descompresión (128 KiB) activos, y tests pasando (16/16). Build: `cmake --build . --config Debug` + `ctest -C Debug`.
- 2026-03-28 (UPnP feedback): habilitado `ENABLE_UPNP=ON` con wrapper `wmule_upnp_sdk` (targets `UPNP::Shared/LibUPNP::LibUPNP` + PThreads4W), nueva telemetría `CUPnPOperationReport` con persistencia `CUPnPLastResult`, supresión de reintentos equivalentes <15 min, reintento manual por ámbito en Preferencias y pruebas MuleUnit (`UPnPFeedbackPolicyTest`).

---

## Fase 2 – Rutas, Archivos y Configuración Remota

**Objetivo**: Eliminar path traversal, endurecer EC y ofrecer UPnP seguro con feedback claro.

### Tareas
#### 2.1 Path Traversal & FS Safety
- [x] Crear helper común (p. ej. `NormalizeSharedPath`) que use `wxFileName::Normalize(wxPATH_NORM_ALL | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE)` y verifique `IsRelative` antes de aceptar rutas externas.
- [ ] Aplicar el helper a todos los puntos de entrada de paths externos.
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

#### 2.2 Configuración EC / Remota
- [x] Mantener EC desactivado por defecto (`thePrefs::EnableExternalConnections(false)`). *(Implementado en `Preferences.cpp`: EC sólo se activa manualmente o desde `ec-config`, y la UI vuelve a desactivar si no hay password.)*
- [x] Diseñar el formato de credencial seguro + estrategia de migración. *(Ver sección “Diseño PBKDF2 + migración”.)*
- [x] Reemplazar el almacenamiento legacy (`Cfg_Str_Encrypted`/hash MD5) por el formato PBKDF2 definido.
- [x] Limpiar logs de credenciales/hashes y sustituirlos por mensajes neutros (véase `ExternalConn.cpp:496-499`).
- [x] Añadir aviso y confirmación explícita cuando EC se active manualmente (Prefs + CLI), registrando quién lo activa y por qué. *(`PrefsUnifiedDlg.cpp` y `amule.cpp` muestran advertencia, solicitan motivo y registran usuario + razón antes de habilitar EC.)*
- [x] Actualizar `ExternalConn.cpp`, `ExternalConnector.cpp`, `RemoteConnect.cpp` y `WebInterface.cpp` para que hablen el nuevo formato versionado sin romper compatibilidad (detectar hashes legacy y migrarlos al vuelo).
  - [x] `Preferences.cpp` / `Cfg_Str_Encrypted`: persistir PBKDF2 y migrar entradas legacy al guardar.
  - [x] `ExternalConn.cpp`: validar secretos nuevos, añadir tag `EC_TAG_PBKDF2_PARAMS` y limpiar logs sensibles.
  - [x] `ExternalConnector.cpp`: permitir `--password` plano y recalcular PBKDF2 con los parámetros recibidos; actualizar `remote.conf`.
  - [x] `RemoteConnect.cpp`: consumir `EC_TAG_PBKDF2_PARAMS` y derivar el secreto en memoria antes del handshake.
  - [x] `WebInterface.cpp` / `WebServer.cpp`: adoptar el mismo formato para Admin/Guest, exponer migraciones en log neutro.

#### 2.3 UPnP/Servicios externos
- [x] Mantener UPnP habilitado y soportado para conectividad entrante (wrapper `wmule_upnp_sdk` resuelve `miniupnpc` vía vcpkg; ENABLE_UPNP=ON documentado).
- [x] Implementar timeouts y retries limitados (backoff exponencial en `CUPnPControlPoint::AddPortMappings`).
- [x] Mostrar en UI/logs el estado real del mapeo (éxito/fallo/pendiente) y métricas de reintentos (resumen `CUPnPOperationReport` + bloque “Último estado UPnP” en Preferencias con botón de reintento).
- [x] Proveer fallback claro cuando no se logra el mapping (mensaje y acción en preferencias + log; supresión automática con política de 15 minutos y opción de forzar reintento).
- [x] Persistir el último resultado de UPnP (timestamp, router, puertos) para evitar repetir la misma operación si sabemos que fallará.

##### Diseño PBKDF2 + migración (2026-03-26)

- **Formato almacenado**: `pbkdf2-sha256$<iteraciones>$<salt_hex>$<dk_hex>`.
  - `iteraciones`: entero decimal, valor inicial recomendado `150000`.
  - `salt_hex`: 16 bytes aleatorios codificados en hex (32 caracteres).
  - `dk_hex`: resultado de PBKDF2-HMAC-SHA256 de longitud 32 bytes (64 hex).
- **Compatibilidad**:
  - Cualquier valor que no empiece por `pbkdf2-sha256$` se trata como hash legacy (MD5 hex). Durante carga se marca como `legacy` y se reescribe al nuevo formato en cuanto el usuario confirme preferencias o ejecute `amulecmd --write-config`.
  - Autenticación: el servidor comparará contra `dk_hex`. El handshake EC seguirá enviando un salt aleatorio (`m_passwd_salt`), pero añadiremos parámetros PBKDF2 (iteraciones + salt_hex) al paquete `EC_OP_AUTH_SALT` para que clientes CLI/Web puedan derivar `dk_hex` en tiempo real a partir del password plano introducido por el usuario. Posteriormente ambos lados harán `MD5( dk_hex.lower() + challengeSalt )` para no romper el resto del protocolo.
  - Webserver comparte el mismo formato; `WebInterface.cpp` leerá y escribirá los nuevos secretos tal cual.
- **Migración automática**:
  1. Detectar hashes legacy (`^[0-9a-fA-F]{32}$`).
  2. Generar salt PBKDF2 (16 bytes), aplicar PBKDF2 al hash legacy (lo usaremos como si fuese el “password” para no requerir la contraseña en texto claro) y almacenar el resultado en el nuevo formato.
  3. Cuando el usuario edite la contraseña desde la UI o CLI, repetir PBKDF2 usando el password plano y sobrescribir el archivo de config.
  4. Registrar en log un mensaje neutro “EC password migrated al nuevo formato” sin exponer hashes.
- **Biblioteca**: Crypto++ ya está en el árbol; PBKDF2-HMAC-SHA256 se implementará con `PKCS5_PBKDF2_HMAC<SHA256>`.
- **Handshake remoto**:
  - `ExternalConn.cpp`: añade `EC_TAG_PBKDF2_PARAMS` con iteraciones y salt_hex a la respuesta de `EC_OP_AUTH_SALT`.
  - `RemoteConnect.cpp` / `ExternalConnector.cpp`: si el paquete trae los parámetros, derivan `dk_hex` del password plano, lo guardan opcionalmente en memoria y siguen con el handshake. Si no, retrocompatibilidad total: seguir usando MD5 legacy.
- **Persistencia**: `PrefsUnifiedDlg`, `ec-config`, `remote.conf` y `amuleweb.conf` deben escribir siempre el nuevo formato; sólo se usa el legacy para lectura + migración.

### Validación durante la fase
- Tests específicos de path traversal y manipulación de rutas.
- Ejecución de flujos EC/UPnP tras cambios.

### Validación obligatoria al cierre
- [ ] `cmake --build . --config Debug`
- [ ] `ctest --output-on-failure -C Debug`
- [ ] Verificación básica de `wmule.exe`
- [ ] Verificación básica de `wmulecmd.exe`
- [ ] Actualizar estado/documentación

### Exit criteria
- Rutas normalizadas en todas las entradas externas.
- EC seguro por defecto con secretos protegidos.
- Tests de traversal/EC/UPnP pasando.

### Estado
- Estado actual: `[~] En progreso`

### Notas / incidencias
- `NormalizeAbsolutePath` ya existe (`src/libs/common/Path.cpp/.h`) y `Preferences.cpp` lo usa para validar directorios configurables; falta propagarlo al resto de entradas de la fase.
- 2026-03-26: `NormalizeSharedPath` se aplica en Prefs (Temp/Incoming/OS), categorías, el conversor de partes, wxCas y aLinkCreator; rutas no absolutas muestran error antes de guardarse.
- 2026-03-26: los conectores EC/Web normalizan Temp/Incoming/Shared y descartan rutas inválidas, dejando registro neutro y manteniendo los valores previos.
- [!] 2026-03-30: **Deuda técnica crítica** — el build completo solo progresa si se fuerzan `ENABLE_UPNP=OFF` y `BUILD_TESTING=OFF`. Con UPnP activo fallan `UPnPBase.cpp` (`UPNP_IGD_INVALID/UPNP_IGD_NONE` no existen en el backend actual) y con los tests activos se rompen `ECPreferencesValidationTest`/`InternalPathGuardsTest`, bloqueando `ctest`. El script oficial (`scripts/build-wmule.ps1`) quedó parcheado para desactivar ambos flags y compilar únicamente `wmule`/`wmulecmd`, pero debemos recuperar el build completo corrigiendo esas fuentes.
- 2026-03-30 (tarde): se corrigieron `UPnPBase.cpp` (dejando de referenciar constantes inexistentes), `NormalizeInternalDir` (verifica `IsSubPathOf` antes de crear directorios) y los tests MuleUnit (`ECPreferencesValidationTest`, `InternalPathGuardsTest`, `PathTraversalTest`). `scripts/build-wmule.ps1` vuelve a compilar con `BUILD_TESTING=ON` y genera todos los targets.
- 2026-03-31: la preferencia `Enable UPnP for router port forwarding` se inicializa en `ON` en instalaciones nuevas y queda documentado que el script oficial también lo fuerza. El usuario puede desactivarlo manualmente sin recompilar.
- 2026-03-30 (noche): habilitamos nuevamente `ENABLE_UPNP=ON` por defecto en `scripts/build-wmule.ps1`; la integración con miniupnpc recompila y linkea sin errores. El build completo (binarios + tests) queda alineado con el plan de Fase 2.
- 2026-03-31 (validación en router real): el usuario confirmó que `wmule.exe` abre automáticamente los puertos 11122/11125/11137 vía UPnP en un router doméstico y obtiene HighID/Kad OK tras el fix (`status=ok stage=none`, `discovered=4 filtered=0`).
- [ ] Deuda abierta: permitir que las rutas Incoming/Temp/OS puedan configurarse fuera de `ConfigDir` sin forzar fallback, aplicando normalización equivalente y controles de seguridad.
- 2026-03-27: Hardening de rutas integrado y publicado como **wMule 1.0.1**. Build `cmake --build . --config Debug`, `ctest -R PathTraversalTest -C Debug --output-on-failure` y smoke test manual (`wmule.exe` conecta a servidores, ejecuta búsquedas extendidas con filtros, descarga y comparte) antes de etiquetar el hito.

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

1. Completar subbloques críticos de **Fase 2** (normalización de rutas externas, endurecimiento de EC y feedback visible de UPnP) aplicando `NormalizeAbsolutePath`/helpers similares en todos los puntos de entrada.
2. Integrar `ClientUDPTest`, `KadHandlerFuzzTest`, `KadPacketGuardsTest`, `UPnPUrlUtilsTest` y `UPnPXmlSafetyTest` en `unittests/tests/CMakeLists.txt` para que `ctest` vuelva a reflejar la cobertura real.
3. Reejecutar builds/tests completos en matrices `ENABLE_UPNP=OFF/ON` tras cada bloque para garantizar que los fixes de CMake/libupnp se mantienen.
4. Actualizar `docs/BUILD_MEMORY.md` y este plan al cerrar cada hito parcial para dejar constancia de incidencias y validaciones.

---

## Anexos y Referencias

- `docs/history/Auditoria_Seguridad_aMule.md` – lista de vulnerabilidades y soluciones sugeridas.
- `docs/history/PLAN_MODERNIZACION_WINDOWS.md` – estado de fases 1-4.
- `AGENTS.md` – comandos de build/test y estilo de código.
- `CHANGELOG.md` – historial de versiones y tareas futuras (incluye header simplificado pendiente).
