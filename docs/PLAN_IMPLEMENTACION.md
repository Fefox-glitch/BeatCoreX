# Plan verificable del vertical slice de BeatCoreX

Estado: propuesta para revisión, 2026-10-11. Véase [auditoría y fuentes](AUDITORIA_TECNICA.md). No se implementa código de juego en esta primera PR.

## Experiencia objetivo

Abrir un ejecutable Windows, controlar a Kael en una arena de prueba, escuchar una pista constante, anticipar el beat en HUD y atacar a un CPU. Ver salud, calificación, puntuación y combo; derrotar al CPU o perder y reiniciar la prueba. Un personaje, una arena y un enemigo; arte provisional identificado. El menú completo, Rina, Tremor, campaña, inventario, habilidades y todo multijugador quedan fuera de esta entrega.

Propuesta de graybox: suelo plano, límites con colisión, PlayerStart, spawn de enemigo, iluminación legible y navmesh si se usa navegación. Cámara tercera persona con SpringArm y colisión, movimiento relativo a cámara y ataque hacia frente. Map_Prototype_Combat bajo /Game/BeatCoreX/Maps es un mapa de QA, no reemplaza Map_01_Intro ni establece un capítulo nuevo.

## Decisiones de ensayo que requieren revisión de diseño

| ID | Propuesta inicial | Motivo / criterio de revisión |
| --- | --- | --- |
| DEC01 | Ataques contra beat global; notas en HUD como guía | Unir combate/ritmo sin imponer notas físicas no integradas |
| DEC02 | 120 BPM, 4/4; loop de 8 compases con downbeat en cero | Pista de ensayo, no tema final de Kael; 60/BPM = 0.5 s |
| DEC03 | Perfect <= 50 ms; Good <= 100 ms; Miss fuera | Valores de laboratorio, ajustar tras medir latencia |
| DEC04 | Miss no aplica daño y rompe combo; Perfect/Good aplican 10 de daño | Política mínima propuesta; score 300/100 sí proviene de D03 |
| DEC05 | Salud jugador/CPU 100; CPU daño 10, anuncio un beat antes, ataque cada 4 beats | Valores provisionales para ciclo ganable y reproducible |
| DEC06 | Un ataque aceptado por beat; no repetir score/daño manteniendo tecla | Evitar spam; input extra en beat consumido se ignora sin score ni reset |
| DEC07 | Combo cuenta precisión aceptada incluso sin objetivo; daño requiere alcance | Separar prueba de ritmo de geometría; no confirmado por documentos |
| DEC08 | Omitir pasiva Combo Flow y todas las activas en la primera medición | Aislar daño base; implementar identidad de Kael después de validación |
| DEC09 | Sin pausa parcial: detener transporte y gameplay juntos; reinicio desde cero al reanudar la prueba si el adaptador no garantiza continuidad | Evitar seguir audio con tiempo de juego congelado; política de QA revisable |

Las decisiones son una configuración de ensayo explícita. No hay evidencia de aprobación del diseño ni cifras canónicas; si se rechazan, se actualizan datos y criterios antes de implementar el sistema afectado. No añadir bloqueo/esquiva ni magia para resolver balance por inferencia.

## Contrato musical y combate

SongDefinition: referencia de audio, BPM positivo, numerator/denominator, downbeat offset en segundos, duración de loop conocida y número entero de beats para este slice. CombatDefinition: ventanas con 0 <= Perfect <= Good < intervalo/2, daño, alcance, recuperación y salud inicial. Propuesta de alcance 150 cm y recuperación de un beat, también valores de ensayo. Validación rechaza BPM cero, ventanas solapadas, audio ausente o loop no alineado. Una pista distinta obliga a medir de nuevo sus offsets.

Tiempo musical continuo `t` desde inicio programado en audio; no se reconstruye sumando DeltaTime. Para este slice, objetivo `T(n)=offset+n*(60/BPM)`, `n>=0`. Timestamp del input convertido al dominio musical y compensación firmada `c`: `tEvaluado=tInput-c`; `error=tEvaluado-T(n)`. c positivo descuenta retraso medido, no se elige para inflar resultados. La captura en game thread tiene resolución limitada por el frame; registrarla como tal.

Elegir beat más cercano y no consumido dentro de la ventana, con desempate determinista hacia el anterior. Guardar BeatId global y generación de sesión; el índice no vuelve a cero al hacer loop de audio. Una pulsación aceptada consume ese beat una vez. Expirar beats opcionales del HUD no rompe el combo automáticamente: el slice evalúa ataques voluntarios, no obliga a atacar cada beat. Si se confirma una pista de notas obligatorias, esta regla cambia explícitamente.

Quartz agenda inicio de pista en una frontera conocida; el adaptador debe comprobar en UE5.6 cómo obtener y convertir el tiempo de transporte, contemplando latencia de su proxy de game thread. No puntuar por hora de recepción de un callback ni afirmar precisión de muestra para input. Eventos de beat arrastran índice y tiempo objetivo. Ante hitch, recorrer índices cruzados para mantener estado, descartar ataques enemigos ya vencidos en lugar de descargar varios daños en un frame. Audio no se reinicia por cada Tick. El HUD interpola fase desde snapshot; no mantiene otro cronómetro.

Ataque: entrada única -> clasificación -> comprobación de estado/beat -> consumo -> animación y ventana de impacto -> trace barrido de alcance -> salud. Propuesta: calificar al input y aplicar daño en ventana de AnimNotify durante el mismo ataque; no recalificar al notify. Si el montaje no permite una ventana útil antes del siguiente beat, ajustar el montaje o programar la ventana explícitamente; no disimular un retraso medido. AttackId y conjunto de actores impactados impiden daño doble. Muerte cancela ataque pendiente. Good/Perfect comparten daño base en esta prueba; score difiere. Reinicio limpia score, salud, beats consumidos, ataques, delegates y audio.

CPU: Idle -> Approach -> Telegraph -> Attack -> Recovery -> Approach; Dead terminal. En alcance, anunciar un ataque futuro del patrón de cuatro beats y ejecutar en índice programado; fuera de alcance, acercarse sin daño remoto. Sin esquiva añadida, el jugador puede salir del alcance caminando. Estados y aviso visual son implementación de prueba, no lore. El primer ataque y recuperación se definen en beats. Usar el mismo HealthComponent y transporte del jugador.

## Secuencia de PR y puertas

Cada PR debe ser pequeña, revisable, sin modificar DOCX originales y sin mezclarse con red. No fusionar automáticamente. Dependencias son secuenciales; las pruebas se repiten solo por cambios relevantes.

| PR | Trabajo y entregables | Dependencia | Aceptación verificable |
| --- | --- | --- | --- |
| PR0 actual | Auditoría, matriz, arquitectura, este plan | Base documental | Originales idénticos a base, enlaces/fuentes revisados, decisiones visibles |
| PR1 base UE5.6 Windows | .uproject, runtime y targets Editor/Game, Config, gitignore, política LFS, instrucciones build, mapa mínimo y personaje/cámara/input | Revisión PR0 y toolchain disponible | Build Editor Win64 real sin Live Coding; abrir mapa; caminar, saltar y cámara sin doble input; BPs sin errores |
| PR2 transporte y precisión | Adaptador Quartz, SongDefinition, función pura de clasificación, tests temporales y log diagnóstico | PR1 + pista autorizada | Inicio y loops medidos; fronteras y reset probados; un único reloj |
| PR3 combate local | Componentes combat/health/combo, montaje/trace, score y datos de ensayo | PR2 + revisión DEC03/04/06/07/08 | Perfect/Good/Miss y daño comprobados; mantener tecla no duplica; ataque fuera de alcance no daña |
| PR4 CPU y HUD | Enemigo, FSM/patrón, arena completa, HUD, victoria/derrota/reinicio | PR3 | Partida completa contra CPU; ataques con aviso y beat; muerte detiene daño; 10 reinicios limpios |
| PR5 paquete Windows | Cook/stage/package Development Win64, instrucciones, manifiesto de pruebas y lista de limitaciones | PR4 | Ejecutable lanzado fuera del editor, ciclo completo, logs archivados, fallo de build bloquea aceptación |
| Posterior | Rina/Tremor, habilidades y combos avanzados, menú final, progresión y campaña | Aceptación PR5 y revisión de prioridades | Criterios específicos en futuras PR; red solo tras validar combate local |

No se asignan fechas ficticias: duración depende de toolchain, assets y calibración. PR1 desbloquea capacidad real de construir; PR2 es el principal riesgo técnico. Si falla sincronización, no extender contenido para ocultarlo.

## Matriz de pruebas

| ID | Procedimiento | Resultado exigido |
| --- | --- | --- |
| T01 | Evaluador con error 0, ±50 ms, ±50 ms + epsilon, ±100 ms, ±100 ms + epsilon | Perfect/Good/Miss según límites inclusivos, en ambos sentidos |
| T02 | Entradas sintéticas a T(n), antes del inicio y a mitad entre beats; BPM cero y ventanas inválidas | BeatId/empate deterministas, no juzgar beats negativos, rechazar configuración inválida |
| T03 | Dos inputs aceptables mismo beat y tecla mantenida por 2 s | Solo un resultado puntuable y un impacto por AttackId; sin Tick input repetido |
| T04 | Dejar pasar 10 s y hacer primer ataque de secuencia; reset y repetir | Inicio aceptable sin depender de LastInputTime=0; secuencia vacía segura |
| T05 | 10 minutos de pista, registrar inicio/loop y al menos 20 transiciones de loop | Índice continuo, ningún beat duplicado; error de alineación p95 <= 20 ms y máximo <= 40 ms como objetivo propuesto de laboratorio |
| T06 | Repetir entrada sintética equivalente a 30/60/120 FPS y un hitch de 200 ms | Misma clasificación para timestamps idénticos; informar diferencia de input humano por captura de frame; sin ráfaga de daño tras hitch |
| T07 | Golpear en/fuera del alcance, sobreposición sostenida y actor muerto | Daño una vez, cero fuera de rango o tras muerte; score/combos conforme DEC07 |
| T08 | CPU en alcance y fuera, 20 ataques programados | Aviso previo, daño local en ventana y recuperación; registrar error respecto a beat, objetivo <= un frame + 20 ms |
| T09 | Victoria, derrota, pausa/foco y 10 reinicios | Salud/score/transport limpios, sin delegates/audio duplicados ni ataques viejos |
| T10 | Ejecutable Development Win64 en Windows fuera del editor | Audio, inputs, HUD, CPU y reinicio funcionan; registrar hardware, dispositivo de audio, FPS, engine, MSVC, SDK y commit |

T05 requiere observación independiente del audio: grabación loopback o herramienta equivalente autorizada, comparación de transientes con referencia de pista y timestamps. Un log de reloj que se compara consigo mismo no prueba sincronía audible. Latencia constante y deriva son medidas distintas; registrar ambas y la compensación usada. T06 no puede prometer iguales entradas físicas a distinto FPS; distingue prueba matemática de prueba humana. Los umbrales son objetivos propuestos, no resultados alcanzados.

## Compilación y evidencia futura

Comandos de ejemplo para después de PR1, no ejecutados. Ajustar EngineRoot a instalación real y confirmar nombres de targets; trabajar desde la raíz del checkout.

```powershell
$engineRoot = 'C:\Program Files\Epic Games\UE_5.6'
$projectFile = Join-Path (Get-Location) 'BeatCoreX.uproject'
& "$engineRoot\Engine\Build\BatchFiles\Build.bat" BeatCoreXEditor Win64 Development "-Project=$projectFile" -WaitMutex
# Solo con build exit code 0: abrir editor, compilar BPs y ejecutar pruebas.
& "$engineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$(Join-Path (Get-Location) 'Artifacts\Windows')"
```

En cada PR de implementación guardar comandos, códigos de salida, logs relevantes y pruebas pendientes. Pruebas del evaluador bajo Unreal Automation; test funcional en mapa para vida/ataque/reinicio. Captura de partida y manifiesto para QA manual. No añadir un workflow que declare éxito sin tener runner UE5.6 y assets disponibles. No subir Binaries/Intermediate/Saved ni ejecutables al Git de código; definir ubicación de entrega revisable para el paquete cuando exista.

## Validación de esta entrega

Esta PR solo incorpora Markdown de planificación. Verificar con `git diff --name-status <base>...HEAD`, `git diff --check` y hashes de blobs Git de los 13 DOCX frente al commit base. El conjunto de cambios debe ser exclusivamente archivos nuevos de docs; no existen resultados de T01–T10 todavía. El informe no autoriza considerar el proyecto compilado ni jugable.
