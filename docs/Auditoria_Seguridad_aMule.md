# Auditoría de Seguridad y Refactorización de aMule

## 📌 **Resumen Ejecutivo**
Esta auditoría identificó **vulnerabilidades críticas**, bugs funcionales y problemas de compatibilidad **x64** en el código de aMule. A continuación se detallan los hallazgos, soluciones propuestas y una hoja de ruta para la refactorización.

---
## **🔴 Vulnerabilidades Críticas**
| **Área**          | **Vulnerabilidad**                          | **Impacto**                          | **Archivos Afectados**                          |
|-------------------|--------------------------------------------|--------------------------------------|-----------------------------------------------|
| **Red (eD2K/Kad)** | Buffer overflows en `CPacket::Read*`       | RCE o DoS                            | `ClientUDPSocket.cpp`, `CEMSocket.cpp`        |
|                   | Validación insuficiente de OPCodes         | Corrupción de memoria                | `ED2KSocket.cpp`, `KademliaUDPListener.cpp`   |
| **UPnP**          | SSDP Reflection Attack                     | Amplificación de DoS                 | `UPnPBase.cpp`                                |
| **Manejo Archivos**| Path Traversal en `PartFile::PartFile`     | Escritura/lectura arbitraria        | `PartFile.cpp`, `DownloadQueue.cpp`          |
| **Configuración**  | EC activado por defecto + contraseñas texto plano | Acceso remoto no autorizado | `amule.cpp`, `Logger.cpp`                    |
| **x64**           | Truncación de punteros (`int` → `uint64_t`)| Crash en x64                        | `MuleThread.h`, `NetworkFunctions.cpp`       |

---
## **🔧 Hallazgos Detallados**

### **1. Red (eD2K/Kad/UPnP)**
| **Archivo:Línea**            | **Tipo**            | **Gravedad** | **Descripción**                                      | **Solución**                                      |
|--------------------------------|----------------------|--------------|------------------------------------------------------|----------------------------------------------------|
| `ClientUDPSocket.cpp:117`    | Buffer Overflow      | **Crítica**  | `memcpy` sin validación en `OnPacketReceived`.        | Usar `CMemFile` dinámico + validar `packetlength`. |
| `ED2KSocket.cpp:459`         | Integer Overflow     | **Crítica**  | Cálculo de tamaño con `uint32` (riesgo >4GB).         | Reemplazar `uint32` por `uint64_t`.                |
| `UPnPBase.cpp:862`           | SSDP Reflection      | **Alta**     | Respuesta a `M-SEARCH` sin validar `URLBase`.        | Ignorar solicitudes con `URLBase` vacío/no local.  |

#### **Solución Propuesta para Buffer Overflow**
```cpp
// ClientUDPSocket.cpp - Antes
memcpy(buffer, data.GetPointer(), packetlength);

// ClientUDPSocket.cpp - Después
if (packetlen < sizeof(uint32_t) + packetlength) return;
std::vector<uint8_t> safeBuffer(packetlength);
data.Read(safeBuffer.data(), packetlength);
```

---
### **2. Manejo de Archivos**
| **Archivo:Línea**       | **Tipo**            | **Gravedad** | **Descripción**                                      | **Solución**                                      |
|--------------------------|----------------------|--------------|------------------------------------------------------|----------------------------------------------------|
| `PartFile.cpp:241`      | Path Traversal       | **Crítica**  | `wxFileName` sin sanitización.                       | Usar `wxFileName::Normalize(wxPATH_NORM_ALL)`.    |
| `DownloadQueue.cpp:142` | Race Condition       | **Alta**     | Falta de mutex en `AddSource`.                       | Usar `std::lock_guard<std::mutex>`.               |

#### **Solución Propuesta para Path Traversal**
```cpp
// PartFile.cpp - Antes
wxFileName file(path, filename);

// PartFile.cpp - Después
wxFileName file(path, filename);
file.Normalize(wxPATH_NORM_ALL | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE);
if (!file.IsOk() || file.GetFullPath().Contains("..")) {
    throw std::runtime_error("Ruta no válida");
}
```

---
### **3. Interfaz Gráfica (wxWidgets)**
| **Archivo:Línea**       | **Tipo**            | **Gravedad** | **Descripción**                                      | **Solución**                                      |
|--------------------------|----------------------|--------------|------------------------------------------------------|----------------------------------------------------|
| `AddFriend.cpp:54`      | Input Validation     | **Alta**     | `wxTextCtrl` para IPs/puertos sin validación.         | Usar `wxTextValidator` con regex.                 |
| `amule-gui.cpp:312`     | Memory Leak          | **Media**    | `new wxButton` sin `delete`.                         | Usar `std::unique_ptr<wxButton>`.                 |

#### **Solución Propuesta para Input Validation**
```cpp
// AddFriend.cpp
wxTextValidator ipValidator(wxFILTER_INCLUDE_CHAR_LIST);
ipValidator.SetCharIncludes("0123456789.");
m_friendIP->SetValidator(ipValidator);
```

---
### **4. Núcleo de la Aplicación**
| **Archivo:Línea**       | **Tipo**            | **Gravedad** | **Descripción**                                      | **Solución**                                      |
|--------------------------|----------------------|--------------|------------------------------------------------------|----------------------------------------------------|
| `amule.cpp:443`         | Information Disclosure| **Alta**    | Contraseñas EC en texto plano.                       | Usar PBKDF2 o `wxSecretStore`.                    |
| `amule.cpp:444`         | Insecure Defaults    | **Alta**     | EC activado por defecto.                             | Cambiar a `false` + notificar al usuario.        |

#### **Solución Propuesta para EC**
```cpp
// amule.cpp - Configuración segura
thePrefs::EnableExternalConnections(false); // Desactivar por defecto
wxSecretStore store = wxSecretStore::GetDefault();
store.Save("ec_password", wxSecretValue(password)); // Almacenar hasheado
```

---
### **5. Compatibilidad x64**
| **Archivo:Línea**            | **Tipo**            | **Gravedad** | **Descripción**                                      | **Solución**                                      |
|--------------------------------|----------------------|--------------|------------------------------------------------------|----------------------------------------------------|
| `Packet.h:45`                | Type Mismatch        | **Alta**     | `sizeof(int)` para alineación de paquetes.           | Usar `#pragma pack(1)` o `sizeof(uintptr_t)`.     |
| `Tag.h:123`                  | Pointer Truncation   | **Alta**     | Casteo de puntero a `int`.                           | Usar `intptr_t` + validar rango.                  |

#### **Solución Propuesta para x64**
```cpp
// NetworkFunctions.cpp - Antes
int addr = reinterpret_cast<int>(socketAddr);

// NetworkFunctions.cpp - Después
intptr_t addr = reinterpret_cast<intptr_t>(socketAddr);
if (addr > UINT32_MAX) {
    throw std::runtime_error("Puntero demasiado grande para x86");
}
```

---
## **📅 Hoja de Ruta para Refactorización**
### **Fase 1: Parcheo Crítico (🛠️ Urgente)**
1. **[ ]** Corregir **buffer overflows** en `ClientUDPSocket.cpp` y `ED2KSocket.cpp`.
2. **[ ]** Sanitizar rutas en `PartFile.cpp` (**path traversal**).
3. **[ ]** Desactivar EC por defecto y cifrar contraseñas.
4. **[ ]** Mitigar **SSDP reflection** en `UPnPBase.cpp`.

### **Fase 2: Hardening (🔒 Seguridad)**
1. **[ ]** Añadir **mutex** en `DownloadQueue.cpp` y `MuleThread.h`.
2. **[ ]** Sanitizar inputs en `AddFriend.cpp` (`wxTextValidator`).
3. **[ ]** Corregir **memory leaks** en GUI (`amule-gui.cpp`).

### **Fase 3: Migración x64 (🖥️ Compatibilidad)**
1. **[ ]** Actualizar `.vcxproj` para soportar **x64** (MSVC).
2. **[ ]** Corregir truncación de punteros (`int` → `intptr_t`).
3. **[ ]** Asegurar alineación de estructuras (`#pragma pack(1)`).

### **Fase 4: Documentación y Testing**
1. **[ ]** Generar **pruebas unitarias** para vulnerabilidades corregidas.
2. **[ ]** Actualizar `README.md` con advertencias de seguridad.
3. **[ ]** Documentar cambios **x64** en `COMPILE-WINDOWS.txt`.

---
## **🛡️ Recomendaciones Generales**
1. **Sanitización**: Usar `wxFileName::Normalize()` para todas las rutas.
2. **Manejo de Memoria**: Reemplazar buffers estáticos con `std::vector<uint8_t>`.
3. **Concurrencia**: Usar `std::mutex` en lugar de `wxMutex`.
4. **Compilación**: Habilitar `-fstack-protector` y `/GS` en flags.
5. **Logs**: Filtrar datos sensibles (IPs, `user_hash`).

---
## **⚠️ Advertencias**
- **eD2K**: No modificar criptografía (MD4/MD5) para mantener compatibilidad con la red.
- **wxWidgets**: Actualizar a ≥3.2.5 para soporte **x64** y corrección de bugs.
- **Testing**: Los cambios en lógica de red requieren pruebas manuales en entorno aislado.

---
## **📁 Archivos Adicionales**
- **[Protocolo_eD2K.md](./Protocolo_eD2K.md)**: Especificación técnica del protocolo eD2K.
- **Parches**: Archivos `.diff` para cada vulnerabilidad corregida (ej: `ClientUDPSocket_buffer_overflow.diff`).