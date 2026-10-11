# Plan verificable del vertical slice de BeatCoreX

Estado: cierre documental de PR #1, 2026-10-11. Véase [auditoría y fuentes](AUDITORIA_TECNICA.md). Los contratos siguientes incorporan las correcciones solicitadas; los valores de ensayo siguen identificados como propuestas. No se implementa código de juego en esta PR. Fase A significa base UE5.6 Windows y corresponde a la próxima PR #2, no a una segunda PR documental.

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
| DEC09 | Pausa/foco detienen e invalidan la sesión; reanudar reinicia desde cero con nueva generación | Política única del prototipo; no preservar capas ni comandos durante pausa |

Las decisiones son una configuración de ensayo explícita. No hay evidencia de aprobación del diseño ni cifras canónicas; si se rechazan, se actualizan datos y criterios antes de implementar el sistema afectado. No añadir bloqueo/esquiva ni magia para resolver balance por inferencia.

## Contrato musical y combate

SongDefinition: referencia de audio, BPM finito y positivo, numerator/denominator, downbeat offset en segundos, duración de loop conocida y número entero de beats para este slice. CombatDefinition: ventanas configurables con 0 < Perfect < Good < intervalo/2, daño, alcance, recuperación y salud inicial. Valores iniciales: Perfect = 0.050 s y Good = 0.100 s, simétricos e inclusivos. Propuesta de alcance 150 cm y recuperación de un beat. Validación rechaza NaN/Inf, BPM cero, ventanas inválidas, audio ausente o loop no alineado. Una pista distinta obliga a medir de nuevo sus offsets. No reducir ventanas silenciosamente si un BPM alto vuelve inválida la configuración.

Tiempo musical continuo desde inicio programado en audio; no se reconstruye sumando DeltaTime. Para este slice, objetivo `T(n)=offset+n*(60/BPM)`, `n>=0`. Se captura una sola vez `InputTimestampSeconds` en reloj monotónico al recibir la entrada (resolución de game thread explícita), junto con SessionGeneration. No reemplazarlo por la hora de procesar, un callback Quartz ni el tiempo actual del mundo.

Una calibración inmutable contiene `ReferenceTimestampSeconds`, `ReferenceMusicSeconds`, `CompensationSeconds`, `SessionGeneration`, `CalibrationRevision` y perfil de dispositivo/pista. `CalibrationAge = NowSeconds - ReferenceTimestampSeconds` valida vigencia; `InputDeltaSeconds = InputTimestampSeconds - ReferenceTimestampSeconds` convierte la entrada original al dominio musical. `EvaluatedMusicSeconds = ReferenceMusicSeconds + InputDeltaSeconds - CompensationSeconds`; `SignedBeatErrorSeconds = EvaluatedMusicSeconds - T(n)`. CalibrationAge nunca sustituye InputDeltaSeconds. Compensación positiva descuenta retraso medido. Calibración de ejemplo: referencia monotónica 100 s, musical 0 s; entrada 100.500 s, procesada en 100.700 s -> InputDeltaSeconds 0.500 s, CalibrationAge 0.700 s; con compensación cero, beat 1 Perfect, no Miss.

Parámetros técnicos configurables para ensayo: `MaxCalibrationAgeSeconds=1.0`, `MaxInputQueueAgeSeconds=0.25`, `MaxCalibrationExtrapolationSeconds=1.0`; no son ventanas de Perfect/Good. Validar finitud, edad no negativa, entrada entre comienzo de sesión y Now (sin tolerancia futura por defecto), antigüedad de cola y distancia absoluta a referencia dentro del límite. Una referencia posterior a la entrada puede producir InputDeltaSeconds negativo legítimo; no confundirlo con timestamp inválido. La calibración se refresca sin cambiar generación, fijando una revisión para cada evaluación. Un mapeo recién refrescado no debe introducir saltos; medir su error independientemente del audio. La latencia de consulta Quartz limita esta calibración, no se promete precisión de muestra.

Elegir beat más cercano con desempate determinista hacia el anterior, sin desplazar una entrada al siguiente beat porque el más cercano esté consumido. Evaluar Perfect si |error| <= PerfectWindowSeconds, Good si <= GoodWindowSeconds, Miss en el resto del dominio musical válido. No hay beat anterior a n=0. Guardar BeatId global y generación; el índice no vuelve a cero al loop de audio. Perfect/Good aceptados consumen ese beat una vez; Miss queda procesado pero no consume un beat. Expirar beats opcionales del HUD no rompe combo: no es una pista de notas obligatorias.

### Validación y registro de ataques

Deduplicación definitiva por `(SessionGeneration, AttackId)`, separada de beats consumidos. AttackId uint64 asignado por el dueño de input, positivo y monotónico dentro de la sesión; no reciclarlo. Un mismo ID con payload distinto es IdentityConflict, nunca otro ataque. El procesamiento y commit ocurren serializados en game thread; evitar reentrada de callbacks durante el commit.

Orden obligatorio: comprobar identidad y generación actual -> devolver resultado previo si ya procesado y payload idéntico -> validar transporte Running, BPM/ventanas, calibración/revisión/perfil y timestamp -> calificar usando entrada original -> comprobar estado de combate y beat consumido -> registrar resultado y reservar beat cuando proceda, en un único commit -> publicar score/combo/animación/daño. Revalidar generación antes del commit. No insertar en ProcessedAttacks antes de esas validaciones.

| Resultado | Registro definitivo | Efecto y política de reintento |
| --- | --- | --- |
| Perfect/Good válido | Sí; reserva BeatId | Score/ataque una vez; duplicado devuelve resultado almacenado sin efectos |
| Miss musical válido | Sí; sin reserva BeatId | Cero daño y reset de combo una vez; duplicado no repite reset |
| InvalidBPM/InvalidWindows, CalibrationUnavailable/Invalid/Stale, InvalidTimestamp/StaleInput, WrongSession, TransportNotRunning | No | Rechazo técnico sin juicio, daño, score, combo ni capas; log con causa, contador limitado; no almacenar como Miss |
| Busy, Dead, BeatAlreadyConsumed | No | Rechazo de gameplay sin juicio definitivo ni efectos; no cola automática |
| IdentityConflict | No nuevo registro | Conservar original; descartar payload conflictivo y registrar diagnóstico |

Eventos rechazados se descartan, no se reintentan automáticamente. El llamador puede reenviar el mismo payload/ID si desaparece una causa transitoria y aún cumple los límites temporales; nunca refrescar su timestamp para hacerlo válido. Un cambio de intención exige nueva entrada y nuevo AttackId. WrongSession o timestamp malformado no se reparan reinterpretando el evento en otra sesión. Una indisponibilidad de audio antes del ataque es AudioUnavailable/TransportNotRunning y rechaza sin registrar; fallo de una capa opcional después del commit no revierte ni duplica un ataque ya juzgado.

Quartz agenda inicio de pista en una frontera conocida; el adaptador debe comprobar en UE5.6 cómo obtener y convertir el tiempo de transporte, contemplando latencia de su proxy de game thread. No puntuar por hora de recepción de un callback ni afirmar precisión de muestra para input. Eventos de beat arrastran índice y tiempo objetivo. Ante hitch, recorrer índices cruzados para mantener estado, descartar ataques enemigos ya vencidos en lugar de descargar varios daños en un frame. Audio no se reinicia por cada Tick. El HUD interpola fase desde snapshot; no mantiene otro cronómetro.

Ataque: validación -> clasificación sobre timestamp original -> commit -> animación y ventana de impacto -> trace -> salud. Propuesta: daño en AnimNotify durante el mismo ataque; no recalificar al notify. Si el montaje no permite una ventana útil antes del siguiente beat, ajustar el montaje o programar la ventana explícitamente. `(SessionGeneration, AttackId)` y conjunto de actores impactados impiden daño doble, también en notifies tardíos. Muerte cancela impacto pendiente. Good/Perfect comparten daño base; score difiere.

CPU: Idle -> Approach -> Telegraph -> Attack -> Recovery -> Approach; Dead terminal. En alcance, anunciar un ataque futuro del patrón de cuatro beats y ejecutar en índice programado; fuera de alcance, acercarse sin daño remoto. Sin esquiva añadida, el jugador puede salir del alcance caminando. Estados y aviso visual son implementación de prueba, no lore. El primer ataque y recuperación se definen en beats. Usar el mismo HealthComponent y transporte del jugador.

### Capas musicales y MetaSounds

Contrato solicitado para gestión de capas, sin añadir habilidades ni exigir música dinámica en Fase A. `LayerKey=(SessionGeneration, LayerId)` identifica una única instancia con Component, CommandToken y LayerRevision. Duración expresada en beats. El ciclo normal es `None -> Requested -> Scheduled -> Active -> Expiring -> None`; fallos/cancelaciones salen de Requested/Scheduled hacia Expiring o None con limpieza. No saltar estados por una segunda solicitud.

| Estado | Transición válida | Solicitud repetida |
| --- | --- | --- |
| None | Validar recurso/sesión/duración y fijar StartBeat objetivo; crear único registro -> Requested | Una sola instancia por LayerKey |
| Requested | Emitir PlayQuantized una vez; COMMAND_ON_QUEUED correspondiente -> Scheduled | Fusionar duración; no componente ni comando adicionales |
| Scheduled | Solo COMMAND_ON_STARTED del token/revisión/generación vigentes -> Active | Renovar ExpiryBeat; permanecer Scheduled; QUEUED/ABOUT_TO_START no activan |
| Active | Alcanzar ExpiryBeat -> Expiring | Extender ExpiryBeat in situ; sin reiniciar audio ni emitir PlayQuantized |
| Expiring | Parada/fin y limpieza -> None | Rechazar LayerExpiring; sin resurrección ni nueva reproducción hasta None |

Reserva del CommandToken y bandera CommandIssued ocurre antes de llamar a Quartz para soportar callbacks reentrantes. Callbacks duplicados son idempotentes; STARTED sin QUEUED observado confirma que el comando fue ejecutado: el adaptador reconcilia recorriendo Scheduled antes de Active con esa evidencia, sin inventar un callback QUEUED. Un evento inválido, cancelado o de otra revisión no promueve estado. Si no llega confirmación antes del timeout configurable de ensayo (2 s), invalidar token, entrar en limpieza y registrar AudioScheduleTimeout; ningún estado Scheduled se queda eterno. No intentar cancelar un comando individual con una API supuesta: para fallo de scheduling no recuperable se invalida la sesión de audio completa.

Al renovar: `ExpiryBeat=max(ExpiryBeat, StartBeat+DurationBeats)` en Requested/Scheduled; en Active `max(ExpiryBeat, CurrentBeat+DurationBeats)`. StartBeat es el beat objetivo de inicio, no la hora de callback. Si el procesamiento ya alcanzó ExpiryBeat, pasar primero a Expiring y rechazar renovación. Validar duración finita positiva y generación. No programar anticipadamente un comando Quartz de fin que deba cancelarse/recrearse en cada renovación: el gestor comprueba expiración musical y emite una única parada/fade al cruzarla. Esa parada está limitada por game thread y se mide; no se afirma finalización a precisión de muestra. Cada renovación mantiene Component y CommandToken, y únicamente actualiza la duración.

En Expiring ejecutar Stop o FadeOut una sola vez, quitar watchers/delegates, y retirar registro/componente al confirmar fin; timeout de limpieza configurado 2 s fuerza Stop/DestroyComponent y retira el registro. Tras retirada, el callback antiguo no puede tocar una nueva instancia de LayerId por LayerRevision distinta. La duración del fade se define como parámetro visual/audio, no extiende autorización para daño o beneficios. No añadir componentes indefinidamente.

`MetaSoundReady` es estado de preparación/observación del grafo, independiente de `TimingJudgement`. `Ready` no es una API nativa ni prueba de que el audio llegó al dispositivo: es un Bool/output propio del asset si se usa. La validación previa del asset no espera un output de una Source que aún no reproduce. WatchOutput observa un generador reproduciéndose; el ack Ready posterior no activa una capa Scheduled ni cambia Perfect/Good/Miss. `WatchOutput=true` confirma registro del watcher, no Ready ni reproducción audible. El primer combate posterior a Fase A usa pista base de prueba sin depender de capas opcionales; PR #2 todavía no implementa audio.

Estados técnicos separados: CalibrationStatus (Unavailable/Valid/Invalid/Stale), AudioStatus (Unavailable/Prepared/Scheduled/Playing/Failed) y MetaSoundReady (Unknown/Ready/NotReady). Fallo de calibración invalida juicio hasta recuperar mapeo, aunque audio continúe. FailedToQueue, timeout, Source ausente o parada inesperada son fallos de audio, no CalibrationStale ni Miss. AudioStatus Playing requiere confirmación propia, no Ready. Diagnóstico de silencio/volumen/dispositivo requiere medición audible, no deducción del callback.

### Pausa, reinicio y cambios de sesión

Pausa explícita o pérdida de foco: cerrar aceptación de input y detener combate/audio; invalidar la sesión. Reanudar crea una nueva generación, recalibra y reinicia desde cero. No usar únicamente pausa del World mientras Quartz/audio siguen. Esta política sustituye cualquier continuidad condicional anterior.

Reinicio, cambio de mapa/pista y cambio BPM permitido desde configuración: incrementar generación antes de limpiar; marcar todos los CommandToken antiguos inválidos; cancelar eventos pendientes con StopClock(CancelPendingEvents=true), detener cada UAudioComponent incluso si estaba Scheduled, retirar watchers/suscripciones y eliminar reloj antiguo. Crear reloj con nombre único por generación tras limpieza. Descartar callbacks/metronome/Ready/notifies antiguos por generación+token+revisión, sin estados ni efectos. Una invalidación lógica sola no silencia audio encolado: comprobar cancelación física y parada en UE5.6, manteniendo referencias de limpieza hasta terminar. Si no se puede asegurar que no arranque audio viejo, no habilitar nueva sesión y reportar fallo técnico de audio.

BPM permanece fijo durante Running; no se acepta cambio en caliente. BPM/configuración inválidos se rechazan sin mutar la sesión actual. Para nuevo BPM válido, usar el reinicio anterior, validar ventanas y seleccionar pista alineada con ese BPM; SetBeatsPerMinute cambia el reloj, no hace time-stretch de una grabación. No implementar mapas de tempo ni conservar entradas pendientes. El loop de pista no cambia generación; un reinicio sí. Deinitialize ejecuta la misma limpieza. Limpiar ProcessedAttacks solo al invalidar su generación, nunca para permitir repetir efectos en una sesión viva.

### Contrato C++ propuesto

Tipos y métodos propios, aún sin headers ni compilación; no son firmas atribuidas a Epic. Doubles monotónicos en segundos, beats int64 y IDs/generación uint64; uso Blueprint requerirá reflejar explícitamente tipos compatibles y validar conversiones en UHT.

```cpp
struct FBCXAttackInput {
    uint64 SessionGeneration;
    uint64 AttackId;
    double InputTimestampSeconds; // Inmutable desde captura.
};
struct FBCXCalibrationSnapshot {
    uint64 SessionGeneration;
    uint64 CalibrationRevision;
    double ReferenceTimestampSeconds;
    double ReferenceMusicSeconds;
    double CompensationSeconds;
    // Perfil de dispositivo/pista y CalibrationStatus.
};
// Resultado discriminado: Rejected(reason), Duplicate(prior), Processed(hit).
// Solo Processed tiene juicio Perfect/Good/Miss nuevo.
FBCXAttackOutcome SubmitAttack(const FBCXAttackInput& Input);
FBCXTimingEvaluation EvaluateOriginalInput(
    const FBCXAttackInput& Input, const FBCXCalibrationSnapshot& Calibration,
    const FBCXTimingConfig& Config, double NowSeconds) const;
// Evaluate no escribe ProcessedAttacks ni consume BeatId.
FBCXLayerOutcome RequestOrRenewLayer(
    uint64 Generation, FName LayerId, double DurationBeats);
void HandleQuartzEvent(const FBCXCommandToken& Token, EQuartzCommandDelegateSubType Event);
void HandleMetaSoundReady(const FBCXLayerToken& Token, bool bReady);
void InvalidateSession(EBCXSessionEndReason Reason);
```

MusicClockSubsystem posee calibración/transporte y cancelación; CombatComponent posee el commit deduplicado; MusicLayerManager posee registros/ciclo/renovación. TimingEvaluator no accede a MetaSounds ni reproduce audio. No exponer una función MarkProcessed que BP pueda ejecutar antes de validación. En auditoría figuran dependencias y APIs contrastadas, con límites de evidencia.

## Alcance de la próxima PR #2 y puerta de Fase A

Cada PR debe ser pequeña, revisable, sin modificar DOCX originales y sin mezclarse con red. No fusionar automáticamente. Dependencias son secuenciales; las pruebas se repiten solo por cambios relevantes.

| PR | Trabajo y entregables | Dependencia | Aceptación verificable |
| --- | --- | --- | --- |
| PR #1 actual | Cierre de auditoría, contrato y pruebas | Base documental | Originales idénticos a base, correcciones 1–12 trazables, ningún cambio de main |
| PR #2 Fase A | .uproject UE5.6, módulo runtime y targets Editor/Game, Config, gitignore, LFS, mapa graybox y personaje/cámara/input | Entorno disponible y revisión PR #1 | Build Editor Win64 real sin Live Coding; BPs compilados; caminar/saltar/cámara en PIE y standalone sin input duplicado |

PR #2 añade construcción real, no otro informe de diseño. Implementar solo base controlable: ABCXPlayerCharacter/Controller, BP_BCX_PlayerCharacter con placeholder autorizado, IMC/IA, Map_Prototype_Combat, GameMode y configuración de mapas. Acompañar instrucciones breves de build y evidencia del entorno. No implementar combate, capas/MetaSounds, calibración ni CPU todavía; sus contratos quedan cerrados aquí para trabajo posterior, sin ampliar ahora esa planificación ni abrir otras PR.

Bloqueos reales para construir/validar PR #2: no hay .uproject en repo; no se localizó engine 5.6 en Launcher/ruta habitual/registro consultados; no se localizó VS/MSVC con vswhere ni Windows SDK en ruta habitual. Confirmar instalación o ruta alternativa y compilador/SDK soportados antes del build. No se requiere música final, calibración aprobada ni personajes finales para comenzar movimiento: placeholders autorizados bastan. Git LFS está disponible (3.7.1); configurar .gitattributes antes de los nuevos uasset/umap sin migrar los DOCX existentes. El permiso de push a esta rama ya está comprobado. No instalar herramientas ni empezar implementación en este cierre.

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
| T11 | Referencia 100 s/0 s; input 100.500 s; procesar 100.500 y 100.700 s, c=0 | Perfect en ambos, InputDeltaSeconds=0.500; CalibrationAge cambia y nunca desplaza juicio |
| T12 | Cambiar demora hasta superar MaxInputQueueAge o vigencia de calibración | Rechazo técnico sin Miss/commit; umbrales configurables separados de ventanas |
| T13 | NaN/Inf/0 BPM, generación vieja, calibración inválida, timestamp futuro/pre-sesión | Ningún registro definitivo, beat consumido, score, daño ni capa; causa explícita |
| T14 | Rechazar por calibración ausente; recuperar y reenviar payload idéntico aún vigente | Un solo commit al validar; evento rechazado no quedó como procesado; sin reintento automático |
| T15 | Repetir mismo SessionGeneration+AttackId; cambiar payload; usar ID igual en sesión nueva | Duplicado sin efectos; conflicto rechazado; sesión nueva independiente; notifies viejos ignorados |
| T16 | Pedir capa dos veces antes/después de QUEUED, antes de STARTED | Requested/Scheduled no se convierten en Active por petición; un componente y un PlayQuantized |
| T17 | QUEUED/ABOUT_TO_START/STARTED duplicados, fuera de orden o generación/token viejos | Solo STARTED vigente confirma Active; reconciliación conserva ciclo; inválidos no mutan |
| T18 | Renovar Requested/Scheduled/Active y justo después de ExpiryBeat | Extender beats con mismo componente/token; no comandos extra; capa vencida entra Expiring |
| T19 | Cruce de expiración y renovación en Expiring; callback fin ausente | Una parada; renovación rechazada; timeout limpia watcher/delegate/registro/componente |
| T20 | Reiniciar con PlayQuantized pendiente y callback STARTED tardío | Cancelación física sin audio viejo; token invalidado no activa ni daña; nueva generación aislada |
| T21 | Ready true/false, WatchOutput fallido y Perfect/Good/Miss independientes | Ready no califica ni activa Scheduled; return true del watcher no equivale a Ready |
| T22 | CalibrationStale con audio Playing; FailedToQueue con calibración válida | Causas independientes; no convertir errores técnicos en Miss; audio opcional fallido no repite ataque |
| T23 | Pausa/foco/reanudar, cambio BPM inválido y cambio BPM válido por configuración | Pausa invalida y reanudar reinicia; inválido no muta; válido limpia/recalibra; no tempo en caliente |
| T24 | Configurar ventanas distintas válidas y BPM que las invalida | Evaluador usa configuración; defaults ±50/±100 ms; invalidez rechazada sin clamping silencioso |
| T25 | No llegan QUEUED/STARTED/fin y repetir crear/expirar 100 capas | Timeouts acotados, sin registros huérfanos; conteos de componentes/watchers/comandos no crecen |

T05 requiere observación independiente del audio: grabación loopback o herramienta equivalente autorizada, comparación de transientes con referencia de pista y timestamps. Un log de reloj que se compara consigo mismo no prueba sincronía audible. Latencia constante y deriva son medidas distintas; registrar ambas y la compensación usada. T06 no puede prometer iguales entradas físicas a distinto FPS; distingue prueba matemática de prueba humana. Los umbrales son objetivos propuestos, no resultados alcanzados.

## Compilación y evidencia futura

Comandos de ejemplo para PR #2 y posterior empaquetado del slice, no ejecutados. Ajustar EngineRoot a instalación real y confirmar nombres de targets; trabajar desde la raíz del checkout. BuildCookRun completo no es un criterio ya cumplido de Fase A.

```powershell
$engineRoot = 'C:\Program Files\Epic Games\UE_5.6'
$projectFile = Join-Path (Get-Location) 'BeatCoreX.uproject'
& "$engineRoot\Engine\Build\BatchFiles\Build.bat" BeatCoreXEditor Win64 Development "-Project=$projectFile" -WaitMutex
# Solo con build exit code 0: abrir editor, compilar BPs y ejecutar pruebas.
& "$engineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$(Join-Path (Get-Location) 'Artifacts\Windows')"
```

En cada PR de implementación guardar comandos, códigos de salida, logs relevantes y pruebas pendientes. Pruebas del evaluador bajo Unreal Automation; test funcional en mapa para vida/ataque/reinicio. Captura de partida y manifiesto para QA manual. No añadir un workflow que declare éxito sin tener runner UE5.6 y assets disponibles. No subir Binaries/Intermediate/Saved ni ejecutables al Git de código; definir ubicación de entrega revisable para el paquete cuando exista.

## Validación de esta entrega

Esta PR solo incorpora los dos Markdown de planificación. Verificar con `git diff --name-status <base>...HEAD`, `git diff --check` y hashes de blobs Git de los 13 DOCX frente al commit base. No existen resultados de ejecución T01–T25 todavía. El informe no autoriza considerar el proyecto compilado ni jugable. Este cierre termina la expansión documental: el siguiente trabajo autorizado por una solicitud de implementación será el alcance acotado de PR #2.
