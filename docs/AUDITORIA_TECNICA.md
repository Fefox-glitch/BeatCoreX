# Auditoría técnica de BeatCoreX para el primer prototipo Windows

Fecha: 2026-10-11. Base auditada: `dc8da7fcda8dcce68b3aa66596ae05e76daea1e2` (main).

El repositorio es una base documental, no un proyecto Unreal ejecutable. Contiene README y 13 DOCX; no contiene `.uproject`, `Source`, `Config`, mapas, Blueprints, música ni modelos. Se propone construir primero un combate local con Kael Sync contra CPU y BPM constante. Este informe y el [plan](PLAN_IMPLEMENTACION.md) cierran la documentación de PR #1 con los contratos corregidos de tiempo, validación y capas. No constituyen implementación ni evidencia de compilación. La próxima PR #2 se limita a la base controlable de Fase A.

## Fuentes y método

Se leyeron todos los documentos extrayendo párrafos de `word/document.xml`, incluidos textos de tablas. Las referencias P indican párrafos no vacíos en orden XML, no páginas de Word. Se analizó contenido textual; no se verificó la maquetación ni objetos gráficos incrustados. Los originales permanecen intactos. No hay AGENTS.md en el checkout.

| ID | Documento original | Evidencia relevante |
| --- | --- | --- |
| D01 | [ManualTecnico_BeatCoreX_Completo_Final.docx](../ManualTecnico_BeatCoreX_Completo_Final.docx) | P3–43 clases y pseudocódigo; P44–78 convenciones |
| D02 | [Prototipo_UE5.6_Completo.docx](../Prototipo_UE5.6_Completo.docx) | P1–43; reproduce el núcleo de D01, sin el anexo |
| D03 | [Blueprints.docx](../Blueprints.docx) | P2–62 notas, hit zone, puntuación, sincronización; P63–135 diagramas textuales |
| D04 | [Pasos Blueprint en Unreal.docx](../Pasos%20Blueprint%20en%20Unreal.docx) | P1–46 pasos del sistema de notas |
| D05 | [BeatCoreX_2.docx](../BeatCoreX_2.docx) | P8–15 prototipo básico; P16–25 demo; P26–43 fases posteriores |
| D06 | [Habilidades cada personaje.docx](../Habilidades%20cada%20personaje.docx) | P2–29 habilidades; P30–47 desbloqueos y modos |
| D07 | [Personajes multijugador.docx](../Personajes%20multijugador.docx) | P1–61 seis personajes, estilos y relaciones |
| D08 | [jefes update posibles part 2.docx](../jefes%20update%20posibles%20part%202.docx) | P2–45 propuesta de seis jefes |
| D09 | [historia temas a hablar.docx](../historia%20temas%20a%20hablar.docx) | P3–23 preguntas abiertas; P24–41 Kael y mundo |
| D10 | [Pauta_Desarrollo_Unreal_5_6.docx](../Pauta_Desarrollo_Unreal_5_6.docx) | P7–38 tecnologías y sistemas; P97–112 próximos pasos y anexos anunciados |
| D11 | [BeatCoreX_MenuYConfiguracion.docx](../BeatCoreX_MenuYConfiguracion.docx) | P1–7 menús, configuración, audio, créditos |
| D12 | [aaaaa.docx](../aaaaa.docx) | P1–287 menús, XP, Vivox y audio; parcialmente duplica D11 |
| D13 | [proximos pasos.docx](../proximos%20pasos.docx) | P1–15 inventario, tienda, lobby, fases de jefe |
| D14 | [README.md](../README.md) | Concepto de combate sincronizado con música |

No hay jerarquía normativa ni versiones consistentes entre documentos. Para esta propuesta se usa D05 para ordenar las fases, D01 para convenciones, D03 para precisión/puntuación y la restricción del usuario para posponer todo multijugador. Cualquier resolución de diseño se marca como propuesta, no como requisito confirmado.

## Sistemas y alcance

| Sistema definido | Primer slice propuesto | Posterior | Fuente |
| --- | --- | --- | --- |
| Personaje 3D y movimiento libre | Kael, movimiento y salto | Resto del elenco y personalización | D02 P5–10; D05 P10; D06 P31 |
| Ritmo y música | Una pista de BPM constante, reloj único, indicador | BPM variable, compases irregulares, música dinámica, editor | D03 P55–62; D05 P15; D08; D09 |
| Combate | Ataque físico básico calificado y daño local | Secuencias Attack/Kick/Special, magia y encantamientos | D02 P11–17; D10 P30–33 |
| Precisión, score y combo | Perfect/Good/Miss, combo de aciertos y máximo | Balance, secuencias y modificadores de personaje | D03 P31–52, P107–123 |
| Enemigos | CPU básico con ataque al beat | IA avanzada y seis jefes | D05 P12; D08 |
| HUD | Salud, pulso, precisión, score y combo | Estilo final, inventario y XP | D05 P13; D03 P43–54; D13 |
| Menús/configuración | Reiniciar prueba y salir como utilidad propuesta | Menú completo, selección, créditos, persistencia de opciones | D11; D12 |
| Habilidades | Diferidas para medir combate base | Pasivas, activas, BeatOverdrive | D06 P2–29 |
| Inventario/tienda | Diferidos | Ítems, pilas, transacciones, buffs | D02 P18–31; D13 P2–9 |
| XP/desbloqueos/guardado | Diferidos | Niveles, puntos, capítulos y SaveGame | D02 P32–38; D06 P30–36; D12 P224–287 |
| Red y voz | Diferidos íntegramente, incluso co-op local | Co-op, versus, lobby, RPC, proveedor de voz | D05 P32–37; D06 P41–47; D10 P17–18 |

La obligatoriedad de esta columna es la propuesta de alcance del primer prototipo, no una modificación de los originales. Perfect/Good/Miss aparece en fase demo de D05; se adelanta como instrumento necesario para verificar los ataques sincronizados solicitados. Dos personajes y Maestro Tremor corresponden a la demo de fase 3, no al primer slice. Bloqueo y esquiva son preguntas en D09, no controles confirmados. VS player en D05 P14 no se implementa antes de validar local contra CPU.

## Personajes y jefes

| Personaje | Estilo | D06 pasiva / activa / BeatOverdrive | Desbloqueo |
| --- | --- | --- | --- |
| Kael Sync | Hip-hop/Funk | Combo Flow / Drop Beat / Ritmo Supremo (10 s) | Inicio |
| Rina Voxel | Techno/EDM | Glitch Loop / Data Pulse / Neon Cascade | Primer nodo de ritmo |
| Axel Furia | Metal/Industrial | Riff Brutal (10% crítico) / Martillo del Metal / Furia del Breakdown (10 s) | Jefe capítulo 2 |
| Selena Mar | Salsa, reggaetón, cumbia | Clave Latina (10% evasión) / Ritmo Tropical / Carnaval Explosivo | Ruta alternativa capítulo 3 |
| Dr. Bassline | Dubstep/Drum and Bass | Subgraves Eternos / Distorsión Central / Bass Apocalypse (5 beats consecutivos) | Pulso Oscuro difícil |
| Luma Vox | Pop/Soul/Gospel | Vibrato Vital / Canto Armónico / Himno de la Redención | Final capítulo 4 |

D07 denomina habilidades especiales diferentes: Rina Loop Glitch, Axel Headbang Smash, Dr. Bassline Grave Drop y Luma Nota Divina; no asumir que son alias de D06. Selena también tiene efectos distintos (bailar/vulnerabilidad frente a reducción de agresividad). Kael y sus Guantes Resonantes pueden representarse con placeholders identificados, sin inventar activos finales.

D08 propone Maestro Tremor (4/4, contratiempos, golpes y proyectiles), RAZORKODE (128 BPM, secuencias y luces), Kharon Riff (6/8 y cambios de tempo), Condesa Cadencia (5/4 y 7/8), Vektor 9 (170–180 BPM, fills) y Orfeo Silens (pulso interno sin beat audible). Sus zonas son respectivamente Distrito de los Discos Rotos, Torre del Latido Binario, Forja de Frecuencias Rojas, Sala de Ritmos Extintos, Zona Antisonora y Cúpula Muda. Todos se difieren; el CPU de prueba no recibe identidad narrativa nueva.

## Controles

Confirmados conceptualmente: MoveForward, MoveRight, Jump, AttackCombo/Attack; Attack/Kick/Special son un ejemplo de secuencia y HitKey una entrada de notas (D02 P8–17; D03 P99). No hay teclas, mando, sensibilidad, orientación ni cámara definitivos.

Propuesta de prueba: WASD movimiento relativo a cámara, ratón cámara, Espacio salto, clic izquierdo ataque, R reinicio y Escape salida/pausa de prueba. Usar `IA_BCX_Move` (Axis2D), `IA_BCX_Look` (Axis2D), `IA_BCX_Jump` y `IA_BCX_Attack` (Bool) en `IMC_BCX_Gameplay`. Capturar ataque una vez al presionar, no cada frame al mantener. Reinicio/pausa son herramientas para QA, no nuevas mecánicas. Mando y reasignación quedan pendientes.

## Incompatibilidades y defectos documentales

1. **Combate frente a notas físicas:** D02 trata combate 3D, D03 un juego de notas que viajan a un collider. No está definida su unión. Propuesta: evaluar ataque contra el beat; representar próximos beats en HUD. Posponer actores NoteSpawner/BP_Note/BP_HitZone hasta decidir si son mecánica obligatoria. No asumir que golpear una nota equivale a golpear un enemigo.
2. **Relojes:** Delay/Timer y tiempo de overlap no garantizan alineación con audio. La llegada al borde del collider no es necesariamente el centro musical del beat; el flujo ActiveNote exige overlap y dificulta aciertos anticipados. Comparar igualdad de porcentajes puede saltar beats. Usar tiempos objetivo e intervalos cruzados, no igualdad ni integración acumulativa de DeltaTime.
3. **Combo inicial:** D02 P15 inicia LastInputTime=0 y exige CurrentTime-LastInputTime <= MaxTime incluso en paso 0. Al esperar más que MaxTime el primer input falla, y ResetCombo devuelve 0. Aceptar explícitamente el inicio, luego validar intervalos; manejar secuencia vacía. Ese componente valida separación entre inputs, no fase musical. Combo rítmico y secuencia deben tener responsabilidades distintas.
4. **Exposición a Blueprint:** RegisterInput y ResetCombo carecen de UFUNCTION BlueprintCallable pese al flujo BP que los invoca (D02 P13–17). Corregir en código futuro y validar con UHT. El pseudocódigo no es un asset BP.
5. **Archivos C++:** fragmentos concatenados carecen de saltos, módulos, targets y proyecto. C++ Classes es una vista del editor, no una carpeta fuente bajo Content. Crear `.h/.cpp` en Source y mantener generated.h como último include del header.
6. **Guardado incompleto:** FInventoryItem se usa sin include visible en SaveGameData; SaveManager no tiene implementación ni esquema de migración; PostLogin tampoco tiene cuerpo mostrado. No tratar estas declaraciones como sistemas terminados.
7. **Habilidades:** LastUsedTime no tiene valor inicial explícito; el criterio de cooldown puede impedir primer uso; efectos son comentarios. Inventario no valida cantidades negativas ni límites. Diferir e incluir validación al implementarlos.
8. **Inputs duplicados:** D02 P8 y P10 procesan movimiento y salto tanto en C++ como BP. Elegir un único dueño de la entrada. Legacy BindAxis/BindAction no es por sí mismo prueba de incompatibilidad UE5.6; depende de mappings que faltan. Se recomienda Enhanced Input, no afirmar que legacy fue eliminado.
9. **Configuración:** D12 P177–179 atribuye volumen a GameUserSettings y mezcla Save Settings To Slot; después describe SoundMix/SoundClass correctamente. Separar resolución/fullscreen de audio y persistir volumen explícitamente; SaveGame y GameUserSettings no son el mismo almacenamiento.
10. **Nombres y fases:** coexistencia Maps/Levels, Network/Networking y BP_PlayerCharacter/BP_BCX_PlayerCharacter; D01 P48/P67 también difiere en nombre de mapa Boss1. Usar /Game/BeatCoreX y prefijos del anexo. D10 P101 propone red temprana, incompatible con la restricción actual y el orden de D05: se difiere.
11. **Lore:** Maestro Drop es mentor desaparecido (D09/D07), Tremor se presenta también como mentor traidor (D08). Puede coexistir, pero no se explica la relación. Axel jefe de capítulo 2 y Kharon jefe metal requieren ordenar encuentros, no suprimir uno.

## Arquitectura modular propuesta

Un módulo runtime `BeatCoreX` inicialmente, con carpetas lógicas Core, Rhythm, Player, Combat, Enemy, UI y Data en `Source/BeatCoreX`. Separar módulos de compilación solo cuando aporten independencia real. Content en `/Game/BeatCoreX/{Characters,Blueprints,UI,Maps,Data,Audio,FX}`. No crear red ni plugins propios para el slice.

| Dueño C++ propuesto | Responsabilidad | Blueprint y datos |
| --- | --- | --- |
| UBCXMusicClockSubsystem (WorldSubsystem) | Adaptador Quartz, transporte, tiempo musical continuo, loop, reset, eventos con índice | Configuración musical, sin reloj alternativo en Level BP |
| FBCXCalibrationSnapshot | Mapeo monotónico/musical inmutable por evaluación; CalibrationAge valida vigencia, InputDeltaSeconds usa entrada original | Estado técnico independiente del juicio y audio |
| UBCXMusicLayerManager | Único registro por sesión/capa, ciclo Requested/Scheduled/Active/Expiring, renovación y limpieza | Ready propio de MetaSound no activa ni califica; contrato, diferido fuera de Fase A |
| FBCXTimingEvaluator / FBCXHitResult | Función pura de clasificación, BeatId, error firmado, calidad | Ventanas editables; no escribe HUD ni daño |
| ABCXPlayerCharacter / ABCXPlayerController | Movimiento, cámara e input único | BP_BCX_PlayerCharacter: mesh, SpringArm, Camera, AnimBP |
| UBCXCombatComponent | Validación antes de commit; deduplicación SessionGeneration+AttackId; consumo de beat, estado y trace | AnimMontage y feedback, parámetros en DataAsset |
| UBCXComboComponent | Combo de precisión, max y score; futura secuencia separada | Dispatchers para HUD; sin Tick para puntuar |
| UBCXHealthComponent | Salud, daño validado, muerte y reset | Eventos visuales jugador/enemigo |
| ABCXEnemyCharacter / ABCXEnemyAIController | Acercarse, anunciar, atacar en beat programado, recuperar, morir | BP_BCX_EnemyBasic y parámetros; FSM inicial, Behavior Tree después si se necesita |
| ABCXPrototypeGameMode | Spawn, victoria/derrota, reiniciar sesión | BP_BCX_PrototypeGameMode y Map_Prototype_Combat |
| UBCXPrototypeHUDWidget | Suscripción a estado, actualización visual | Widget_BCX_HUD; no calcula calificación |
| UBCXSongDefinition / UBCXCombatDefinition | Audio, BPM, compás, offset, loop; ventanas y daño | DataAssets versionados, validados antes de jugar |

Flujo: captura original -> validación de sesión/BPM/calibración/timestamp -> calificación -> commit deduplicado y reserva -> ataque/impacto -> salud; resultado de ritmo -> score/combo -> HUD. Rechazo técnico no se registra como procesado ni Miss. Enemigo lee el mismo transporte y agenda índices futuros. UI/animación consumen resultados sin recalcularlos. Exponer solicitudes con BlueprintCallable, resultados con BlueprintReadOnly/dispatchers y parámetros con EditDefaultsOnly. Nunca aplicar daño en C++ y BP simultáneamente. El [contrato C++ y ciclo de sesión](PLAN_IMPLEMENTACION.md#contrato-c-propuesto) del plan es la referencia única de comportamiento; las capas no añaden habilidades al alcance.

## Dependencias y entorno

Requeridas propuestas: UE 5.6 fijado a una versión concreta, toolchain C++ Windows compatible, Core/CoreUObject/Engine/InputCore, EnhancedInput y UMG; AudioMixer según uso de Quartz. Slate/SlateCore solo si el código de UI los utiliza; AIModule/NavigationSystem solo si hay AIController/navmesh. Confirmar nombres/includes contra la instalación 5.6 antes de Build.cs. No se necesita GAS para un ataque y salud básicos.

Se consultó [Enhanced Input 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine?application_version=5.6): acciones y contextos son assets y ofrecen transición desde input legacy. [Configuración de Visual Studio 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.6) indica VS2022 17.8 mínimo y 17.14 recomendado. Verificar también MSVC/SDK exactos en esa tabla y en UBT; no adoptar por accidente recomendaciones de UE5.8.

### APIs contrastadas de UE5.6

Consulta documental realizada con `application_version=5.6`. Las páginas C++ Quartz intentadas no fueron accesibles mediante la herramienta; se verificaron métodos reflejados en la referencia Python oficial 5.6, que identifica sus clases/módulos/headers C++. No trasladar firmas Python (tuplas/out parameters) literalmente a C++; los overloads exactos de Quartz quedan por cotejar con headers instalados en PR #2 o al implementar su adaptador. No se sustituye esta evidencia por documentación 5.8.

| Clase y módulo | API concreta verificada y uso | Evidencia 5.6 |
| --- | --- | --- |
| UQuartzSubsystem, AudioMixer, QuartzSubsystem.h | CreateNewClock(context, name, settings, override, useAudioEngineClockManager); DeleteClockByHandle/Name; reloj de audio y nombre único por generación | [Referencia reflejada](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/QuartzSubsystem?application_version=5.6) |
| UQuartzClockHandle, AudioMixer, AudioMixerClockHandle.h | GetCurrentTimestamp(context), GetEstimatedRunTime(context); StopClock(context, cancelPendingEvents), UnsubscribeFromAllTimeDivisions; StartClock y SetBeatsPerMinute con frontera/delegate | [Referencia reflejada](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/QuartzClockHandle?application_version=5.6) |
| UAudioComponent, Engine, AudioComponent.h | PlayQuantized(context, handle, boundary, delegate, startTime, fadeInDuration, fadeVolumeLevel, fadeCurve); Stop/FadeOut para limpieza de instancia | [Referencia reflejada](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AudioComponent?application_version=5.6) |
| EQuartzCommandDelegateSubType, Engine, QuartzQuantizationUtilities.h | COMMAND_ON_QUEUED, COMMAND_ON_STARTED, COMMAND_ON_ABOUT_TO_START, COMMAND_ON_CANCELED, COMMAND_ON_FAILED_TO_QUEUE | [Enum 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/QuartzCommandDelegateSubType?application_version=5.6) |
| FQuartzTransportTimeStamp, Engine, QuartzQuantizationUtilities.h | Bars/Beat/BeatFraction/Seconds: snapshot de transporte; no timestamp del input | [Struct 5.6](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/QuartzTransportTimeStamp?application_version=5.6) |
| UMetaSoundOutputSubsystem, plugin Metasound, módulo MetasoundEngine, MetasoundOutputSubsystem.h | WatchOutput y UnwatchOutput; observación de output de Source reproduciéndose, no preparación previa | [Referencia reflejada](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MetaSoundOutputSubsystem?application_version=5.6) y [WatchOutput C++ 5.6](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/MetasoundEngine/UMetaSoundOutputSubsystem/WatchOutput?application_version=5.6) |

Firma C++ documental comprobada: `bool WatchOutput(UAudioComponent*, FName, const FOnMetasoundOutputValueChanged&, FName, FName)`, también overload nativo; include `MetasoundOutputSubsystem.h`. Su bool indica registro exitoso. `Ready` es un output que BeatCoreX tendría que definir en el asset, no un método ni una garantía de audio audible. MetasoundEngine solo se agrega al usar ese contrato en implementación posterior.

GetEstimatedRunTime advierte que su estimación tiene latencia; no usarlo como tiempo exacto del input. El handle es wrapper de game thread: conservar timestamp original y mapping calibrado, sin equiparar llegada del delegate con beat audible. La precisión de audio no garantiza input/gameplay a nivel de muestra. OnAudioPlaybackPercent queda como diagnóstico, no garantía bajo lag. Cancelación, timeouts, callbacks y limpieza de componentes requieren pruebas de integración en 5.6; aquí no están demostrados.

Faltan pista con licencia, BPM/downbeat/loop verificables, modelo y animaciones compatibles, hitboxes, datos de daño y assets UI. Mixamo/Blender/DAW/MetaSounds son herramientas citadas en D10, no assets ni dependencias instaladas. Vivox/Photon/Steam son alternativas futuras sin proveedor elegido; no instalar ni incorporar credenciales. Git LFS para futuros uasset/umap/audio debe definirse antes de añadir binarios.

En este equipo no se encontró UE_5.6 en la ruta habitual de Epic Games ni un registro de builds en HKCU consultado; el manifiesto `C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat` devuelve InstallationList vacía. No se localizó vswhere.exe ni SDK en las rutas habituales de Visual Studio Installer/Windows Kits. Esto no demuestra ausencia en otras unidades. Confirmar ruta de engine y VS/MSVC/SDK compatible es bloqueo real para compilar/validar Fase A. Git LFS 3.7.1 está disponible; push HTTPS a la rama funciona. No se realizó compilación, PIE ni empaquetado; no hay proyecto que compilar.

## Riesgos e información pendiente

| Riesgo / falta | Impacto | Acción y puerta de validación |
| --- | --- | --- |
| Semántica de fallo: cancelar golpe o reducir daño | Define la sensación de combate | Revisar propuesta del plan antes de balance |
| Ventanas, BPM, daño, alcance, recuperación sin cifras | Balance no reproducible | DataAssets con valores de ensayo etiquetados, registro en cada prueba |
| Latencia de audio/input/display y callbacks | Calificación injusta | Medición independiente en Windows; compensación de ensayo; comparar 30/60/120 FPS |
| Loops, reinicio, pausa y pérdida de foco | Drift y beats duplicados | Transporte único y pruebas de ciclo de vida |
| Animación separada del instante musical | Impacto visual/audible incongruente | Fijar política de input versus impacto; medir notifies y hitches |
| Hitbox, dirección de ataque y cámara pendientes | Combate ilegible | Graybox en tercera persona, revisión de alcance y colisiones |
| Sin assets ni licencia de pista | No existe ejecutable representativo | Usar material de prueba autorizado y documentar origen |
| Ambigüedad notas/combate y habilidades | Retrabajo | Revisar decisiones abiertas antes de extender el slice |
| Toolchain y hardware objetivo no inventariados | No se puede garantizar build ni rendimiento | PR base con build real y manifiesto del entorno |
| Documentos duplicados y sin precedencia | Deriva de requisitos | Matriz de fuentes, decisiones versionadas en PR |

El plan fija las políticas de ensayo de Miss/combo, cámara, notas en HUD y pasiva diferida; ya no son contratos contradictorios ni bloqueos para iniciar Fase A. Su balance/dirección final, pista con licencia, calibración y sincronía física siguen por validar en implementación. Mando, bloqueo/esquiva, dificultad y lore final quedan fuera del alcance inmediato. No exigir esos elementos para construir la base controlable. Los anexos UML de D10 están anunciados, no desarrollados en el texto extraído.

El primer entregable implementado se acepta solo con evidencia de compilación UE5.6, ejecución local y paquete Windows. Una revisión documental o una compilación C++ aislada no valida el prototipo jugable.
