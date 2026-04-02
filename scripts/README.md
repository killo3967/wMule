# Scripts de Build para wMule

Este directorio contiene scripts de PowerShell para automatizar la configuración, compilación y mantenimiento del proyecto **wMule (Windows Mule)** en Windows 11 con MSVC 2022 y vcpkg.

---

## 📜 Scripts disponibles

| Script | Descripción |
|--------|-------------|
| [`build-wmule.ps1`](build-wmule.ps1) | Script **principal** que combina configuración y compilación. Permite operaciones completas o parciales (solo configurar, solo compilar, reconstruir, limpiar). |
| [`sync_skills_library.ps1`](sync_skills_library.ps1) | Script auxiliar (no relacionado con el build): sincroniza la biblioteca de skills de agentes. |

---

## ▶️ Uso recomendado

### 1. Build completo (configurar + compilar)
```powershell
Set-Location K:\wMule\scripts
.\build-wmule.ps1
```
- Genera:
  - `K:\wMule\build\src\Debug\wmule.exe` (GUI principal)
  - `K:\wMule\build\src\Debug\wmulecmd.exe` (interfaz CLI)

### 2. Solo recompilar (después de modificar código)
```powershell
.\build-wmule.ps1 -compilar
```
> **Nota**: Asume que ya se ejecutó la configuración previamente. Si no, fallará.

### 3. Reconstruir desde cero (limpiar + configurar + compilar)
```powershell
.\build-wmule.ps1 -reconstruir
```
Útil cuando se cambian opciones de CMake o se actualizan dependencias.

### 4. Limpiar el directorio de build
```powershell
.\build-wmule.ps1 -limpiar
```
Elimina `K:\wMule\build\` y todo su contenido.

### 5. Ver ayuda
```powershell
.\build-wmule.ps1 -help
```
Muestra todas las opciones disponibles.

---

## ⚙️ Características de `build-wmule.ps1`

- **Variables declaradas al inicio**: Todas las configuraciones (rutas, tipo de build, generador, flags) están al comienzo del script con comentarios explicativos.
- **Entorno configurado automáticamente**: Añade al `PATH` y `LIB` las rutas necesarias de MSVC 2022 y Windows SDK.
- **Salida informativa**: Mensajes coloreados (Cyan = info, Green = éxito, Yellow = advertencia, Red = error).
- **Manejo de errores**: Verifica códigos de salida de CMake y del compilador, deteniéndose ante fallos.
- **Modularidad interna**: Operaciones divididas en funciones (`Setup-BuildEnvironment`, `Run-CmakeConfiguration`, `Run-Build`, `Clean-BuildDir`).
- **Compatible con los flujos documentados en `AGENTS.md` y `BUILD_MEMORY.md`**:
  - Usa `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`
  - Respeta los flags `BUILD_MONOLITHIC`, `BUILD_AMULECMD`, `BUILD_TESTING`.
  - Genera ejecutables en `src\Debug\` (o `src\Release\` según configuración).

---

## 📍 Ubicación de los ejecutables tras el build

| Tipo | Ruta relativa al proyecto | Ruta absoluta (ejemplo) |
|------|---------------------------|--------------------------|
| `wmule.exe` (GUI) | `build\src\<BuildType>\wmule.exe` | `K:\wMule\build\src\Debug\wmule.exe` |
| `wmulecmd.exe` (CLI) | `build\src\<BuildType>\wmulecmd.exe` | `K:\wMule\build\src\Debug\wmulecmd.exe` |
| Símbolos (`.pdb`) | Mismo directorio que los `.exe` | Útiles para depuración con WinDbg o Visual Studio |

---

## 🛠️ Requisitos previos

Antes de usar cualquiera de estos scripts, asegúrate de tener:

1. **vcpkg** instalado en `C:\vcpkg` con al menos los paquetes:
   - `wxwidgets3.3:x64-windows`
   - `boost-asio boost-filesystem boost-regex boost-signals2 boost-thread:x64-windows`
   - `cryptopp:x64-windows`
   - `zlib:x64-windows`
   - `miniupnpc:x64-windows` (si `ENABLE_UPNP=ON`)
   - `libpng libwebp pcre2 tiff:x64-windows`
   (Puedes instalarlos con: `.\vcpkg install <package>:x64-windows`)

2. **MSVC 2022 Build Tools** (incluyendo `cl.exe` y `link.exe`) accesibles vía:
   - `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64`

3. **CMake** versión 3.15+ disponible en el `PATH` del sistema.

4. **Windows 11 x64** (plataforma objetivo del proyecto).

---

## 📝 Notas importantes

- El directorio de build (`K:\wMule\build`) está pensado para estar **fuera del código fuente**, facilitando limpiezas completas sin riesgo de borrar archivos fuente.
- Siempre revisa la salida del script: al final indica la ubicación exacta de los ejecutables generados.
- Si deseas compilar en modo **Release**, modifica la variable `$BuildType = "Release"` dentro de `build-wmule.ps1` o pásala como argumento (extensión futura).

---

## 🙋‍♂️ Soporte

Para dudas relacionadas con el build de wMule, consulta:
- `K:\wMule\AGENTS.md` (sección *Build Commands*)
- `K:\wMule\docs\BUILD_MEMORY.md` (guía detallada de configuración y troubleshooting)
- o contacta al mantenedor del proyecto.
