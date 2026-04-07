# Skill Registry

Regenerado por `sdd-init` el 2026-04-05 (modo engram) para el proyecto `wMule`.

## Fuentes escaneadas

### Directorios de skills
- Usuario: `C:\Users\tomas\.claude\skills\` (excluidos `sdd-*`, `_shared`, `skill-registry`)
- Usuario: `C:\Users\tomas\.agents\skills\`
- Usuario: `C:\Users\tomas\.config\opencode\skills\` (excluidos `sdd-*`, `_shared`, `skill-registry`)
- Usuario: no encontrados `C:\Users\tomas\.gemini\skills\`, `C:\Users\tomas\.cursor\skills\`, `C:\Users\tomas\.copilot\skills\`
- Proyecto: no hay skills en `.claude/skills/`, `.gemini/skills/`, `.agent/skills/`, `skills/`

### Ficheros de convención del proyecto
- `AGENTS.md` (reglas primarias; referencia `docs/PLAN_MODERNIZACION_2.0.md`, `docs/blueprint.md`, `docs/BUILD_MEMORY.md`)
- `docs/PLAN_MODERNIZACION_2.0.md`
- `docs/blueprint.md`
- `docs/BUILD_MEMORY.md`

## Reglas compactas

### Stack del proyecto
- Interacción: responder siempre en español de España; preferir PowerShell; Windows 11 x64.
- Build: C++20 con CMake + vcpkg + MSVC 2022, build out-of-source.
- UI/runtime: desktop wxWidgets (`wmule.exe`), CLI EC (`wmulecmd.exe`), adaptador web opcional.
- Dominios: eD2K + Kademlia; UPnP opcional.
- Testing: MuleUnit ejecutable registrado en CTest.

### Arquitectura y estructura
- Monolito modular en transición; `docs/blueprint.md` es objetivo aspiracional, `docs/PLAN_MODERNIZACION_2.0.md` la hoja de ruta real (Fase 2 prioritaria).
- Core en `CamuleApp` (`src/amule.cpp`) con globals `theApp` y `thePrefs`; entrypoints `wmule.exe` y `wmulecmd.exe`.
- Carpetas clave: `src/`, `src/libs/common/`, `src/libs/ec/`, `src/kademlia/`, `unittests/tests/`, `docs/`, `cmake/`.
- Pedir cambios vía seams concretos, extracción incremental, reducción de acoplamiento y compatibilidad de protocolo.

### Convenciones de código
- Bloque GPL e include guards en `.cpp/.h`; orden de includes: header propio → proyecto → librerías → estándar.
- Naming: clases `C*`, miembros `m_*`, funciones `CamelCase`, locales `camelCase`.
- Preferir C++ moderno (`true/false`, `nullptr`, `using`, `static_assert`, RAII, STL); tabs (~100 chars); `wxT("...")` donde aplique.
- Evitar expandir dependencias wxWidgets en código reutilizable; favorecer helpers puros (`Path`, `UPnPUrlUtils`, guards Kad/UDP`).

### Reglas de seguridad
- No editar generados: `build/config.h`, `build/version.rc`, `src/libs/ec/cpp/ECCodes.h`, `src/libs/ec/cpp/ECTagTypes.h`, `pixmaps/flags_xpm/CountryFlags.h`.
- No committear artefactos de build/IDE; build out-of-source.
- Priorizar: **seguridad > estabilidad > arquitectura > features**.
- Mantener compatibilidad eD2K/Kad/EC salvo plan explícito; no presentar `docs/blueprint.md` como estado actual.
- No construir tras cambios salvo petición expresa; sin “Co-Authored-By” ni atribución IA en commits.

### Testing y validación
- Tests en `unittests/tests/`, registrar en `unittests/tests/CMakeLists.txt`.
- Priorizar cobertura en parsing de red, rutas, EC, UPnP, concurrencia.
- Runner canónico: `ctest`; ejecutables MuleUnit standalone. `build-ninja/compile_commands.json` guía para clangd.
- Mientras la Fase 2 siga abierta: normalizar rutas, endurecer EC/PBKDF2, feedback UPnP en UI/logs, tests traversal/EC/UPnP.
- Al cerrar cambios: build completo + `ctest` + smoke de `wmule.exe`/`wmulecmd.exe` + actualizar plan.

### Flujo Git e interacción
- Trabajar en ramas feature; separar seguridad riesgosa de refactors cosméticos.
- Antes de PR: `cmake --build . --config Debug` y `ctest --output-on-failure -C Debug` en `build/`.
- Actualizar `docs/PLAN_MODERNIZACION_2.0.md` si cambia el estado de fase.
- Verificar afirmaciones (“dejame verificar” si hay duda); al preguntar, esperar respuesta.

### Guía de prompting
- Pedir cambios: “extrae esta lógica de `CamuleApp` a un servicio testeable”, “encapsula esta infraestructura detrás de una API local”, “reduce dependencias a `thePrefs`”, “introduce un seam para testear sin GUI/socket real”.
- Evitar: “agregalo al core hexagonal”, “implementalo en la capa de aplicación”, “conecta este adapter al domain kernel” si esas capas no existen.

## Tabla de skills del usuario (triggers)

| Skill | Source | Trigger / Cuándo usar |
| --- | --- | --- |
| api-patterns | user (.agents) | Diseño de APIs, versiones y paginación |
| architecture-patterns | user (.agents) | Patrones backend tipo Clean/Hex/DDD |
| async-python-patterns | user (.agents) | Trabajos Python asyncio y concurrencia |
| brainstorming | user (.agents) | Antes de trabajos creativos o de diseño |
| branch-pr | user (.config/opencode) | Crear o preparar pull requests ATL |
| c4-context | user (.agents) | Documentar contexto C4 y dependencias |
| cmake | user (.agents) | Ajustar CMakeLists, toolchains y presets |
| commit | user (.agents) | Redactar commits convencionales estilo Sentry |
| concise-planning | user (.agents) | Generar checklists atómicas antes de codificar |
| cpp-coding-standards | user (.agents) | Aplicar normas modernas C++ / Core Guidelines |
| cpp-pro | user (.agents) | Optimizaciones C++20/23, memoria y plantillas |
| cpp-testing | user (.agents) | Añadir/fijar tests C++ (GoogleTest/CTest) |
| database-architect | user (.agents) | Modelado de datos y selección de motores |
| fastapi-pro | user (.agents) | Arquitectura y optimización FastAPI |
| fastapi-templates | user (.agents) | Bootstrap completo de proyectos FastAPI |
| find-skills | user (.agents) | Descubrir/instalar nuevas skills disponibles |
| frontend-design | user (.agents) | Diseñar/stilar UIs con alto craft visual |
| git-master | user (.agents) | Flujos Git exigentes y estrategias de ramas |
| go-testing | user (.config/opencode) | Testing Go + Bubbletea/teatest |
| issue-creation | user (.config/opencode) | Abrir issues GitHub siguiendo ATL |
| judgment-day | user (.config/opencode) | Revisiones duales adversariales |
| lint-and-validate | user (.agents) | Ejecutar linters, formatters y validadores |
| media-toolkit | user (.agents) | Procesar audio/video (FFmpeg, MKVToolNix) |
| nodriver-expert | user (.agents) | Automatización nodriver robusta en Windows |
| prompt-engineering | user (.agents) | Mejorar prompts y comportamiento de agentes |
| python-patterns | user (.agents) | Estructura y convenciones de proyectos Python |
| python-pro | user (.agents) | Implementaciones avanzadas/performantes en Python |
| python-testing-patterns | user (.agents) | Estrategias Pytest, fixtures y TDD |
| security-auditor | user (.agents) | Escaneos OWASP y endurecimiento de superficies |
| skill-creator | user (.config/opencode) | Crear nuevas skills siguiendo la spec ATL |
| sql-pro | user (.agents) | SQL avanzado, tuning y análisis híbrido |
| sqlmodel-async-expert | user (.agents) | SQLModel/SQLAlchemy async, MissingGreenlet |
| systematic-debugging | user (.agents) | Diagnóstico sistemático antes de corregir bugs |
| ui-ux-designer | user (.agents) | Experiencia de usuario, flows y design systems |
| web-scraping | user (.agents) | Extracción web con anti-bloqueos |
| writing-plans | user (.agents) | Planificar implementación multi-paso desde specs |

## Notas
- Se excluyen deliberadamente skills `sdd-*`, `_shared` y `skill-registry` de la tabla.
- Si aparecen skills de proyecto, deben sobrescribir a las de usuario con el mismo nombre en futuras regeneraciones.
