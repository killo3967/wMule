# Changelog wMule

## [Pendiente] - Tareas Futuras

### Header simplificado (Opción 2 elegida)
Reemplazar headers GPL largos (~24 líneas) por versión simplificada (~6 líneas):
```cpp
//
// wMule - Windows Mule Client
// Copyright (c) 2026 Tomas Platero
// Copyright (c) 2003-2011 aMule Team (admin@amule.org)
//
// Licensed under GPL v2+ (see COPYING for details)
//
```
- Aplicar a todos los archivos .cpp y .h
- Mantener legalidad GPL v2+

---

## [1.0.3] - 2026-04-05

### Deuda Fase 4 – Conversions & Build Cleanliness
- Contadores y puertos de `BaseClient`, `CClientList`, `PartFile`, `UploadClient`, `ServerSocket` y preferencias migrados a tipos consistentes (`uint32`, `uint64`, `uint16`) con clamps explícitos, eliminando los últimos truncamientos reportados por MSVC (C4242/C4302).
- `GuiEvents`, `KnownFileList` y los helpers de hash actualizados para tratar correctamente `wxChar`, `uint64 transferred` y codificación en `MD4Hash`.
- `config.h.cm` condiciona las macros `PACKAGE*` y `CMakeLists.txt` define `_CRT_SECURE_NO_WARNINGS`, apagando el ruido C4996 heredado de wxWidgets.
- Los lexers de flex (`Scanner.cpp`, `IPFilterScanner.cpp`) quedan congelados por defecto y se añade la opción `WMULE_USE_FLEX` para regenerarlos bajo demanda; el código generado deja de usar `register` y `strdup` inseguros.

### Validaciones
- `cmake --build . --config Debug`
- `ctest --output-on-failure -C Debug`

---

## [1.0.2] - 2026-04-04

### Robustez x64 y Memoria (Fase 3)
- IDs de búsqueda y resultados (`SearchList`, GUI y clientes EC) migrados a `wxUIntPtr`/`uint64_t`, evitando truncaciones de punteros o tamaños grandes en modo remoto.
- `TextClient` y `wmulecmd` muestran correctamente volúmenes > 4 GB al usar `uint64_t` para `lFileSize` y formateo seguro.
- `OtherStructs.h` y los headers empaquetados (`Header_Struct`, `ServerMet_Struct`, `Requested_Block_Struct`, etc.) incluyen `static_assert(sizeof)` documentados, detectando cualquier cambio involuntario en el layout binario.
- Eliminados los últimos `sizeof(int)` heredados (estatísticas, sockets) y revisados los `memcpy`/`memmove` dinámicos para asegurar tamaños correctos en x64.

### Validaciones
- `cmake --build build-ninja --config Debug`
- `ctest --output-on-failure -C Debug`
- Smoke test manual (`wmule.exe` + `wmulecmd.exe`) cubriendo búsquedas extendidas y comandos EC.

---

## [1.0.1] - 2026-03-27

### Hardening de rutas y config remota (Fase 2)
- Categorías (GUI + EC/Web) solo aceptan directorios bajo Incoming o raíces compartidas declaradas; migraciones automáticas a Incoming cuando se detectan rutas heredadas inseguras.
- `SharedFileList`, `DirectoryTreeCtrl` y los conectores EC/Web sanitizan rutas antes de persistirlas (`shareddir.dat` ya no almacena entradas relativas/duplicadas ni carpetas prohibidas).
- `NormalizeRenameTarget` bloquea renombres con separadores/`..` y `CSharedFileList::RenameFile` evita mover ficheros fuera de las carpetas compartidas.
- `PartFileConvert` valida el origen, restringe el borrado (`deleteSource`) a Temp/Incoming y evita traversal cuando se importan `.part` o `.sd`.
- `ED2KLinkParser` y `--config-dir` normalizan rutas, garantizando que `ED2KLinks` se escriban siempre dentro del config dir seguro; las colecciones `.emulecollection` deben ser absolutas válidas.
- Tests MuleUnit (`PathTraversalTest`) ampliados con `IsSubPathOf` y casos positivos/negativos de normalización.

### Validaciones
- `cmake --build . --config Debug` generó `wmule.exe` y `wmulecmd.exe` actualizados.
- `ctest -R PathTraversalTest -C Debug --output-on-failure` cubre los nuevos helpers.
- Smoke test manual: `wmule.exe` conectó a servidores, ejecutó búsquedas extendidas con filtros, descargó y compartió ficheros usando rutas saneadas.

---

## [1.0.0] - 2026-03-24

### Fork wMule
- Renombrado de aMule a wMule
- Versión 1.0.0
- Plataforma: Windows x64 únicamente

---

## [2.4.0] - 2026-03-23 (aMule original)

### Cambios principales
- Migración completa a CMake para Windows x64
- Compilación con Visual Studio 2022 y MSVC
- Soporte para wxWidgets 3.3.1
- Integración con vcpkg para dependencias

#### Tests corregidos (100% pasando)
- **CTagTest**: Corregido encoding UTF-8 (compatibilidad Windows)
- **CUInt128Test**: Removidas assertions de debug no compatibles con Release
- **FileDataIOTest**: Reemplazados strings UTF-8 problemáticos con ASCII
- **FormatTest**: Removidas assertions de debug no compatibles con Release
- **NetworkFunctionsTest**: Habilitada dependencia wxWidgets::NET
- **PathTest**, **RangeMapTest**, **StringFunctionsTest**, **TextFileTest**: Compilación exitosa

#### Cambios técnicos
- `CMakeLists.txt`: Versión actualizada a 2.4.0
- `cmake/options.cmake`: Habilitado wx_NEED_NET para tests
- `src/CMakeLists.txt`: 
  - Corregido target_link_libraries (keyword syntax)
  - Comentado amuled y amulegui (requieren Windows SDK completo)
- `do_configure.ps1`: Configurado BUILD_MONOLITHIC=ON
- `src/amuled.cpp`: Corregido wxApp::OnRun → wxAppConsole::OnRun
- Tests: Modificados para compatibilidad con Release build

### Componentes incluidos
- ✅ **amule.exe** - Cliente principal GUI
- ✅ **amulecmd.exe** - Cliente línea de comandos
- ⏸️ **amuled.exe** - Daemon (deshabilitado, requiere Windows SDK)
- ⏸️ **amulegui.exe** - Cliente remoto (deshabilitado, requiere Windows SDK)

### Dependencias
- wxWidgets 3.3.1
- Boost 1.90.0
- Crypto++
- zlib 1.3.1

### Tests
- 9/9 tests unitarios pasando
- Framework: MuleUnit

---

## [2.3.3] - Versión anterior
- Compatible con Windows x64
- Build system CMake
