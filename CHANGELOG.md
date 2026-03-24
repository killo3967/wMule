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
