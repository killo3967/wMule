# Guía corta para adaptar gentle-ai en un repo existente

## Objetivo

Usar `gentle-ai` y su ecosistema sin asumir cosas falsas sobre el proyecto, el entorno o la arquitectura real.

> **Importante:** todos los bloques `powershell` de esta guía deben ejecutarse en una consola PowerShell abierta en la raíz del repositorio, salvo que se indique lo contrario.

> **Importante:** todos los bloques `opencode` de esta guía deben escribirse en el chat/consola de OpenCode con el agente activo. No son comandos de PowerShell.

---

## 1. Sitúate en la raíz del repo

En Windows:

```powershell
Set-Location K:\TuRepo
```

Trabaja siempre desde la raíz del proyecto.

---

## 2. No empieces pidiendo features

Primero haz que el ecosistema entienda:

- el lenguaje real
- el sistema operativo real
- la shell real
- la arquitectura real
- las librerías reales
- el flujo real de build/test

Si no, los agentes trabajan sobre una ficción.

---

## 3. Adapta `AGENTS.md`

`AGENTS.md` debe reflejar:

- idioma de trabajo
- sistema operativo
- shell preferida
- stack real
- librerías usadas
- arquitectura actual real
- comandos de build/test
- archivos generados o sensibles
- forma correcta de pedir cambios

### Regla clave

Describe la **arquitectura actual**, no la ideal.

Ejemplo bueno:

- monolito modular en transición
- core acoplado a globals
- cambios incrementales con seams

Ejemplo malo:

- “core hexagonal” si todavía no existe de verdad

### Prompt largo recomendado para adaptar `AGENTS.md`

```opencode
Quiero que adaptes o crees el archivo `AGENTS.md` para este repositorio.

Objetivo:
Necesito que los agentes entiendan correctamente este proyecto antes de proponer cambios o escribir código.

Instrucciones:
1. Analiza el repositorio real, no inventes arquitectura ni convenciones.
2. Detecta:
   - lenguaje(s) y stack principal
   - sistema operativo objetivo
   - shell/entorno más seguro para trabajar
   - herramientas de build, test y validación
   - librerías y frameworks relevantes
   - estructura de carpetas importante
   - archivos generados o sensibles que no deben editarse
   - convenciones reales de código
   - riesgos técnicos del repo
   - forma correcta de pedir cambios a los agentes
3. Escribe o adapta `AGENTS.md` para que refleje:
   - idioma de interacción
   - entorno operativo real
   - comandos reales del proyecto
   - arquitectura actual real del código
   - prioridades técnicas
   - reglas de seguridad y estabilidad
   - estrategia de cambios incrementales
   - reglas de testing y validación
4. No describas la arquitectura objetivo como si ya estuviera implementada.
5. Si hay documentación tipo roadmap, blueprint o plan de modernización, úsala para distinguir:
   - estado actual real
   - arquitectura objetivo
   - prioridades vigentes
6. Si algo no está claro, no lo inventes: deja la duda explícitamente indicada.

Quiero que el resultado esté adaptado al:
- código fuente real
- lenguaje real del proyecto
- librerías reales usadas
- características intrínsecas del código
- documentación vigente del repositorio

Si encuentras diferencias entre la arquitectura documentada y la implementada, déjalo explícito dentro del `AGENTS.md`.
```

---

## 4. Inicializa SDD

Cuando el proyecto ya tiene contexto básico:

```opencode
/sdd-init
```

Eso prepara el contexto SDD y la persistencia.

---

## 5. Explora antes de tocar código

Antes de pedir cambios, conviene hacer una exploración real del repositorio para entender:

- arquitectura actual
- módulos principales
- entrypoints
- dependencias importantes
- flujo de datos
- zonas legacy
- puntos frágiles
- riesgos para cambios

El objetivo no es escribir código todavía, sino construir un mapa fiable del proyecto.

### Qué deberías pedir en esta fase

Pide al agente que te diga, como mínimo:

- qué existe realmente en el código
- qué aún no existe aunque aparezca en la documentación
- qué partes están más acopladas o son más frágiles
- qué subsistemas conviene tocar primero
- cómo conviene pedir cambios sin asumir una arquitectura inexistente

### Prompt largo recomendado para explorar el repo

```opencode
/sdd-explore Quiero que analices este repositorio antes de proponer cambios. Necesito una exploración técnica de la arquitectura real del proyecto, no de la arquitectura ideal documentada.

Analiza y resume:
1. módulos principales del sistema
2. entrypoints y binarios principales
3. dependencias relevantes entre subsistemas
4. flujo de datos y de control a alto nivel
5. componentes o carpetas reutilizables frente a zonas monolíticas o acopladas
6. zonas legacy, frágiles o de alto riesgo
7. uso de globals, estado compartido, timers, hilos, persistencia o protocolos sensibles
8. diferencias entre la arquitectura documentada y la implementada
9. riesgos concretos al introducir cambios
10. recomendaciones para trabajar con agentes sin romper el proyecto

Quiero que indiques explícitamente:
- qué partes del diseño objetivo ya existen
- qué partes todavía no existen
- qué tipo de cambios conviene pedir de forma incremental
- qué tipo de prompts conviene evitar porque asumirían una arquitectura falsa

No escribas código ni modifiques archivos. Quiero solo análisis, mapa mental del repo y recomendaciones prácticas para trabajar sobre él con seguridad.
```

### Señales de que la exploración ha sido buena

Una buena exploración debería devolverte:

- una descripción clara de la arquitectura real
- diferencias entre objetivo y realidad
- una lista de riesgos concretos
- una estrategia sensata para pedir cambios
- posibles primeros seams o subsistemas candidatos

---

## 6. Sincroniza `.atl/skill-registry.md`

Si usas subagentes, la skill registry debe heredar lo mismo que `AGENTS.md`:

- idioma
- OS
- shell
- arquitectura real
- prioridades del proyecto
- estilo de prompting

Si no haces esto, `AGENTS.md` dice una cosa y los subagentes hacen otra.

### Qué debería conseguir este paso

La idea no es copiar `AGENTS.md` entero, sino condensarlo en reglas compactas que los subagentes puedan reutilizar sin perder el contexto del proyecto.

La skill registry debería ayudar a que los subagentes:

- respeten el entorno real
- entiendan la arquitectura actual
- no asuman capas inexistentes
- apliquen las prioridades técnicas correctas
- trabajen con cambios incrementales y seguros

### Prompt largo recomendado para adaptar `.atl/skill-registry.md`

```opencode
Quiero que adaptes o generes `.atl/skill-registry.md` para este repositorio.

Objetivo:
Necesito que los subagentes trabajen con las mismas reglas, convenciones y restricciones que quedaron definidas en `AGENTS.md`, sin inventar arquitectura ni asumir un entorno distinto al real.

Instrucciones:
1. Toma como fuente principal `AGENTS.md` y el código real del repositorio.
2. Genera una skill registry compacta, clara y reutilizable por subagentes.
3. Asegúrate de reflejar explícitamente:
   - idioma de interacción
   - sistema operativo real
   - shell recomendada
   - stack/lenguaje real del proyecto
   - librerías y frameworks relevantes
   - arquitectura actual real
   - prioridades técnicas vigentes
   - archivos sensibles o generados que no deben tocarse
   - reglas de testing/validación
   - workflow de git y cambios incrementales
4. No copies `AGENTS.md` entero: condénsalo en reglas compactas, accionables y reutilizables por agentes.
5. Si existe una diferencia entre arquitectura objetivo y arquitectura actual, déjala explícita para que los subagentes no trabajen sobre supuestos falsos.
6. Adapta el contenido al tipo de repo real:
   - lenguaje principal
   - herramientas de build/test
   - organización de carpetas
   - riesgos intrínsecos del código
7. Añade una sección de guidance para prompting, indicando:
   - cómo pedir cambios seguros
   - cómo pedir seams o extracciones incrementales
   - qué tipo de peticiones evitar
8. Si detectas reglas de `AGENTS.md` que no estén representadas en la skill registry, incorpóralas.
9. Si hay ambigüedades o huecos entre `AGENTS.md`, el código y la documentación, déjalos anotados sin inventar.

Quiero que el resultado:
- esté alineado con `AGENTS.md`
- refleje el proyecto real
- sea útil para subagentes
- reduzca el riesgo de respuestas o cambios fuera de contexto
- evite asumir Bash, Linux o arquitecturas inexistentes si no corresponden
```

### Señales de que la skill registry ha quedado bien

Una buena skill registry debería:

- resumir sin duplicar `AGENTS.md`
- conservar reglas importantes del proyecto
- servir de guía práctica para subagentes
- reflejar el stack, el entorno y la arquitectura real
- evitar ambigüedades sobre cómo pedir cambios

---

## 7. Si usas GGA, inicialízalo por repo

```powershell
gga init
gga install
```

Después revisa `.gga`.

### Muy importante

La configuración por defecto puede venir adaptada a otro stack.

Verifica:

- provider real disponible
- patrones de archivos correctos
- exclusiones de build correctas

### Advertencia práctica

No asumas que `.gga` queda bien sólo por ejecutar `gga init`.

Revisa especialmente:

- si el `provider` configurado existe realmente en tu equipo
- si los patrones de archivos coinciden con el lenguaje del repo
- si `gga install` activó hooks que podrían bloquear commits si el provider no funciona

Si el provider configurado no está disponible localmente, el `pre-commit` puede fallar aunque la inicialización haya parecido correcta.

### Prompt largo recomendado para adaptar `.gga`

```opencode
Quiero que adaptes o revises el archivo `.gga` para este repositorio.

Objetivo:
Necesito que GGA quede configurado de forma coherente con el proyecto real, el lenguaje real, el entorno real y el provider realmente disponible en esta máquina.

Instrucciones:
1. Revisa la configuración actual de `.gga`.
2. Detecta si la configuración por defecto está pensada para otro stack o lenguaje.
3. Adapta `.gga` para que refleje correctamente:
   - provider realmente disponible
   - lenguaje principal del repo
   - extensiones/patrones de archivos relevantes
   - exclusiones de build, binarios y archivos generados
   - reglas compartidas del proyecto cuando corresponda
4. No asumas configuraciones por defecto válidas si el proyecto no usa ese stack.
5. Si el repo está en Windows, no presupongas Linux/Bash salvo que esté realmente disponible.
6. Si `gga install` activa hooks, verificá que la configuración no deje el repositorio bloqueado por un provider inexistente.
7. Si detectas que el provider actual no existe o no funciona localmente, propón una alternativa realista y explica el impacto.
8. Adapta los patrones para que GGA revise archivos útiles del proyecto y no pierda tiempo con extensiones ajenas al stack real.
9. No inventes herramientas ni providers que no estén realmente disponibles.

Quiero que el resultado final:
- sea usable en este entorno real
- no bloquee commits por configuración inválida
- esté alineado con el lenguaje y build del proyecto
- evite configuraciones genéricas pensadas para otro ecosistema
```

---

## 8. Comprueba que GGA funciona de verdad

No lo des por hecho:

```powershell
gga run
```

Si falla, corrige el provider o la configuración antes de seguir.

---

## 9. Ajusta `.gitignore`

Ignora:

- artifacts de build
- binarios
- dependencias generadas
- temporales de sesiones/agentes

Pero no ignores archivos compartidos útiles como:

- `AGENTS.md`
- `.gga`
- `.atl/skill-registry.md`

---

## 10. Haz un commit separado de tooling

Primero deja en un commit aparte:

- `AGENTS.md`
- `.gga`
- `.atl/skill-registry.md`
- ajustes de `.gitignore`

No lo mezcles con cambios funcionales del producto.

Ejemplo de commit:

```text
chore: configure agent tooling for existing repo
```

---

## 11. Solo entonces empieza el trabajo real

### Para entender algo

```opencode
/sdd-explore ...
```

### Para abrir un cambio nuevo

```opencode
/sdd-new ...
```

### Para planificación rápida

```opencode
/sdd-ff ...
```

### Para implementar

```opencode
/sdd-apply ...
```

---

## 12. Cómo pedir cambios correctamente

### Pedir así

- “extrae esta lógica a un servicio testeable”
- “encapsula esta infraestructura detrás de una API local”
- “reduce dependencia de globals”
- “introduce un seam”
- “haz un cambio incremental compatible”

### Evitar pedir así

- “pásalo a hexagonal”
- “métele esto en la capa de aplicación”
- “conéctalo al domain kernel”

salvo que esas capas existan realmente en el repo.

---

## Checklist rápida

1. abrir el repo en la raíz  
2. adaptar `AGENTS.md`  
3. ejecutar `/sdd-init`  
4. ejecutar `/sdd-explore ...`  
5. adaptar `.atl/skill-registry.md`  
6. si usas GGA, ejecutar `gga init` + `gga install` y revisar `.gga`  
7. ajustar `.gitignore`  
8. hacer un commit separado para el tooling  
9. solo entonces empezar con los cambios funcionales  

---

## Regla final

`gentle-ai` no se adapta mágicamente al repo.

Hay que darle:

- contexto
- límites
- lenguaje
- arquitectura real
- workflow real

Cuando haces eso, el ecosistema funciona mucho mejor.
