# AGENTS.md - wMule Project Guidelines

**Project:** wMule (Windows Mule)  
**Version:** 1.0.3 (fork of aMule 2.4.0)  
**Platform:** Windows 11 x64 only  
**License:** GPL v2+  
**Build System:** CMake + vcpkg + MSVC 2022

---

## Agent Interaction Rules

- Always respond in **Spanish**.
- Assume **Windows 11** as the working OS.
- Prefer **PowerShell** for examples, commands, and automation; do **not** assume Bash or GNU tools are installed (e.g. `grep`, `sed`, `awk`).
- Use Windows paths (`K:\wMule`, `C:\vcpkg\...`) and PowerShell-compatible quoting.
- Do not describe the current architecture as already hexagonal/clean: treat the repo as a **modular monolith in transition**, using `docs/blueprint.md` as the target and `docs/PLAN_MODERNIZACION_2.0.md` as the real roadmap.
- Always prioritize: **security > stability > architecture > new features**.
- Propose **incremental** changes with protocol compatibility and explicit validation.

---

## Real Architectural Context

- The main entrypoint is `wmule.exe` (desktop GUI on wxWidgets).
- `wmulecmd.exe` consumes the system through **External Connect (EC)**.
- `amuleweb`/webserver is an optional adapter, not the center of the architecture.
- The operational core still lives in `CamuleApp` (`src/amule.cpp`) and depends on globals such as `theApp` and `thePrefs`.
- There is useful physical modularization (`src/libs/common/`, `src/libs/ec/`, `src/kademlia/`), but the strong GUI/Core separation still belongs to **Phase 6** of the plan.

### How to request changes

Ask for changes in terms of:

- concrete seams
- incremental extraction
- reduced coupling
- hardening parsing/configuration/concurrency
- compatibility with existing eD2K/Kad/EC protocol behavior

Avoid prompts that assume non-existent layers, for example:

- “add it to the hexagonal core”
- “implement it in the application layer”
- “connect this adapter to the domain kernel”

Better requests:

- “extract this logic from `CamuleApp` into a testable service”
- “wrap this infrastructure behind a local API”
- “reduce dependencies on `thePrefs` in this flow”
- “introduce a seam so this subsystem can be tested without a real GUI/socket”

---

## Build Commands

### Configure (out-of-source build)

```powershell
Set-Location K:\wMule\build
cmake .. `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DENABLE_UPNP=ON
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

Framework: **MuleUnit** (custom, EasyUnit-based). Tests live in `unittests/tests/`.

Project validation follows `docs/PLAN_MODERNIZACION_2.0.md`:

- after each relevant change, build the affected part and run focused tests
- when closing a block/phase: full build + `ctest` + basic verification of `wmule.exe` and `wmulecmd.exe`
- update documentation if the change alters the real plan status

### Run All Tests

```powershell
Set-Location K:\wMule\build
ctest --output-on-failure -C Debug
```

### Run Single Test

```powershell
# Using ctest regex
ctest -R FormatTest -C Debug

# Or run the executable directly
.\unittests\tests\FormatTest.exe
```

### Canonical validation when closing relevant changes

```powershell
Set-Location K:\wMule\build
cmake --build . --config Debug
ctest --output-on-failure -C Debug
```

Additionally:

- verify basic startup of `src\Debug\wmule.exe`
- verify basic startup/use of `src\Debug\wmulecmd.exe`
- if the change touches documentation or the roadmap, update `docs\PLAN_MODERNIZACION_2.0.md`

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

### Testing priorities based on the code and plan 2.0

- eD2K/Kad network parsing: validate sizes, limits, truncations, and hostile cases.
- UPnP: cover XML, URLs, LAN-only behavior, and retries/fallbacks.
- Paths/configuration: cover traversal, absolute/UNC paths, and normalization.
- EC/Web: cover authentication, credential migration, and compatibility.
- Concurrency changes: add focused tests and document ownership.

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

### Include Order

1. Corresponding header (`"CFile.h"`)
2. Project headers (`"Logger.h"`, `"config.h"`)
3. Library headers (`<common/Path.h>`, `<wx/...>`)
4. Standard library (`<map>`, `<vector>`, `<list>`)

Use `// Needed for ...` comments when a dependency is not obvious.

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
- Use `std::array` / `std::vector` instead of C arrays

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

### Core Code Rules

- Keep compatibility with existing protocols (eD2K, Kad, EC) unless a change is explicitly planned.
- Harden parsing, paths, remote configuration, and concurrency before adding new features.
- Do not introduce unnecessary wxWidgets dependencies in code that could remain reusable/testable.
- Prefer extraction of pure, verifiable helpers, following the pattern already visible in `Path`, `UPnPUrlUtils`, and Kad/UDP guards.
- Avoid large architectural refactors mixed with functional fixes.

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

### Status and priorities according to `docs/PLAN_MODERNIZACION_2.0.md`

- **Phase 0**: completed
- **Phase 1**: completed
- **Phase 2**: completed
- **Phase 3**: completed
- **Phase 4**: completed
- **Phase 5**: pending (**current priority**)
- **Phases 6-8**: pending

### Current work priority

While `Phase 5` remains open, prioritize:

- incremental migration to `AsyncSocket` with limits, timeouts, and backpressure
- latency and throughput telemetry in the migrated flows
- comparison against `ThreadPoolBenchmark` and `DownloadBenchmark`
- a clear fallback / no-regression path for asynchronous routes

Do not divert work toward REST, a new GUI, or larger migrations while phases 5-7 are still open.

### Dependencies (vcpkg)

- wxWidgets 3.3.1
- Boost 1.90.0 (asio, filesystem, regex, signals2, thread)
- Crypto++ (latest)
- zlib, libpng, libwebp, pcre2, tiff
- libupnp (optional; may require specific validation with `ENABLE_UPNP=ON`)

### Key libraries and subsystems to respect

- **wxWidgets**: GUI and app lifecycle; avoid expanding its scope into reusable logic.
- **Boost.ASIO / sockets**: existing network base; do not replace it without an incremental plan.
- **Crypto++**: used for cryptographic hardening, including PBKDF2-HMAC-SHA256 in EC credentials.
- **libupnp**: sensitive integration; always validate LAN-only behavior, retries, and linking when `ENABLE_UPNP=ON`.
- **MuleUnit**: current test framework; any new suite must be integrated in `unittests/tests/CMakeLists.txt`.

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
- Break the CMake configuration
- Assume Bash, GNU tools, or Linux paths exist in the environment
- Present `docs/blueprint.md` as if it already matched the current implementation

---

## Git Workflow

- Work on feature branches; avoid direct commits to `main`.
- Before opening a PR: run `cmake --build . --config Debug` and `ctest --output-on-failure -C Debug`.
- Every PR must have human review + green CI before merge.
- Separate security-critical changes from cosmetic refactors to make review easier.
- If the change closes or moves phase work, update `docs/PLAN_MODERNIZACION_2.0.md` with status, validations, and notes.

## Documentation and phase closeout

When asked to "document" or when a relevant phase/change is closed, the agent must:

1. Move the finished phase or work from `docs/PLAN_MODERNIZACION_2.0.md` to `docs/PLAN_MODERNIZACION_COMPLETADO.md`.
2. Update `README.md` if the change affects the description, status, or visible usage of the project.
3. Update the `### Status and priorities according to \`docs/PLAN_MODERNIZACION_2.0.md\`` block in `AGENTS.md` to reflect the real status.

### Recommended change strategy for agents

1. Explore the specific subsystem.
2. Identify risks around globals, timers, persistence, and protocol behavior.
3. Propose an incremental change.
4. Implement within a narrow scope.
5. Validate with focused tests and, when appropriate, a full build/test run.
6. Update the relevant documentation.

---

## Troubleshooting

### wxWidgets not found

```powershell
# Use the vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### UPnP target not found

```powershell
# Disable UPnP
cmake .. -DENABLE_UPNP=OFF
```

### config.h not found

Ensure the build directory is used (not the source tree). Check `${wMule_BINARY_DIR}` is in the include paths.

---

## Editor and LSP setup (VSCode)

### Files used by the editor setup

- `.vscode/c_cpp_properties.json` - C/C++ IntelliSense configuration
- `.vscode/settings.json` - CMake, clangd and editor settings
- `do_build_ninja.bat` - Helper script for generating `compile_commands.json`

### To make clangd work

1. Install the "clangd" and "CMake Tools" extensions
2. Run `do_build_ninja.bat` to generate `compile_commands.json` in `build-ninja/` (requires Ninja + vcvars)
3. Reload VSCode window: `Ctrl+Shift+P` → "Developer: Reload Window"
4. clangd will use `compile_commands.json` to resolve wxWidgets headers

### Generate `compile_commands.json`

```powershell
# Option 1: Using the batch script
K:\wMule\do_build_ninja.bat   # generates `build-ninja/` and `compile_commands.json`

# Option 2: Manual (requires vcvars64.bat)
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist build-ninja mkdir build-ninja
cd build-ninja
cmake -G Ninja .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Shell note

- Use **PowerShell** for documentation and automation examples.
- If documenting batch files (`.bat`), clearly state whether they should be run from `cmd.exe` or PowerShell.

Note: `compile_commands.json` lives in `build-ninja/`; point clangd at that directory. It is generated with Ninja, not Visual Studio.

---

## Version Info

From `src/include/common/ClientVersion.h`:

```cpp
#define VERSION      1.0.3
#define MOD_VERSION "1.0.3"
#define MOD_VERSION_LONG "wMule 1.0.3"
```
