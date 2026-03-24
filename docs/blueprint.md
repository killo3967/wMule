# BLUEPRINT DEL PROYECTO (v2.2 — Aséptico y Auto-Documentado)

> [!IMPORTANT]
> **ESTE DOCUMENTO ES LA FUENTE DE VERDAD ABSOLUTA (SSOT).**
> Ningún agente debe proponer cambios que contradigan las definiciones aquí establecidas sin un ADR previo.

## 0. PROTOCOLO DE ESTADOS (Gestión de Incertidumbre)
Si un campo es desconocido o no se desea definir aún, usa estos marcadores obligatorios:
- **`TBD` (To Be Determined):** La decisión está pendiente. Es un **campo de libre descripción** que no está limitado a los ejemplos sugeridos en los comentarios. El Orquestador marcará esto como una **Tarea de Investigación Prioritaria**.
- **`AUTO_DETECT`:** El sistema debe intentar identificarlo solo (ej: buscando `.venv`, `package.json`, `Cargo.toml`).
- **`N/A` (Not Applicable):** El campo no aplica a la naturaleza de este proyecto (ej: un proyecto sin persistencia).

---

## 1. IDENTIDAD Y PROPÓSITO
# [CRÍTICO] Nombre identificativo del repositorio y del ejecutable final.
- **Nombre del Proyecto:** `aMule (all-platform Mule)`

# Propósito técnico y de negocio. Define el "Para qué" existe este software.
- **Propósito Central:** `Cliente P2P multiplataforma para las redes eDonkey (ED2K) y Kademlia, basado en el código de eMule.`

# Fase en la que se encuentra el desarrollo (Concepto, Alpha, Beta, Producción).
- **Estado Vital:** `Producción / Mantenimiento`

---

## 2. INFRAESTRUCTURA Y RUTAS (Host: Windows 11)
# [CRÍTICO] Ubicación física en el disco. Permite portabilidad total entre máquinas.
- **Raíz del Workspace:** `k:\amule`

# [CRÍTICO] Ruta al ejecutable o intérprete que gestiona la ejecución del código (ej: python.exe, node.exe).
- **Entorno de Ejecución (Runtime):** `C++ Runtime (Compilado con MSVC 2022 o GCC/Clang)`
- **Herramientas de Construcción (Build):**
  - **MSVC Env:** `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`
  - **CMake Path:** `C:\Program Files\CMake\bin\cmake.exe`

# Directorio donde reside el código fuente principal.
- **Área de Código (Source):** `src/`

# Directorio donde se almacenan las especificaciones y manuales.
- **Área de Documentación:** `docs/`

# Carpeta para bitácoras de sesión y logs históricos de los agentes.
- **Repositorio de Historia:** `docs/history/`

---

## 3. STACK TÉCNICO (INVARIANTES)
# [CRÍTICO] Plataforma base y su versión específica.
- **Motor / Lenguaje:** `C++ (C++17+)`

# Forma en que el usuario interactúa con el sistema (Desktop, Web, CLI, Headless o API).
- **Interfaz / Presentación:** `Multifacética: GUI (wxWidgets), CLI (amulecmd), Web (amuleweb), Daemon (amuled).`

# Mecanismo de control y monitoreo (ej: Archivo .ini, Panel de control, Logs, Dashboard).
- **Control y Monitorización:** `Protocolo External Connect (EC), logs integrados y archivos de configuración (.conf/.ini).`

# Protocolos de transporte y formato de datos (ej: REST/JSON, WebSockets, gRPC, TCP/UDP).
- **Capa de Comunicación:** `Protocolos ED2K (TCP/UDP), Kademlia (UDP), HTTP (Web Server).`

# Almacenamiento a largo plazo (ej: SQLite, Archivos locales .json, NoSQL, PostgreSQL).
- **Persistencia y Acceso a Datos:** `Archivos planos (metadatos .met, .dat), configuración local y gestión de blobs de datos compartidos.`

# Librerías externas sin las cuales el proyecto no puede compilar o ejecutarse.
- **Dependencias Críticas:** 
  - `wxWidgets (GUI y abstracción OS)`
  - `Crypto++ (Criptografía)`
  - `libupnp / libnatpmp (Network traversal)`
  - `zlib (Compresión)`

### 3.1 ARQUITECTURA DE REFERENCIA
# Estilo de diseño arquitectónico (Hexagonal, DDD, MVC). Mantiene la cohesión.
- **Patrón Estructural:** `Hexagonal (Ports & Adapters)`

# [CRÍTICO] Grado de independencia de la lógica central respecto a la tecnología externa.
- **Aislamiento del Núcleo:** `Kernel Aislado (Arquitectura Limpia)`

# Prioridad de construcción. Decide por dónde empezamos:
# - Domain-First (Recomendada): Empezamos por el Kernel/Núcleo y sus reglas. El programa es sólido y fácil de testear desde el día 1.
# - Data-First: Empezamos diseñando las tablas de la Base de Datos. Riesgo de aplicaciones rígidas "atadas" a los datos.
# - UI-First: Empezamos por las Pantallas. Resultados visuales rápidos, pero el corazón puede volverse caótico.
- **Estrategia de Desarrollo:** `Domain-First`

---

## 4. REGLAS DE NEGOCIO Y DOMINIO
# [CRÍTICO] ¿Sobre qué trata el programa? La pieza central que gestionamos (ej: Libros, Sensores, Tareas, Facturas).
- **Entidad Maestro (Núcleo):** `Mule / SharedFile / Peer`

# Forma de los datos (Relacional/SQL, Documental/NoSQL, Vectorial/IA, Grafo, Series Temporales).
- **Modelo de Datos:** `Basado en flujos de red y archivos de metadatos propietarios (.met).`

# ¿Cómo diferenciamos un objeto de otro? (ej: ID único, ISBN, DNI, Hash de archivo, Ruta).
- **Criterio de Identidad:** `MD4 Hash (UserHash y FileHash).`

# [CRÍTICO] Límites de carga y velocidad (ej: Soportar +1M de registros, 100 req/s, latencia < 50ms).
- **Escala y Rendimiento:** `Gestión de miles de conexiones simultáneas y transferencia de datos a alta velocidad.`

---

## 5. RESTRICCIONES TÉCNICAS (BLOQUEANTES)
# [CRÍTICO] Obligación de usar rutas independientes del Sistema Operativo.
- **Norma de Portabilidad:** `Uso obligatorio de abstracciones de sistema (ej: pathlib, os.path, path de Node, etc).`

# Aislamiento del entorno para evitar conflictos de librerías globales.
- **Aislamiento de Entorno:** `Entorno local/aislado obligatorio.`

# [CRÍTICO] Prohibición de guardar claves/tokens en texto plano.
- **Gestión de Secretos:** `Variables ambientales (.env) obligatorias.`

---

## 6. DOCUMENTACIÓN Y REFERENCIAS
- **Guías de Operación:** `N/A`
- **Contratos e Interfaces:** `N/A`

---

## 7. HISTORIAL DE GOBERNANZA
- **2026-03-08:** v2.3 — Hidratación completa tras auditoría: Definición de stack C++/wxWidgets y modelos de identidad ED2K.
- **2026-03-07:** v2.2 — Cierre del estándar: Mejoras en Interfaz, Comunicación, Persistencia y Portabilidad Universal.
