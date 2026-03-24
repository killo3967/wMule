# AGENTS.md - wMule Project Guidelines

**Project:** wMule (Windows Mule)  
**Version:** 1.0.0 (fork of aMule 2.4.0)  
**Platform:** Windows x64 only  
**License:** GPL v2+  
**Build System:** CMake + vcpkg + MSVC 2022

---

## Build Commands

### Configure (out-of-source build)

```bash
cd K:\wMule\build
cmake .. -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DENABLE_UPNP=OFF
```

### Build

```bash
cmake --build . --config Debug
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_MONOLITHIC` | ON | Build main `wmule.exe` GUI |
| `BUILD_AMULECMD` | ON | Build `wmulecmd.exe` CLI |
| `BUILD_TESTING` | ON | Build unit tests |
| `ENABLE_BOOST` | ON | Use Boost.ASIO sockets |
| `ENABLE_UPNP` | OFF | UPnP support |
| `CMAKE_BUILD_TYPE` | Debug | Debug or Release |

---

## Testing

Framework: **MuleUnit** (custom, EasyUnit-based). Tests in `unittests/tests/`.

### Run All Tests

```bash
cd K:\wMule\build
ctest --output-on-failure -C Debug
```

### Run Single Test

```bash
# Using ctest regex
ctest -R FormatTest -C Debug

# Or run executable directly
.\unittests\tests\FormatTest.exe
```

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

| Element | Convention | Example |
|---------|------------|---------|
| Classes | `CamelCase` with `C` prefix | `CFile`, `CServer` |
| Member variables | `m_variableName` | `m_fileName`, `m_count` |
| Functions | `CamelCase` | `GetFileName`, `AddDebugLogLineC` |
| Constants/macros | `UPPER_SNAKE_CASE` | `MAX_CONNECTIONS` |
| Local variables | `camelCase` | `fileName`, `totalCount` |

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

| Old | New | Example |
|-----|-----|---------|
| `TRUE` / `FALSE` | `true` / `false` | `bool ready = true;` |
| `NULL` | `nullptr` | `void* ptr = nullptr;` |
| `typedef` | `using` | `using uint32 = uint32_t;` |

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

---

## Key Directories

| Path | Content |
|------|---------|
| `src/` | Main source code |
| `src/kademlia/` | Kademlia DHT implementation |
| `src/libs/common/` | Common utilities (ThreadPool, AsyncSocket) |
| `src/libs/ec/` | Protocol encoding/decoding |
| `src/libs/socket/` | Socket implementations |
| `unittests/tests/` | MuleUnit tests |
| `build/` | Build output |
| `docs/` | Documentation |
| `cmake/` | CMake modules |

---

## Important Notes

### Dependencies (vcpkg)

- wxWidgets 3.3.1
- Boost 1.90.0 (asio, filesystem, regex, signals2, thread)
- Crypto++ (latest)
- zlib, libpng, libwebp, pcre2, tiff

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

---

## Git Workflow

- Trabaja siempre en ramas feature; evita commits directos en `main`.
- Antes de abrir un PR: ejecuta `cmake --build . --config Debug` y `ctest --output-on-failure -C Debug`.
- Todo PR debe tener revisión humana + CI verde antes de mergear.
- Separa cambios críticos de seguridad de refactors cosméticos para facilitar el review.

---

## Common Issues

### wxWidgets Not Found

```bash
# Use vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### UPNP Target Not Found

```bash
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

```bash
# Option 1: Using the batch script
K:\wMule\do_build_ninja.bat   # genera build-ninja/ y compile_commands.json dentro de ese directorio

# Option 2: Manual (requires vcvars64.bat)
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist build-ninja mkdir build-ninja
cd build-ninja
cmake -G Ninja .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Note: `compile_commands.json` vive en `build-ninja/`; apunta clangd a ese directorio. Solo se genera con el generador Ninja, no con Visual Studio.

---

## Version Info

From `src/include/common/ClientVersion.h`:
```cpp
#define VERSION      1.0.0
#define MOD_VERSION "1.0.0"
#define MOD_VERSION_LONG "wMule 1.0.0"
```
