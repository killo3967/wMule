# AGENTS.md - wMule Project Guidelines

**Project:** wMule (Windows Mule)  
**Version:** 1.0.0 (fork of aMule 2.4.0)  
**Platform:** Windows 11 x64 only  
**License:** GPL v2+  
**Build System:** CMake + vcpkg + MSVC 2022

---

## Reglas de Interacción para Agentes

- Responder **siempre en español**.
- Asumir **Windows 11** como sistema operativo de trabajo.
- Preferir **PowerShell** para ejemplos, comandos y automatización; **no asumir Bash** ni herramientas GNU instaladas.
- Usar rutas de Windows (`K:\wMule`, `C:\vcpkg\...`) y quoting compatible con PowerShell.
- No describir la arquitectura actual como hexagonal/clean ya implementada: tratar el repo como **monolito modular en transición**, usando `docs/blueprint.md` como objetivo y `docs/PLAN_MODERNIZACION_2.0.md` como hoja de ruta real.
- Priorizar siempre: **seguridad > estabilidad > arquitectura > nuevas features**.
- Proponer cambios **incrementales**, con compatibilidad de protocolo y validación explícita.

---

## Contexto Arquitectónico Real

- El entrypoint principal es `wmule.exe` (GUI desktop sobre wxWidgets).
- `wmulecmd.exe` consume el sistema a través de **External Connect (EC)**.
- `amuleweb`/webserver es un adaptador opcional, no el centro de la arquitectura.
- El core operativo sigue concentrado en `CamuleApp` (`src/amule.cpp`) y depende de globals como `theApp` y `thePrefs`.
- Hay modularización física útil (`src/libs/common/`, `src/libs/ec/`, `src/kademlia/`), pero el desacople fuerte GUI/Core todavía pertenece a la **Fase 6** del plan.

### Cómo deben pedirse los cambios

Pedirlos en términos de:

- seams concretos
- extracción incremental
- reducción de acoplamiento
- endurecimiento de parsing/configuración/concurrencia
- compatibilidad con protocolo eD2K/Kad/EC existente

Evitar prompts que asuman capas inexistentes, por ejemplo:

- “agregalo al core hexagonal”
- “implementalo en la capa de aplicación”
- “conectá este adapter al domain kernel”

Mejor pedir:

- “extraé esta lógica de `CamuleApp` a un servicio testeable”
- “encapsulá esta infraestructura detrás de una API local”
- “reducí dependencias a `thePrefs` en este flujo”
- “introducí un seam para poder probar este subsistema sin GUI/socket real”

---

## Build Commands

### Configure (out-of-source build)

```powershell
Set-Location K:\wMule\build
cmake .. `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DENABLE_UPNP=OFF
```

### Build

```powershell
Set-Location K:\wMule\build
cmake --build . --config Debug
```

### CMake Options

| Option             | Default | Description                |
| ------------------ | ------- | -------------------------- |
| `BUILD_MONOLITHIC` | ON      | Build main `wmule.exe` GUI |
| `BUILD_AMULECMD`   | ON      | Build `wmulecmd.exe` CLI   |
| `BUILD_TESTING`    | ON      | Build unit tests           |
| `ENABLE_BOOST`     | ON      | Use Boost.ASIO sockets     |
| `ENABLE_UPNP`      | OFF     | UPnP support               |
| `CMAKE_BUILD_TYPE` | Debug   | Debug or Release           |

---

## Testing

Framework: **MuleUnit** (custom, EasyUnit-based). Tests in `unittests/tests/`.

La validación del proyecto está alineada con `docs/PLAN_MODERNIZACION_2.0.md`:

- tras cada cambio relevante, compilar la parte afectada y ejecutar tests focalizados
- al cerrar un bloque/fase: build completo + `ctest` + verificación básica de `wmule.exe` y `wmulecmd.exe`
- actualizar documentación si el cambio altera el estado real del plan

### Run All Tests

```powershell
Set-Location K:\wMule\build
ctest --output-on-failure -C Debug
```

### Run Single Test

```powershell
# Using ctest regex
ctest -R FormatTest -C Debug

# Or run executable directly
.\unittests\tests\FormatTest.exe
```

### Validación canónica al cerrar cambios relevantes

```powershell
Set-Location K:\wMule\build
cmake --build . --config Debug
ctest --output-on-failure -C Debug
```

Además:

- verificar arranque básico de `src\Debug\wmule.exe`
- verificar arranque/uso básico de `src\Debug\wmulecmd.exe`
- si el cambio toca documentación o roadmap, actualizar `docs\PLAN_MODERNIZACION_2.0.md`

### Write Tests

```cpp
#include <muleunit/test.h>

// Simple test (no fixture)
DECLARE_SIMPLE(MyTest);

TEST(MyTest, BasicCheck)
{
    ASSERT_EQUALS(42, computeValue());
}

// With fixture
DECLARE(MyFixture)
    int* data;
    void setUp() { data = new int(10); }
    void tearDown() { delete data; }
END_DECLARE();

TEST(MyFixture, TestWithData)
{
    ASSERT_EQUALS(10, *data);
}
```

Key macros: `ASSERT_EQUALS`, `ASSERT_TRUE`, `ASSERT_RAISES`, `ASSERT_EQUALS_M`.

When adding a new test, update `unittests/tests/CMakeLists.txt`.

### Prioridades de testing según el código y el plan 2.0

- Parsing de red eD2K/Kad: validar tamaños, límites, truncamientos y casos hostiles.
- UPnP: cubrir XML, URLs, LAN-only y retries/fallbacks.
- Paths/configuración: cubrir traversal, rutas absolutas/UNC y normalización.
- EC/Web: cubrir autenticación, migración de credenciales y compatibilidad.
- Cambios en concurrencia: agregar pruebas focalizadas y documentar ownership.

---

## Code Style

### File Header

Every `.cpp`/`.h` file starts with the GPL license block (see existing files).

### Include Guards

```cpp
#ifndef FILENAME_H
#define FILENAME_H
// ...
#endif /* FILENAME_H */
```

### Includes Order

1. Corresponding header (`"CFile.h"`)
2. Project headers (`"Logger.h"`, `"config.h"`)
3. Library headers (`<common/Path.h>`, `<wx/...>`)
4. Standard library (`<map>`, `<vector>`, `<list>`)

Use `// Needed for ...` comments when dependency is non-obvious.

### Naming Conventions

| Element          | Convention                  | Example                           |
| ---------------- | --------------------------- | --------------------------------- |
| Classes          | `CamelCase` with `C` prefix | `CFile`, `CServer`                |
| Member variables | `m_variableName`            | `m_fileName`, `m_count`           |
| Functions        | `CamelCase`                 | `GetFileName`, `AddDebugLogLineC` |
| Constants/macros | `UPPER_SNAKE_CASE`          | `MAX_CONNECTIONS`                 |
| Local variables  | `camelCase`                 | `fileName`, `totalCount`          |

### Types

Use types from `src/Types.h`:

```cpp
// Modern C++ aliases (preferred)
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using sint32 = int32_t;
using sint64 = int64_t;

// Compile-time verification already done in Types.h
static_assert(sizeof(uint32) == 4, "uint32 must be 4 bytes");
```

### Modern C++ Guidelines (Mandatory)

| Old              | New              | Example                    |
| ---------------- | ---------------- | -------------------------- |
| `TRUE` / `FALSE` | `true` / `false` | `bool ready = true;`       |
| `NULL`           | `nullptr`        | `void* ptr = nullptr;`     |
| `typedef`        | `using`          | `using uint32 = uint32_t;` |

- Use `static_assert` for compile-time checks
- Prefer RAII and smart pointers over raw memory
- Use `std::array` / `std::vector` over C arrays

### Indentation & Formatting

- **Tabs** for indentation (tab width = 4 spaces)
- Braces: follow surrounding code style
- Use `wxT("...")` macro for string literals in wxWidgets code
- Maximum line length: ~100 characters

### Platform Conditionals

```cpp
#ifdef __WINDOWS__     // wxWidgets Windows
#ifdef _WIN32          // raw Windows (non-wx apps)
#ifdef __LINUX__       // wxWidgets Linux (legacy)
#ifndef __WXDEBUG__    // release builds
```

### Error Handling

- Logging: `AddDebugLogLineC()` / `AddLogLineCS()`
- Debug asserts: `wxASSERT(condition)`
- Tests: `ASSERT_RAISES(CAssertFailureException, ...)`

### Reglas intrínsecas del código

- Mantener compatibilidad con protocolos existentes (eD2K, Kad, EC) salvo cambio explícitamente planificado.
- Endurecer primero parsing, rutas, configuración remota y concurrencia antes de agregar features nuevas.
- No introducir dependencias innecesarias a wxWidgets en código que pueda quedar reusable/testeable.
- Favorecer extracción de helpers puros y validables, siguiendo el patrón ya visible en `Path`, `UPnPUrlUtils` y guards Kad/UDP.
- Evitar refactors arquitectónicos masivos mezclados con fixes funcionales.

---

## Key Directories

| Path               | Content                                    |
| ------------------ | ------------------------------------------ |
| `src/`             | Main source code                           |
| `src/kademlia/`    | Kademlia DHT implementation                |
| `src/libs/common/` | Common utilities (ThreadPool, AsyncSocket) |
| `src/libs/ec/`     | Protocol encoding/decoding                 |
| `src/libs/socket/` | Socket implementations                     |
| `unittests/tests/` | MuleUnit tests                             |
| `build/`           | Build output                               |
| `docs/`            | Documentation                              |
| `cmake/`           | CMake modules                              |

---

## Important Notes

### Estado y prioridades según `docs/PLAN_MODERNIZACION_2.0.md`

- **Fase 0**: completada
- **Fase 1**: completada
- **Fase 2**: en progreso (**prioridad actual**)
- **Fases 3-8**: pendientes

### Prioridad actual de trabajo

Mientras `Fase 2` siga abierta, priorizar:

- normalización y validación de rutas externas
- endurecimiento de External Connect y credenciales PBKDF2
- feedback real de UPnP en UI/logs
- cobertura de tests de traversal, EC y UPnP

No desviar el trabajo hacia REST, nueva GUI o migraciones mayores mientras las fases 2-7 no estén cerradas.

### Dependencies (vcpkg)

- wxWidgets 3.3.1
- Boost 1.90.0 (asio, filesystem, regex, signals2, thread)
- Crypto++ (latest)
- zlib, libpng, libwebp, pcre2, tiff
- libupnp (opcional; puede requerir validación específica con `ENABLE_UPNP=ON`)

### Librerías y subsistemas clave a respetar

- **wxWidgets**: GUI y ciclo de vida de la app; evitar expandir su alcance hacia lógica reusable.
- **Boost.ASIO / sockets**: base de red existente; no reemplazar sin plan incremental.
- **Crypto++**: usado para endurecimiento criptográfico, incluyendo PBKDF2-HMAC-SHA256 en credenciales EC.
- **libupnp**: integración sensible; validar siempre comportamiento LAN-only, retries y linking cuando `ENABLE_UPNP=ON`.
- **MuleUnit**: framework de pruebas actual; cualquier suite nueva debe integrarse en `unittests/tests/CMakeLists.txt`.

### Generated Files (DO NOT EDIT)

- `build/config.h`
- `build/version.rc`
- `src/libs/ec/cpp/ECCodes.h`
- `src/libs/ec/cpp/ECTagTypes.h`
- `pixmaps/flags_xpm/CountryFlags.h`

### Do NOT

- Modify generated files (listed above)
- Commit `config.h`, build artifacts, or IDE files
- Use `TRUE`/`FALSE` or `NULL` - use modern equivalents
- Break CMake configuration
- Assume Bash, GNU tools or Linux paths exist in the environment
- Present `docs/blueprint.md` as if it already matched the current implementation

---

## Git Workflow

- Trabaja siempre en ramas feature; evita commits directos en `main`.
- Antes de abrir un PR: ejecuta `cmake --build . --config Debug` y `ctest --output-on-failure -C Debug`.
- Todo PR debe tener revisión humana + CI verde antes de mergear.
- Separa cambios críticos de seguridad de refactors cosméticos para facilitar el review.
- Si el cambio cierra o mueve trabajo de una fase, actualizá `docs/PLAN_MODERNIZACION_2.0.md` con estado, validaciones y notas.

### Estrategia de cambios recomendada para agentes

1. Explorar el subsistema concreto.
2. Identificar riesgos sobre globals, timers, persistencia y protocolo.
3. Proponer cambio incremental.
4. Implementar con alcance acotado.
5. Validar con tests focalizados y, cuando corresponda, build/test completo.
6. Actualizar documentación relevante.

---

## Common Issues

### wxWidgets Not Found

```powershell
# Use vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### UPNP Target Not Found

```powershell
# Disable UPNP
cmake .. -DENABLE_UPNP=OFF
```

### config.h Not Found

Ensure build directory is used (not source). Check `${wMule_BINARY_DIR}` is in include paths.

---

## IDE Configuration (VSCode)

### Files Created

- `.vscode/c_cpp_properties.json` - C/C++ IntelliSense configuration
- `.vscode/settings.json` - CMake, clangd and editor settings
- `do_build_ninja.bat` - Build script with vcvars for compile_commands.json

### For LSP (clangd) to Work

1. Install extensions: "clangd" and "CMake Tools"
2. Run `do_build_ninja.bat` to generate `compile_commands.json` en `build-ninja/` (requiere Ninja + vcvars)
3. Reload VSCode window: `Ctrl+Shift+P` → "Developer: Reload Window"
4. clangd will use compile_commands.json to resolve wxWidgets headers

### Generate compile_commands.json

```powershell
# Option 1: Using the batch script
K:\wMule\do_build_ninja.bat   # genera build-ninja/ y compile_commands.json dentro de ese directorio

# Option 2: Manual (requires vcvars64.bat)
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist build-ninja mkdir build-ninja
cd build-ninja
cmake -G Ninja .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Nota de shell

- Para documentación y automatización, preferir comandos compatibles con **PowerShell**.
- Si se documentan batch files (`.bat`), aclarar cuándo deben ejecutarse desde `cmd.exe` o desde PowerShell.

Note: `compile_commands.json` vive en `build-ninja/`; apunta clangd a ese directorio. Solo se genera con el generador Ninja, no con Visual Studio.

---

## Version Info

From `src/include/common/ClientVersion.h`:

```cpp
#define VERSION      1.0.0
#define MOD_VERSION "1.0.0"
#define MOD_VERSION_LONG "wMule 1.0.0"
```
