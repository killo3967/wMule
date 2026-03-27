# Exploración SDD — Arquitectura real de wMule

## Resumen ejecutivo

Verifiqué el código y la configuración real del repo: hoy wMule **no** implementa todavía la arquitectura hexagonal/clean que describe `docs/blueprint.md`.

Lo que existe es un **monolito modular en transición**:

- `wmule` concentra el ciclo de vida en `CamuleApp` (`src/amule.cpp`)
- hay separación física útil en `src/libs/common`, `src/libs/ec` y `src/kademlia`
- `wmulecmd` y `amuleweb` consumen el sistema vía **External Connect (EC)**
- persisten acoplamientos fuertes por `theApp`, `thePrefs`, timers, eventos wxWidgets y estado compartido
- el documento más alineado con la realidad actual es `docs/PLAN_MODERNIZACION_2.0.md`, no `docs/blueprint.md`

---

## Exploración: arquitectura real de wMule

### Estado actual

El sistema actual está construido como una **aplicación de escritorio monolítica con modularización interna**, no como un core aislado con adapters limpios.

El wiring real pasa por:

- `src/amule.cpp` → bootstrap, init, sockets, scheduler, Kad, EC, webserver opcional, shutdown
- `src/amule.h` → agregado principal con punteros a casi todos los subsistemas
- `src/Preferences.cpp` → configuración global basada en muchos `static`
- `src/amuleAppCommon.cpp` → bootstrap compartido, config files, single-instance, links
- `src/ExternalConn.cpp` → servidor EC dentro del proceso principal

La separación por librerías existe, pero el desacople fuerte todavía **no**.

### 1. Módulos principales del sistema

- `src/`
  - núcleo operativo real + GUI + networking + persistencia + wiring
- `src/libs/common/`
  - utilidades relativamente reutilizables: `Path`, `TextFile`, `Format`, `SecretHash`, `ThreadPool`, `AsyncSocket`
- `src/libs/ec/`
  - protocolo External Connect: paquetes, tags, sockets, `RemoteConnect`
- `src/kademlia/`
  - DHT Kad separada físicamente en `kademlia/`, `net/`, `routing/`, `utils/`
- `src/webserver/`
  - `amuleweb`, separado como binario opcional, pero apoyado en EC y código legacy PHP/parser
- `unittests/tests/`
  - MuleUnit + benchmarks + tests focalizados de path, Kad, UDP, UPnP, etc.

### 2. Entrypoints y binarios principales

Reales según CMake y código:

- `wmule` → binario principal GUI (`src/CMakeLists.txt`, `src/amule-gui.cpp`, `src/amule.cpp`)
- `wmulecmd` → CLI remota vía EC (`src/TextClient.cpp`, `src/ExternalConnector.cpp`)
- `amuleweb` → webserver opcional vía EC (`src/webserver/src/WebInterface.cpp`)
- `ed2k` → handler opcional de links (`src/ED2KLinkParser.cpp`)

Importante:

- `BUILD_DAEMON` y `BUILD_REMOTEGUI` fueron eliminados del build Windows actual (`cmake/options.cmake`)
- es decir: el blueprint/documentación histórica sigue nombrando `amuled` y `amulegui`, pero en esta implementación Windows esos targets no son la realidad operativa principal

### 3. Dependencias relevantes entre subsistemas

Dependencias fuertes:

- GUI → core vía `theApp`
- core → config global vía `thePrefs`
- EC server (`ExternalConn.cpp`) → descarga, compartidos, prefs, stats, client list
- Kad → depende de `theApp`, `thePrefs`, `ClientUDPSocket`, `Packet`, `ClientList`
- webserver/CLI → no hablan con un “core desacoplado”; hablan con EC
- sockets → backend `LibSocket.cpp` delega a `LibSocketAsio.cpp` o `LibSocketWX.cpp`
- persistencia → mezclada entre `wxConfig`, `preferences.dat`, `addresses.dat`, `shareddir.dat`, `nodes.dat`, `.met`, `.dat`

### 4. Flujo de datos y de control a alto nivel

Flujo principal:

1. arranca `wmule`
2. `CamuleApp::OnInit()` inicializa prefs, paths, sockets, listas, colas, stats y network
3. crea timers y scheduler
4. levanta ED2K/Kad/EC según configuración
5. carga `.met`, shared files y friendlist
6. la GUI interactúa directamente con el estado del proceso
7. `wmulecmd` / `amuleweb` envían comandos por EC
8. `ExternalConn.cpp` ejecuta sobre colas/listas/prefs del mismo proceso
9. shutdown centralizado en `CamuleApp::ShutDown()`

### 5. Componentes reutilizables frente a zonas monolíticas o acopladas

**Reutilizables o relativamente encapsulados**

- `src/libs/common/Path.*`
- `src/libs/common/SecretHash.*`
- `src/libs/common/TextFile.*`
- `src/libs/common/ThreadPool.*`
- `src/libs/common/AsyncSocket.*`
- `src/libs/ec/cpp/*`
- partes de `src/kademlia/*`
- helpers nuevos de endurecimiento (`ClientUDPGuards`, `KadPacketGuards`, `UPnPUrlUtils`)

**Monolíticas o fuertemente acopladas**

- `src/amule.cpp`
- `src/amule.h`
- `src/Preferences.cpp`
- `src/DownloadQueue.cpp`
- `src/SharedFileList.cpp`
- `src/ExternalConn.cpp`
- buena parte de la GUI en `amuleDlg`, `PrefsUnifiedDlg`, list controls, etc.

### 6. Zonas legacy, frágiles o de alto riesgo

- `src/amule.cpp` por tamaño, rol central y orden de inicialización/shutdown
- `src/Preferences.cpp` por la cantidad de estado global estático
- `src/ExternalConn.cpp` por mezclar protocolo, auth, acceso a estado vivo y operaciones del core
- `src/kademlia/net/KademliaUDPListener.cpp` por sensibilidad protocolaria
- `src/ClientUDPSocket.cpp`, `EncryptedDatagramSocket.cpp`, `ClientTCPSocket.cpp`
- `src/webserver/src/*` por parser PHP legacy + compatibilidad
- persistencia `.met/.dat` y migraciones de configuración
- flujos de red y timers

### 7. Globals, estado compartido, timers, hilos, persistencia y protocolos sensibles

**Globals / estado compartido**

- `theApp` global real
- `thePrefs` como alias a `CPreferences` con estado `static`
- scheduler global (`ThreadScheduler.cpp` usa `s_scheduler`, `s_running`, `s_terminated`)
- IO service global en Asio (`LibSocketAsio.cpp`)

**Timers**

- `CTimer` propio para core
- `wxTimer` en GUI y componentes visuales
- temporizadores de búsquedas, retry, refresh, etc.

**Hilos / concurrencia**

- `CThreadScheduler`
- `UploadBandwidthThrottler`
- `CHTTPDownloadThread`
- `PartFileConvert`
- backend Asio con thread pool
- `ThreadPool` moderno coexistiendo con infraestructura antigua

**Persistencia**

- `wxConfig`
- `preferences.dat`
- `addresses.dat`
- `shareddir.dat`
- `nodes.dat`
- `part.met`, índices Kad, logs, archivos auxiliares

**Protocolos sensibles**

- eD2K TCP/UDP
- Kad UDP
- EC
- UPnP

### 8. Diferencias entre arquitectura documentada e implementada

**Ya existe del diseño objetivo**

- cierta modularización física
- librerías reutilizables (`libs/common`, `libs/ec`)
- algunos seams útiles para endurecimiento y tests
- abstracciones parciales de sockets/utilidades
- tests focalizados en áreas críticas

**Todavía no existe**

- core aislado de wxWidgets
- puertos/adapters limpios alrededor del dominio
- separación real GUI/Core
- límites arquitectónicos firmes entre app/network/storage/gui
- inversión de dependencias consistente
- arquitectura hexagonal/clean implementada de punta a punta

Además:

- `docs/blueprint.md` está desalineado en nombre, rutas, targets y madurez arquitectónica
- `docs/PLAN_MODERNIZACION_2.0.md` describe mejor la realidad: el desacople fuerte está recién en fase 6

### 9. Riesgos concretos al introducir cambios

- romper el orden de init/shutdown
- tocar `thePrefs` y generar inconsistencias entre UI, EC y archivos de configuración
- romper compatibilidad wire-level en Kad/eD2K/EC
- introducir carreras por ownership poco explícito
- romper timers o eventos wx al mover lógica “aparentemente pura”
- acoplar más wxWidgets a código reutilizable
- meter refactors grandes mezclados con fixes funcionales y volver imposible el review

### 10. Recomendaciones para trabajar con agentes sin romper el proyecto

**Conviene pedir cambios así**

- “extrae esta lógica de `CamuleApp` a un helper testeable”
- “encapsula este acceso a `thePrefs` detrás de una API local”
- “endurece el parsing de este paquete con límites explícitos”
- “normaliza estas rutas externas con el helper común”
- “añade tests focalizados para este flujo Kad/EC/UPnP”
- “introduce un seam para probar este subsistema sin GUI/socket real”

**Prompts que conviene evitar**

- “añádelo al core hexagonal”
- “implémentalo en la capa de aplicación”
- “conéctalo al domain kernel”
- “separa GUI y core” en un solo cambio
- “migra todo a clean architecture”
- “convierte el proyecto en servicio REST” ahora

### Enfoques

1. **Incremental sobre seams reales**
   - Pros: seguro, compatible, revisable
   - Contras: evolución gradual
   - Esfuerzo: bajo/medio

2. **Refactor arquitectónico grande guiado por blueprint ideal**
   - Pros: apariencia de alineación documental
   - Contras: asume una arquitectura falsa, riesgo altísimo
   - Esfuerzo: alto

### Recomendación

La recomendación es tratar wMule como **monolito modular en transición**.

Primero conviene priorizar:

- seguridad
- rutas/configuración remota
- compatibilidad EC/Kad/eD2K
- helpers extraíbles
- tests focalizados
- reducción puntual de acoplamiento

Solo después tiene sentido empujar un desacople estructural más fuerte.

### Riesgos principales

- `CamuleApp` es el orquestador real del sistema
- `CPreferences` concentra demasiado estado global
- EC toca muchas piezas vivas del proceso
- Kad y UDP son sensibles a tamaños, orden y compatibilidad
- concurrencia y shutdown tienen varias capas coexistiendo

### Listo para propuesta

Sí, pero **solo** si la propuesta parte de la arquitectura real implementada y mantiene un alcance incremental.

---

## Próximos pasos recomendados

- Tomar `docs/PLAN_MODERNIZACION_2.0.md` como base operativa para próximos cambios.
- Buenos candidatos inmediatos:
  1. endurecer otra entrada de rutas externas
  2. encapsular un acceso crítico a `thePrefs`
  3. extraer lógica pura desde `CamuleApp` o `ExternalConn`
  4. ampliar tests de EC / UPnP / Kad sin tocar arquitectura grande
