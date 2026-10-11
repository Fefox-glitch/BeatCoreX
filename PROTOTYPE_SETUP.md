# Construir y probar el personaje base

Estado de esta entrega: **BLOCKED para compilación y ejecución Unreal**. Se añade proyecto/código y generador de contenido. Los seis `.uasset`, dos Blueprints y el `.umap` no están generados todavía: requieren UE5.6, el módulo compilado y ejecutar el script en el editor. La configuración apunta a esas rutas futuras; abrir el proyecto antes de generarlas puede avisar de mapa/GameMode ausentes. La entrega aún no es un prototipo jugable validado.

## Construcción y contenido

Con UE5.6 y VS2022/MSVC/Windows SDK compatibles instalados, desde la raíz:

```powershell
.\Scripts\build_editor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.6'
```

Exigir exit code 0 y revisar `Saved/Validation/EditorBuild.log`. Después abrir `.uproject` en UE5.6, crear/abrir un nivel temporal vacío sin cambios pendientes y ejecutar `Scripts/create_prototype_assets.py` mediante Tools → Execute Python Script. El script crea `/Game/BeatCoreX/Input/IA_Move`, `IA_Look`, `IA_Jump`, `IA_Punch`, `IA_Kick`, `IA_Headbutt`, `IMC_Gameplay`, BP_BCX_PlayerCharacter, BP_BCX_PrototypeGameMode y Map_Prototype_Combat. Usa solo cubos del motor como placeholders. Rechaza rutas ya existentes; un fallo parcial requiere inspeccionar los assets creados antes de reintentar, sin borrado automático. Compilar y revisar ambos Blueprints en el editor, guardar contenido y añadir los binarios mediante Git LFS en esta misma rama.

## Comportamiento implementado en fuente

WASD mueve respecto al yaw de cámara; ratón rota cámara; Espacio inicia/libera salto; clic izquierdo Punch, E Kick y Q Headbutt. Estos tres ataques solo emiten `OnAttackIntent` y una línea `BCX_ATTACK` en Output Log; no hay animación, daño, IA ni evaluación musical.

Los siete handlers capturan `FPlatformTime::Seconds()` como primera instrucción. Movimiento/cámara usan Triggered continuo; salto y ataques usan Started, con Completed/Canceled para terminar salto. Las acciones Bool no llevan Pulse/Hold/Pressed triggers que alteren la transición. No añadir nodos input duplicados en Blueprint. Cada intención contiene tipo, InputTimeSeconds double original y la pareja positiva int64 SessionGeneration+AttackId. El GameInstance asigna IDs compartidos por sesión; un nuevo mapa/GameMode incrementa generación y reinicia contador. Reaparecer/volver a poseer no reinicia IDs. No son timestamps de audio ni IDs truncados a int32.

El contexto se instala solo para PlayerController local con LocalPlayer/subsystem/configuración y sesión válidos, en posesión y PawnClientRestart. HasMappingContext evita Add repetidos; se retiran únicamente los mappings añadidos por este personaje y bindings propios. Las teclas ya pulsadas al añadir el contexto se ignoran hasta liberar. Referencias ausentes, tipo de acción incorrecto, acciones duplicadas o sin entrada en el contexto deshabilitan los bindings con error explícito.

## Validación pendiente en UE5.6

| Comprobación | Procedimiento / evidencia requerida | Estado actual |
| --- | --- | --- |
| Editor Win64 | Build sin Live Coding, exit 0 y log | BLOCKED |
| Assets y Blueprints | Ejecutar generador; compilar ambos BP sin errores; guardar mapa | BLOCKED |
| Movimiento/cámara/salto | PIE y Standalone: WASD, ratón, Space, suelo/colisión/bloque de salto | BLOCKED |
| Ataques y tecla mantenida | Mantener cada ataque 2 s: una línea BCX_ATTACK; liberar/pulsar: exactamente una nueva; repetir a 30/60/120 FPS | BLOCKED |
| Identidad y tiempo | Comparar logs: IDs consecutivos entre tipos y timestamps monotónicos; respawn conserva secuencia, nueva sesión incrementa generación | BLOCKED |
| Posesión/configuración | Reposeer/reiniciar pawn, sin duplicación; quitar una acción en BP de ensayo y verificar error sin crash | BLOCKED |
| Test nativo | Session Frontend → Automation → BeatCoreX.Core.SessionIdentity | BLOCKED |

No se localizaron engine registrado ni toolchain en las rutas consultadas. El preflight de `build_editor.ps1` devuelve BLOCKED, exit 2: no es un intento de compilación ni un resultado UBT. Checks de JSON, sintaxis Python y diff no sustituyen UHT, build, compilación BP ni pruebas de input. Quartz, MetaSounds, combos, daño, IA y multijugador no forman parte de los archivos fuente añadidos. Se conservan los 13 DOCX y el commit documental; no se cambia main ni se realiza merge.

APIs consultadas: [Enhanced Input 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EnhancedInputLocalPlayerSubsystem?application_version=5.6), [mapping structs](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EnhancedActionKeyMapping?application_version=5.6), [BlueprintEditorLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/BlueprintEditorLibrary?application_version=5.6) y [LevelEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/LevelEditorSubsystem?application_version=5.6). Firmas C++ y comportamiento de serialización aún requieren verificación contra instalación UE5.6 real.
