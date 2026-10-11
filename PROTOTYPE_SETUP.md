# Construir y probar el personaje base

Validación 2026-10-11 reanudada desde `592487a`, en `feat/pr2-base-character-enhanced-input`. **PASS: build real UE5.8.3, UHT, C++/linker, diez assets reales, Blueprints y pruebas automatizadas en PIE. BLOCKED: publicación de assets en GitHub LFS por presupuesto agotado.** No se rehizo la migración. La prueba física de teclado/ratón y Standalone siguen pendientes; no se confunden con la inyección de input en PIE. PR #2 permanece en borrador, sin merge.

## Toolchain y build real

UBT detectó motor **5.8.3**, CL **58210709**, CompatibleCL **55116800**, en `C:\Program Files\Epic Games\UE_5.8`. Usó DotNet **10.0 win-x64** suministrado por el engine, Visual Studio **18 Insiders**, MSVC **14.51.36260** (familia/carpeta 14.51.36231, x64, Preview) y Windows SDK **10.0.26100.0**.

Contrastado con `Engine/Config/Windows/Windows_SDK.json`: SDK dentro de MinVersion=10.0.19041.0 y MaxVersion=10.9.99999.0; MSVC por encima del mínimo 14.38.33130 y fuera de rangos vetados. **MSVC 14.51 no es preferido**: UBT advierte que es más nuevo que las familias preferidas 14.50/14.44. El build pasó con esa advertencia; no se presenta Preview como toolchain preferida por Epic.

Se ejecutó `Scripts/build_editor.ps1` desde la raíz, invocando BeatCoreXEditor Win64 Development, sin Live Coding:

```text
UHT processed BeatCoreXEditor in 3.8399505 seconds (10 generated files written)
Using Visual Studio 14.51.36260 toolchain ... and Windows 10.0.26100.0 SDK
[11/12] Link [x64] UnrealEditor-BeatCoreX.dll
[12/12] WriteMetadata BeatCoreXEditor.target [NoUba]
Result: Succeeded
Total execution time: 170.27 seconds
Editor build exit code: 0
```

**PASS** de UBT/UHT/C++/linker. No hubo errores C++/UHT/link que requirieran cambios. Permanecen warnings C4996 en headers del motor y el warning MSVC Preview. El fallo de entorno anterior (SDK ausente, exit 6 en `592487a`) está resuelto por la toolchain ahora instalada; no se omitió validación alguna. [Build completo](Validation/UE5.8.3/EditorBuild.log), [versión del engine](Validation/UE5.8.3/EngineVersion.json).

## Assets y errores corregidos

UE5.8.3 abrió el proyecto. El generador falló realmente en `unreal.Key(key_name=...)`: su constructor Python no acepta ese argumento. Se cambió a `unreal.Key()` y `set_editor_property("key_name", key)`. La primera ejecución había guardado seis Input Actions; se reutilizaron con las funciones del mismo script para terminar, sin eliminarlas ni fabricar archivos. El generador sigue rechazando sobreescrituras por defecto.

Diez assets serializados por Unreal y versionados mediante Git LFS:

| Tipo | Ruta bajo Content/BeatCoreX |
| --- | --- |
| InputAction Axis2D | Input/IA_Move.uasset; Input/IA_Look.uasset |
| InputAction Bool | Input/IA_Jump.uasset; Input/IA_Punch.uasset; Input/IA_Kick.uasset; Input/IA_Headbutt.uasset |
| Mapping Context | Input/IMC_Gameplay.uasset |
| Personaje BP | Characters/Player/BP_BCX_PlayerCharacter.uasset |
| GameMode BP | Blueprints/Core/BP_BCX_PrototypeGameMode.uasset |
| Mapa graybox | Maps/Map_Prototype_Combat.umap |

Son **nueve .uasset y un .umap reales**. Ambos BP devolvieron éxito de CompileBlueprint y se guardaron de nuevo al finalizar. El editor confirmó clases InputAction/InputMappingContext/Blueprint/World y diez mappings: W/S/A/D, MouseX/MouseY, SpaceBar, LeftMouseButton, E y Q. [Inventario](Validation/UE5.8.3/AssetInventory.json).

La subida LFS real fue rechazada: `This repository exceeded its LFS budget. The account responsible for the budget should increase it to restore access.` Se subieron **0/10 objetos**. Los assets están versionados LFS en un commit local separado; no se publican punteros remotos sin sus objetos. Código e informes se publican por separado en la misma rama. Se entrega además un ZIP local con el Content real; no se convierten los binarios a Git normal ni se cambia facturación. Para cerrar la publicación de PR #2, el responsable debe restablecer cuota LFS y luego hacer push del commit local pendiente.

## Pruebas reales y límites

`Scripts/validate_prototype_pie.py` se ejecutó en un **mundo PIE real**, con el personaje y GameMode generados. Inyecta valores por frame mediante EnhancedInputLocalPlayerSubsystem; no llama directamente a handlers. Prueba gameplay y Started reales, pero no acredita teclado/ratón físico ni el recorrido de eventos de hardware por los mappings.

| Comprobación | Estado | Evidencia |
| --- | --- | --- |
| Editor Win64 / UHT / C++ / linker | PASS | Exit 0; diez archivos UHT |
| Assets / BP / mapa | PASS | Diez assets, dos BP compilados y guardados |
| Publicación GitHub LFS | BLOCKED | Presupuesto del repositorio excedido; 0/10 objetos aceptados |
| Movimiento | PASS | Desplazamiento medido superior a 100 cm |
| Cámara | PASS | Cambio de yaw/pitch; distancia cámara-personaje 358.225 cm |
| Salto | PASS | Elevación medida superior a 30 cm |
| Punch / Kick / Headbutt | PASS | Tipos recibidos en OnAttackIntent y log nativo |
| Entrada sostenida | PASS con inyección | Dos segundos por ataque: una intención; liberar/repetir: una nueva |
| IDs y reposesión | PASS | Seis pulsaciones IDs 1–6; reposesión conserva generación 1 y siguiente ID 7, sin duplicados |
| RestartLevel | PASS | Mismo GameInstance: generación 1 → 2; primer ataque nuevo ID 1 |
| Timestamp original | PASS | Ocho timestamps positivos y monotónicos; cada evento coincide con un log nativo a tolerancia 1e-8 s |
| BeatCoreX.Core.SessionIdentity | PASS | AutomationController: Test Completed. Result={Success} |
| Hardware teclado/ratón | BLOCKED para esta automatización | No ejercitado; input inyectado a nivel de acción |
| Standalone / 30/60/120 FPS | BLOCKED para esta entrega | No ejecutados; no se extrapola el PASS de PIE |

La primera corrida terminó **FAIL del comprobador** por interpretar el texto del enum `<BCXAttackType.PUNCH: 0>`. Los eventos del personaje ya eran correctos. Se corrigió el parser y se repitió toda la corrida: **PASS**. El informe final y [log relevante](Validation/UE5.8.3/RelevantEditor.log) son la evidencia aceptada; el fallo inicial queda descrito para trazabilidad. [Informe PIE con valores originales](Validation/UE5.8.3/PIEValidation.json).

La coincidencia de timestamps se verificó contra el log C++ original, sin reemplazarlos por la hora del callback Python. No se afirma timing musical ni precisión de muestra. Binaries/Intermediate/Saved siguen ignorados por Git.

## Reproducir y revisar

Desde la raíz ejecutar `Scripts/build_editor.ps1`. Tras exit 0, abrir BeatCoreX.uproject en UE5.8.3 y Map_Prototype_Combat. Los assets ya existen: no ejecutar de nuevo el generador sobre ellos. Controles: WASD, ratón, Espacio, clic izquierdo Punch, E Kick, Q Headbutt. Los ataques solo emiten intención/log; no hay animación de ataque ni daño.

Iniciar PIE y ejecutar `Scripts/validate_prototype_pie.py` en el editor Python. Usa callbacks de frame y escribe Saved/Validation/PIEValidation.json; no bloquear el game thread con sleep. Automation → BeatCoreX.Core.SessionIdentity ejecuta el test nativo. La prueba física pendiente debe ejercer los controles y contrastar BCX_ATTACK al mantener/liberar/pulsar. Mantener la distinción con inyección de acciones en cualquier revisión.

El C++ de personaje, cámara, GameInstance e identidad permanece intacto en esta reanudación. Se descarta la configuración AndroidFileServer que el editor añadió automáticamente, fuera del alcance Windows. Sin Quartz, MetaSounds, combos, daño, IA enemiga ni multijugador. Los 13 DOCX, README y documentos de PR #1 se conservan; main permanece en dc8da7f.

PR #1 sigue en UE5.6 y no se modifica automáticamente: antes de fusionar ambas PR, revisar explícitamente versión, requisitos, comandos y tabla API para coordinar con UE5.8, preservando los DOCX históricos. PR #2 queda en borrador para revisión de build/assets/PIE automatizado, con pruebas físicas/Standalone pendientes. Sin merges automáticos.
