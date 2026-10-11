# Construir y probar el personaje base

Estado de esta entrega: **BLOCKED por toolchain Windows**. Migración incremental de PR #2 desde `05e5d97` a **UE5.8**, conservando personaje, cámara, Enhanced Input, GameInstance, SessionGeneration+AttackId y timestamp original. El motor instalado es **5.8.3**, pero el build real falla al validar el SDK. Los seis Input Actions, Mapping Context, dos Blueprints y mapa no se han generado: requieren build exitoso y ejecutar el script en el editor. Config apunta a esas rutas futuras y puede avisar de assets ausentes. No es un prototipo jugable validado.

## Decisión de versión y coordinación de PR

La instrucción de migración establece UE5.8 como objetivo de esta implementación; EngineAssociation=5.8 admite patches 5.8.x y el generador exige ese major/minor. PR #2 permanece en `feat/pr2-base-character-enhanced-input`, basada en el cierre documental `97a51a7`, con base de revisión `docs/auditoria-plan-prototipo-windows`. No se cambian PR #1, sus documentos, main ni los 13 DOCX.

Antes de fusionar ambas PR, revisar explícitamente en PR #1 la versión objetivo, requisitos VS/MSVC/SDK, comandos Build/UAT y tabla de APIs del informe y plan: hoy siguen diciendo UE5.6. Registrar la decisión UE5.8 y comprobar sus contratos contra este motor; los DOCX históricos deben conservarse y anotarse como referencias de UE5.6, sin reescribirlos. Ese ajuste documental no se hace automáticamente aquí. No hay merge automático ni nuevas ramas.

## Entorno y evidencia de build real

Auditoría 2026-10-11. `Engine/Build/Build.version` de `C:\Program Files\Epic Games\UE_5.8` informa Major=5, Minor=8, Patch=3, CL=58210709, CompatibleCL=55116800, Branch=`++UE5+Release-5.8`. Existen Build.bat y UnrealEditor.exe; aunque el manifiesto del Launcher todavía devuelve InstallationList vacía, la versión se confirma leyendo el motor real. `build_editor.ps1` detectó esa instalación sin argumento y la validó. No se detectó proceso UnrealEditor ni herramientas callable de unreal-mcp/proxy; no se alteró su configuración.

VS2022/VS2026: **no detectados** mediante rutas habituales, registro consultado ni vswhere (ausente). MSVC: **versión no disponible**, sin carpetas VC/Tools/MSVC en las rutas consultadas. Windows SDK: **no encontrado por UBT**, sin versiones Include en Windows Kits ni Installed Roots en el registro consultado. Una ruta externa personalizada debe verificarse con UBT; no confundir ausencia de PATH con incompatibilidad. DotNet **10.0 win-x64**, suministrado por el engine, sí ejecutó UBT; no es un compilador MSVC.

Comando realizado desde el checkout: `Scripts/build_editor.ps1`, que invocó:

```text
Build.bat BeatCoreXEditor Win64 Development -Project=<checkout>\BeatCoreX.uproject -WaitMutex -NoHotReloadFromIDE
Engine: C:\Program Files\Epic Games\UE_5.8 (5.8.3, CL 58210709)
Using bundled DotNet SDK version: 10.0 win-x64
Creating makefile for BeatCoreXEditor (no existing makefile)
Platform Win64 is not a valid platform to build. SDK validation failed:
  Sdk: not found. Required version 10.0.19041.0.
Result: Failed (OtherCompilationError)
Total execution time: 3.31 seconds
Editor build exit code: 6
```

El proceso sí ejecutó UBT y falló antes de la fase de compilación; **no hay evidencia de UHT ejecutado, objetos C++ compilados ni link exitoso**. No existen diagnósticos MSVC de este proyecto porque todavía no se llegó al compilador. Se conservan logs locales en `Saved/Validation/EditorBuild.log`, `EditorBuild.exitcode` (6), `EngineVersion.json`, `UnrealBuildTool.log` y `ToolchainInventory.json` (vswhere false, VS/MSVC/SDK vacíos). Saved sigue ignorado por Git. UBT limitó paralelismo a una acción por memoria disponible; no es la causa de este fallo.

## Compatibilidad comprobada en el motor instalado

Se inspeccionaron los headers y archivos UBT de **esta instalación 5.8.3**, además de la referencia oficial. No se creó un proyecto sustituto ni assets de prueba para evitar el bloqueo de build.

| Área | Verificación y adaptación | Evidencia dentro de Engine |
| --- | --- | --- |
| Targets | V7 es el default 5.8; Unreal5_8 existe, Unreal5_6 está deprecated. Ambos targets se fijan a V7/Unreal5_8 | Source/Programs/UnrealBuildTool/Configuration/Rules/TargetRules.cs |
| Mapping Context | Mappings superior deprecated desde 5.7; defaults en FInputMappingContextMappingData. Python usa InputMappingContextMappingData y default_key_mappings, leyendo mappings del struct | Plugins/EnhancedInput/Source/EnhancedInput/Public/InputMappingContext.h |
| Blueprints | CompileBlueprint ahora devuelve bool: se exige true antes de continuar o guardar; CreateBlueprintAssetWithParent/GeneratedClass siguen disponibles | Source/Editor/BlueprintEditorLibrary/Public/BlueprintEditorLibrary.h |
| Input nativo | GetMappings lee defaults actuales; HasMappingContext y RemoveBindingByHandle siguen disponibles. Se conservan Started y todos los handlers | Plugins/EnhancedInput/Source/EnhancedInput/Public/{InputMappingContext,EnhancedInputSubsystemInterface,EnhancedInputComponent}.h |
| Acciones y modifiers | ValueType Boolean/Axis2D, EnhancedActionKeyMapping, InputModifierNegate x/y/z, SwizzleAxis Order=YXZ y KeyName contrastados | Plugins/EnhancedInput/Source/EnhancedInput/Public/{InputAction,InputActionValue,InputModifiers,EnhancedActionKeyMapping}.h; Source/Runtime/InputCore/Classes/InputCoreTypes.h |
| Creación y guardado | DataAssetFactory.DataAssetClass, SaveLoadedAsset/DoesAssetExist, GetDirtyMapPackages/GetDirtyContentPackages presentes | Source/Editor/UnrealEd/Classes/Factories/DataAssetFactory.h; Plugins/Editor/EditorScriptingUtilities/Source/EditorScriptingUtilities/Public/EditorAssetLibrary.h; Source/Editor/UnrealEd/Public/FileHelpers.h |
| Nivel y actores | NewLevel(path,bIsPartitionedWorld), SaveCurrentLevel, IsInPlayInEditor, SpawnActorFromClass y GetAllLevelActors presentes | Source/Editor/LevelEditor/Public/LevelEditorSubsystem.h; Source/Editor/UnrealEd/Public/Subsystems/EditorActorSubsystem.h |
| Plugins/módulos/Config | EnhancedInput, PythonScriptPlugin y EditorScriptingUtilities existen; los dos últimos siguen limitados a Editor. Core/CoreUObject/Engine/InputCore/EnhancedInput y rutas Config se conservan | Plugins/EnhancedInput, Plugins/Experimental/PythonScriptPlugin, Plugins/Editor/EditorScriptingUtilities |

El generador valida clases y métodos reflejados, propiedades actuales y ausencia de PIE antes de modificar assets. Esta inspección verifica disponibilidad declarada, **no ejecución de wrappers Python, serialización ni compilación BP**. Eso queda bloqueado. [BlueprintEditorLibrary 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/BlueprintEditorLibrary) confirma el resultado bool de compile_blueprint. No se detectaron cambios que exijan alterar el código del personaje/identidad; su compatibilidad de compilación permanece pendiente por SDK.

## Construcción y contenido

El motor 5.8.3 ya está instalado. Para desbloquear:

1. Instalar o modificar Visual Studio con **Game development with C++**, herramientas MSVC x64/x86 y Windows SDK. [Instrucciones oficiales de Microsoft](https://learn.microsoft.com/en-us/visualstudio/gamedev/unreal/get-started/vs-tools-unreal-install).
2. Para esta build, `Engine/Config/Windows/Windows_SDK.json` prefiere VS2026/MSVC 14.50 o VS2022 17.14/MSVC 14.44. Seleccionar **14.50.35723 o posterior de la familia 14.50**, o **14.44.35211 o posterior de la familia 14.44**: el engine veta sus versiones anteriores y las familias 14.39–14.43. Mínimos declarados: VS2022 17.8 / VS2026 18.0; no basta con instalar un IDE sin el compilador.
3. Instalar **Windows SDK 10.0.22621.0**, versión principal de ese JSON. El mínimo admitido es 10.0.19041.0; el mensaje de UBT no implica que solo acepte el mínimo. No cambiar paths ni fingir una instalación para saltarse esa validación.
4. Reabrir terminal, ejecutar de nuevo el build y confirmar qué MSVC/SDK selecciona UBT en su log. Los requisitos anteriores proceden del motor instalado; los componentes recomendados en documentación genérica pueden diferir.

Desde la raíz, detección automática o ruta explícita:

```powershell
.\Scripts\build_editor.ps1
# Alternativa para otra instalación real:
.\Scripts\build_editor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
```

Exigir exit code 0, logs de UHT/C++/link y revisar `Saved/Validation/EditorBuild.log`. Solo entonces abrir `.uproject` en UE5.8, guardar el nivel/content actual y ejecutar `Scripts/create_prototype_assets.py` mediante Tools → Execute Python Script. El script crea `/Game/BeatCoreX/Input/IA_Move`, `IA_Look`, `IA_Jump`, `IA_Punch`, `IA_Kick`, `IA_Headbutt`, `IMC_Gameplay`, BP_BCX_PlayerCharacter, BP_BCX_PrototypeGameMode y Map_Prototype_Combat. Usa solo cubos del motor como placeholders. Rechaza rutas ya existentes; un fallo parcial requiere inspeccionar los assets creados antes de reintentar, sin borrado automático. El script exige éxito de compilación de ambos BP. Guardar y añadir los binarios reales mediante Git LFS en esta misma rama. En esta migración se generaron **cero assets**, respetando la puerta de compilación exitosa.

## Comportamiento implementado en fuente

WASD mueve respecto al yaw de cámara; ratón rota cámara; Espacio inicia/libera salto; clic izquierdo Punch, E Kick y Q Headbutt. Estos tres ataques solo emiten `OnAttackIntent` y una línea `BCX_ATTACK` en Output Log; no hay animación, daño, IA ni evaluación musical.

Los siete handlers capturan `FPlatformTime::Seconds()` como primera instrucción. Movimiento/cámara usan Triggered continuo; salto y ataques usan Started, con Completed/Canceled para terminar salto. Las acciones Bool no llevan Pulse/Hold/Pressed triggers que alteren la transición. No añadir nodos input duplicados en Blueprint. Cada intención contiene tipo, InputTimeSeconds double original y la pareja positiva int64 SessionGeneration+AttackId. El GameInstance asigna IDs compartidos por sesión; un nuevo mapa/GameMode incrementa generación y reinicia contador. Reaparecer/volver a poseer no reinicia IDs. No son timestamps de audio ni IDs truncados a int32.

El contexto se instala solo para PlayerController local con LocalPlayer/subsystem/configuración y sesión válidos, en posesión y PawnClientRestart. HasMappingContext evita Add repetidos; se retiran únicamente los mappings añadidos por este personaje y bindings propios. Las teclas ya pulsadas al añadir el contexto se ignoran hasta liberar. Referencias ausentes, tipo de acción incorrecto, acciones duplicadas o sin entrada en el contexto deshabilitan los bindings con error explícito.

## Validación en UE5.8

| Comprobación | Procedimiento / evidencia requerida | Estado actual |
| --- | --- | --- |
| Editor Win64 / UHT | Build sin Live Coding, exit 0, ejecución UHT y log | UBT ejecutado: FAILED exit 6 por SDK; UHT BLOCKED |
| Assets y Blueprints | Ejecutar generador; compilar ambos BP sin errores; guardar mapa | BLOCKED |
| Movimiento/cámara/salto | PIE y Standalone: WASD, ratón, Space, suelo/colisión/bloque de salto | BLOCKED |
| Ataques y tecla mantenida | Mantener cada ataque 2 s: una línea BCX_ATTACK; liberar/pulsar: exactamente una nueva; repetir a 30/60/120 FPS | BLOCKED |
| Identidad y tiempo | Comparar logs: IDs consecutivos entre tipos y timestamps monotónicos; respawn conserva secuencia, nueva sesión incrementa generación | BLOCKED |
| Posesión/configuración | Reposeer/reiniciar pawn, sin duplicación; quitar una acción en BP de ensayo y verificar error sin crash | BLOCKED |
| Test nativo | Session Frontend → Automation → BeatCoreX.Core.SessionIdentity | BLOCKED |

Checks estáticos: JSON del proyecto, sintaxis Python/PowerShell y diff revisados; no sustituyen build ni pruebas reales. No se abrieron mapa/BP ni se ejecutaron PIE/Standalone, movimiento, cámara, salto, teclas mantenidas, posesión, reinicio o test nativo. Sin Quartz, MetaSounds, combos, daño, IA enemiga ni multijugador. Los 13 DOCX conservan sus hashes frente a 05e5d97; los dos Markdown documentales y README permanecen intactos; main conserva dc8da7f. PR #2 permanece abierta en borrador y BLOCKED, sin merge.

Archivos modificados respecto a 05e5d97: BeatCoreX.uproject, Scripts/build_editor.ps1, Scripts/create_prototype_assets.py, Source/BeatCoreX.Target.cs, Source/BeatCoreXEditor.Target.cs y este PROTOTYPE_SETUP.md. No se reconstruye ni sustituye el proyecto. Las APIs y requisitos exactos se contrastan arriba con la instalación real, no solo mediante sustitución de cadenas de versión.
