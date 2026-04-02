# Skill Registry

Regenerado por `sdd-init` el 2026-04-01 (modo engram) para el proyecto `wMule`.

## Fuentes escaneadas

### Directorios de skills
- Usuario: `C:\Users\tomas\.config\opencode\skills\` (excluidos `sdd-*`, `_shared`, `skill-registry`)
- Usuario: `C:\Users\tomas\.agents\skills\`
- Usuario: no encontrados `C:\Users\tomas\.claude\skills\`, `C:\Users\tomas\.gemini\skills\`, `C:\Users\tomas\.cursor\skills\`, `C:\Users\tomas\.copilot\skills\`
- Proyecto: no hay skills en `.claude/skills/`, `.gemini/skills/`, `.agent/skills/`, `skills/`

### Ficheros de convención del proyecto
- `AGENTS.md` (instrucciones primarias; referencia `docs/PLAN_MODERNIZACION_2.0.md`, `docs/blueprint.md`, `docs/BUILD_MEMORY.md`)
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
- Monolito modular en transición; `docs/blueprint.md` es objetivo, `docs/PLAN_MODERNIZACION_2.0.md` la hoja de ruta real (Fase 2 prioritaria).
- Core en `CamuleApp` (`src/amule.cpp`) con globals `theApp` y `thePrefs`; entrypoints `wmule.exe` y `wmulecmd.exe`.
- Carpetas clave: `src/`, `src/libs/common/`, `src/libs/ec/`, `src/kademlia/`, `unittests/tests/`, `docs/`, `cmake/`.
- Pedir cambios vía seams concretos, extracción incremental, reducción de acoplamiento y compatibilidad de protocolo.

### Convenciones de código
- Bloque GPL e include guards en `.cpp/.h`; orden de includes: header propio → proyecto → librerías → estándar.
- Naming: clases `C*`, miembros `m_*`, funciones `CamelCase`, locales `camelCase`.
- Preferir C++ moderno (`true/false`, `nullptr`, `using`, `static_assert`, RAII, STL); tabs (~100 chars); `wxT("...")` donde aplique.
- Evitar expandir dependencias wxWidgets en código reutilizable; favorecer helpers puros (`Path`, `UPnPUrlUtils`, guards Kad/UDP).

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
- Verificar afirmaciones (“dejame verificar” si duda); al preguntar, esperar respuesta.

### Guía de prompting
- Pedir cambios: “extrae esta lógica de `CamuleApp` a un servicio testeable”, “encapsula esta infraestructura detrás de una API local”, “reduce dependencias a `thePrefs`”, “introduce un seam para testear sin GUI/socket real”.
- Evitar: “agregalo al core hexagonal”, “implementalo en la capa de aplicación”, “conecta este adapter al domain kernel” si esas capas no existen.

## Tabla de skills del usuario (triggers)

| Skill | Source | Trigger / Cuándo usar |
| --- | --- | --- |
| api-patterns | user (.agents) | API design choices, versioning, pagination |
| architecture-patterns | user (.agents) | Clean/Hexagonal/DDD architecture work |
| async-python-patterns | user (.agents) | asyncio, concurrency, non-blocking Python |
| brainstorming | user (.agents) | Before creative feature/design work |
| branch-pr | user (.config/opencode) | Creating or preparing pull requests |
| c4-context | user (.agents) | High-level system context documentation |
| cmake | user (.agents) | CMakeLists, generators, toolchains, targets |
| commit | user (.agents) | Conventional commit message drafting |
| concise-planning | user (.agents) | Short actionable coding plans |
| database-architect | user (.agents) | Data modeling and DB architecture decisions |
| fastapi-pro | user (.agents) | FastAPI architecture and optimization |
| fastapi-templates | user (.agents) | Bootstrapping FastAPI projects |
| find-skills | user (.agents) | Discovering/installing skills |
| frontend-design | user (.agents) | Frontend styling and UI craft |
| git-master | user (.agents) | Strict git workflow and semantic commits |
| go-testing | user (.config/opencode) | Go tests and Bubbletea TUI testing |
| issue-creation | user (.config/opencode) | Creating GitHub issues |
| judgment-day | user (.config/opencode) | Dual adversarial review workflow |
| lint-and-validate | user (.agents) | Lint, format, static analysis, validation |
| media-toolkit | user (.agents) | FFmpeg/media processing work |
| nodriver-expert | user (.agents) | Robust async browser automation on Windows |
| prompt-engineering | user (.agents) | Improving prompts and agent behavior |
| python-patterns | user (.agents) | Python structure and framework choices |
| python-pro | user (.agents) | Advanced Python implementation work |
| python-testing-patterns | user (.agents) | Pytest and Python test strategy |
| security-auditor | user (.agents) | Security reviews and threat analysis |
| skill-creator | user (.config/opencode) | Creating new agent skills |
| sql-pro | user (.agents) | SQL optimization and advanced queries |
| sqlmodel-async-expert | user (.agents) | Async SQLModel / SQLAlchemy integration |
| systematic-debugging | user (.agents) | Bug investigation before fixes |
| ui-ux-designer | user (.agents) | UX, flows, design systems |
| web-scraping | user (.agents) | Web extraction and anti-blocking patterns |
| writing-plans | user (.agents) | Multi-step implementation planning from specs |

## Notas
- Se excluyen deliberadamente skills `sdd-*`, `_shared` y `skill-registry` de la tabla.
- Si aparecen skills de proyecto, deben sobrescribir a las de usuario con el mismo nombre en futuras regeneraciones.
