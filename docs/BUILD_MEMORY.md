# wMule Build Memory - AI Agent Documentation

**Project:** wMule (Windows Mule)  
**Version:** 1.0.1  
**Base:** aMule 2.4.0  
**Date:** 2026-03-29  
**Author:** Tomas Platero  

---

## Project Origin

This project is a **fork of aMule** (multi-platform eMule client) renamed to **wMule** with version 1.0.0.  
**License:** GPL v2+ (inherited from aMule - cannot change to CC0)  
**Location:** K:\wMule  

---

## What Was Done

### 1. Project Copy & Rename
- Copied entire wMule project from K:\amule to K:\wMule
- Renamed project from "aMule" to "wMule"
- Set version to 1.0.0

### 2. Source Code Changes

#### Version Files
- `CMakeLists.txt`: Project name, version (1.0.0), package name
- `src/include/common/ClientVersion.h`: VERSION, MOD_VERSION_LONG

#### Namespace Change
- Only 2 files use `namespace amule`: `AsyncSocket.h` and `AsyncSocket.cpp`
- Changed to `namespace wmule` (including header guard)
- Other files use class names like `CamulewebApp` - NOT changed

#### Copyright Addition
- Added "Copyright (c) 2026 Tomas Platero" to ~200 files

#### Documentation
- Moved from `/documentacion` to `/docs`

#### Executable Names
- Renamed `amule` → `wmule` in `src/CMakeLists.txt`
- Renamed `amulecmd` → `wmulecmd` in `src/CMakeLists.txt`

---

## Key CMakeLists.txt Fixes

### Problem: `amule_BINARY_DIR` Not Defined
The variable `${amule_BINARY_DIR}` was used but never defined because the project was renamed.

**Solution:** Replace all instances with `${wMule_BINARY_DIR}`

**Files modified:**
- `unittests/muleunit/CMakeLists.txt` (line 16)
- `src/CMakeLists.txt` (lines 226, 361, 387)
- `src/libs/common/CMakeLists.txt` (line 15)
- `src/utils/wxCas/src/CMakeLists.txt` (line 13)
- `src/utils/aLinkCreator/src/CMakeLists.txt` (lines 9, 41)

### Problem: Executable Names
**Solution:** Changed `add_executable (amule` → `add_executable (wmule` and similar for amulecmd

---

## Release 1.0.1 – 2026-03-27

### Objetivo
Cerrar el bloque de hardening de rutas (Fase 2 – Opción A) y publicar un build verificable después de comprobar conectividad completa.

### Cambios principales
- **Normalización centralizada** de rutas de categorías y compartidos desde GUI y EC/Web (solo se aceptan directorios bajo Incoming o raíces declaradas).
- **Sanitización preventiva** en `PartFileConvert` y `Ed2kLinkParser`: rutas absolutas sin traversal, `deleteSource` limitado a Temp/Incoming y `ED2KLinks` siempre dentro del config dir normalizado.
- **Protecciones adicionales**: rename helper (solo nombres base), guardas en `SharedFileList`/`DirectoryTreeCtrl`, filtros PBKDF2 activos en conectores remotos y logging neutro de descartes.

### Validaciones
- `cmake --build . --config Debug` (wmule.exe y wmulecmd.exe recién generados en `build/src/Debug`).
- `ctest -R PathTraversalTest -C Debug --output-on-failure` para cubrir los helpers de rutas.
- Smoke test manual de `wmule.exe`: conexión a servidores, búsquedas extendidas con filtro y descargas/compartidos en los nuevos directorios normalizados.

### Resultado
- Se etiqueta como **wMule 1.0.1**, nuevo baseline para continuar el resto de Fase 2.
- Documentación (`AGENTS.md`, `PLAN_MODERNIZACION_2.0.md`, `CHANGELOG.md`) actualizada con el hito y las pruebas ejecutadas.

---

## Build Configuration

### Dependencies (via vcpkg)
- wxWidgets 3.3.1 (core, sound, debug-support)
- Boost 1.90.0 (asio, filesystem, regex, signals2, thread)
- cryptopp 2026-03-02
- zlib 1.3.1
- miniupnpc 2.2.x (habilitada con `ENABLE_UPNP=ON`; el wrapper `wmule_upnp_sdk` detecta `miniupnpc::miniupnpc` o variantes exportadas por vcpkg)
- libpng 1.6.55
- libwebp 1.6.0
- pcre2 10.47
- tiff 4.7.1

### CMake Configuration Command
```powershell
Set-Location K:\wMule\build
cmake .. `
    -DCMAKE_BUILD_TYPE=Debug `
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
    -DENABLE_UPNP=ON
```

### Build Command
```powershell
Set-Location K:\wMule\build
cmake --build . --config Debug
```

### Build Output
- `K:\wMule\build\src\Debug\wmule.exe` (13.9 MB)
- `K:\wMule\build\src\Debug\wmulecmd.exe` (1.9 MB)

### Run Tests
```powershell
Set-Location K:\wMule\build
ctest --output-on-failure -C Debug
```

**Result:** 12/12 tests passed

---

## CMake Variables Reference

| Variable | Value | Description |
|----------|-------|-------------|
| `CMAKE_BUILD_TYPE` | Debug/Release | Build configuration |
| `ENABLE_UPNP` | ON | UPnP support (wrapper `wmule_upnp_sdk` resuelve miniupnpc) |
| `BUILD_MONOLITHIC` | ON | Build wmule.exe |
| `BUILD_AMULECMD` | ON | Build wmulecmd.exe |
| `BUILD_TESTING` | ON | Build unit tests |

---

## Known Issues & Solutions

### Issue: miniupnpc CMake Target Not Found
**Error:**
```
Target "miniupnpc::miniupnpc" not found.
```

**Solution:** Instala miniupnpc vía vcpkg (`vcpkg install miniupnpc:x64-windows`). El wrapper `wmule_upnp_sdk` detecta `miniupnpc::miniupnpc`, `MINIUPNPC::miniupnpc` o `MINIUPNPC::miniupnpc-static`. Configura con `-DENABLE_UPNP=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`. Desactiva UPnP solo si no se necesita (`-DENABLE_UPNP=OFF`).

### Issue: wxWidgets Not Found by CMake
**Error:**
```
Could NOT find wxWidgets (missing: base)
```

**Solution:** Use vcpkg toolchain: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

### Issue: config.h Not Found
**Error:**
```
No se puede abrir el archivo incluir: 'config.h'
```

**Solution:** Ensure `${wMule_BINARY_DIR}` is in include paths (fixed by CMakeLists.txt edits above)

### Issue: "There are no translations installed for wMule"
When opening the language selector, wxWidgets does not find any `.mo` catalogs even though the `.po` files exist.

**Solution:** Usa el script `scripts/update-translations.ps1`, que compila todos los `.po` mediante `python` + `po2mo.py`, guarda los `.mo` en `assets/locale/<lang>/LC_MESSAGES/amule.mo` y sincroniza la carpeta `build/src/<Config>/locale`.

**Importante:** además el árbol de build debe configurarse con `ENABLE_NLS=ON`. Si `build/config.h` contiene `/* #undef ENABLE_NLS */`, el selector de idioma seguirá mostrando idiomas detectados, pero la UI arrancará siempre en inglés porque el runtime no llega a compilar la carga de catálogos.

```powershell
Set-Location K:\wMule\scripts
./update-translations.ps1 -CopyToBuild -Configs Debug
```

Los binarios siempre buscan catálogos debajo del ejecutable (`<exe>/locale/<lang>/LC_MESSAGES/amule.mo`), por lo que basta con copiar `assets/locale` después de ejecutar el script (el propio `-CopyToBuild` lo hace si el build existe).

---

## Important Paths

| Path | Description |
|------|-------------|
| `K:\wMule` | Project root |
| `K:\wMule\src` | Source code |
| `K:\wMule\build` | Build directory |
| `K:\wMule\docs` | Documentation |
| `K:\wMule\unittests` | Unit tests |
| `C:\vcpkg` | vcpkg installation |
| `C:\vcpkg\installed\x64-windows` | Installed packages |

---

## Files Generated During Build

- `build/config.h` - Generated by CMake configure
- `build/version.rc` - Generated from `version.rc.in`
- `build/src/libs/ec/cpp/ECCodes.h` - Generated
- `build/src/libs/ec/cpp/ECTagTypes.h` - Generated
- `build/pixmaps/flags_xpm/CountryFlags.h` - Generated

---

## Version Information

From `src/include/common/ClientVersion.h`:
```cpp
#define VERSION      1.0.1
#define MOD_VERSION "1.0.1"
#define MOD_VERSION_LONG "wMule 1.0.1"
```

---

## Future Maintenance Notes

1. **Do NOT rename class names** - Classes like `CamulewebApp` are intentional
2. **UPnP support** - `ENABLE_UPNP` espera `miniupnpc` vía vcpkg; `CMakeLists.txt` ya ejecuta `find_package(miniupnpc CONFIG REQUIRED)` y resuelve el target exportado.
3. **Copyright years** - Update to current year when making changes
4. **vcpkg packages** - May need regeneration with `vcpkg upgrade`
5. **wxWidgets version** - Currently 3.3.1, check compatibility before upgrading

---

## Contact

Original wMule: https://www.amule.org/  
This fork: Tomas Platero
