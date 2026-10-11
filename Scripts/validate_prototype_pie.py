"""Run in UE5.8 Editor with Map_Prototype_Combat already in PIE.

Injects real Enhanced Input action values through its local-player subsystem.
This tests native gameplay/Started semantics, not physical keyboard hardware.
Results and original intent timestamps are written to Saved/Validation.
"""
import json
import pathlib
import time
import traceback
import unreal

ROOT = '/Game/BeatCoreX/Input/'
OUTPUT = pathlib.Path(unreal.Paths.project_saved_dir()) / 'Validation' / 'PIEValidation.json'
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
actions = {name: unreal.load_asset(ROOT + 'IA_' + name)
           for name in ('Move', 'Look', 'Jump', 'Punch', 'Kick', 'Headbutt')}
events = []
report = {'method': 'Real PIE, Enhanced Input vector injection each editor frame', 'tests': {}, 'intents': events}


def get_runtime():
    world = unreal.EditorLevelLibrary.get_game_world()
    if not world:
        raise RuntimeError('PIE world is unavailable')
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = controller.get_controlled_pawn() if controller else None
    if not pawn:
        raise RuntimeError('PIE controller/pawn unavailable')
    context = unreal.load_asset(ROOT + 'IMC_Gameplay')
    subsystems = [obj for obj in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                  if not obj.get_name().startswith('Default__')
                  and obj.has_mapping_context(context) is not None]
    if len(subsystems) != 1:
        raise RuntimeError('Expected one local-player subsystem with IMC_Gameplay')
    return world, controller, pawn, subsystems[0]


def capture(intent):
    events.append({'generation': int(intent.session_generation), 'id': int(intent.attack_id),
                   'time': float(intent.input_time_seconds),
                   'type': str(intent.attack_type).split('.')[-1].split(':')[0].rstrip('>')})


world, controller, pawn, subsystem = get_runtime()
pawn.on_attack_intent.add_callable_unique(capture)
first_generation = unreal.GameplayStatics.get_game_instance(world).get_session_generation()
start_location = pawn.get_actor_location()
start_rotation = controller.get_control_rotation()
jump_base = start_location.z
max_jump_z = jump_base
phase_start = time.monotonic()
phase_index = 0
steps = [('Idle', 0.5, None, (0, 0, 0)), ('Move', 1.0, 'Move', (0, 1, 0)),
         ('Look', 0.5, 'Look', (0.2, 0.1, 0)), ('Jump', 0.25, 'Jump', (1, 0, 0)),
         ('Land', 1.5, None, (0, 0, 0))]
for attack in ('Punch', 'Kick', 'Headbutt'):
    steps += [(attack + 'Hold', 2.0, attack, (1, 0, 0)), ('Release', 0.35, None, (0, 0, 0)),
              (attack + 'Repress', 0.2, attack, (1, 0, 0)), ('Release', 0.35, None, (0, 0, 0))]
steps += [('Repossess', 0.75, None, (0, 0, 0)), ('PostPossessPunch', 0.5, 'Punch', (1, 0, 0)),
          ('Release', 0.35, None, (0, 0, 0)), ('RestartLevel', 2.0, None, (0, 0, 0)),
          ('PostRestartPunch', 0.5, 'Punch', (1, 0, 0)), ('Release', 0.35, None, (0, 0, 0))]
repossessed = False
restart_requested = False
restarted = False
busy = False


def record_check(name, condition, evidence):
    report['tests'][name] = {'status': 'PASS' if condition else 'FAIL', 'evidence': evidence}
    if not condition:
        raise AssertionError(name + ': ' + str(evidence))


def tick(delta):
    global phase_start, phase_index, max_jump_z, repossessed, restart_requested, restarted, busy
    if busy:
        return
    busy = True
    try:
        if phase_index >= len(steps):
            first = [e for e in events if e['generation'] == first_generation]
            second = [e for e in events if e['generation'] != first_generation]
            record_check('HeldAndRepressedAttacks', len(first) == 7, first)
            record_check('IncrementalIds', [e['id'] for e in first] == list(range(1, 8)), first)
            record_check('AttackTypes', [e['type'].split('.')[-1] for e in first] ==
                         ['PUNCH', 'PUNCH', 'KICK', 'KICK', 'HEADBUTT', 'HEADBUTT', 'PUNCH'], first)
            record_check('SessionRestart', len(second) == 1 and second[0]['generation'] == first_generation + 1
                         and second[0]['id'] == 1, second)
            record_check('OriginalTimestamps', all(e['time'] > 0 for e in events) and
                         all(a['time'] < b['time'] for a, b in zip(events, events[1:])), events)
            report['status'] = 'PASS'
            OUTPUT.write_text(json.dumps(report, indent=2), encoding='utf-8')
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.log('BCX_PIE_VALIDATION_PASS')
            return
        name, duration, action_name, value = steps[phase_index]
        elapsed = time.monotonic() - phase_start
        w, pc, p, sub = get_runtime()
        if name == 'Repossess' and not repossessed:
            pc.un_possess()
            pc.possess(p)
            repossessed = True
            record_check('PossessionGeneration', unreal.GameplayStatics.get_game_instance(w).get_session_generation()
                         == first_generation, first_generation)
        if name == 'RestartLevel' and not restart_requested:
            restart_requested = True
            unreal.SystemLibrary.execute_console_command(w, 'RestartLevel', pc)
        if name == 'PostRestartPunch' and not restarted:
            p.on_attack_intent.add_callable_unique(capture)
            restarted = True
        for key, action in actions.items():
            raw = unreal.Vector(*value) if key == action_name else unreal.Vector(0, 0, 0)
            sub.inject_input_vector_for_action(action, raw, [], [])
        max_jump_z = max(max_jump_z, p.get_actor_location().z)
        if elapsed >= duration:
            if name == 'Move':
                loc = p.get_actor_location()
                distance = ((loc.x - start_location.x)**2 + (loc.y - start_location.y)**2)**0.5
                record_check('Movement', distance > 100, {'distance_cm': distance})
            if name == 'Look':
                rot = pc.get_control_rotation()
                record_check('CameraControl', abs(rot.yaw - start_rotation.yaw) > 1,
                             {'yaw': rot.yaw, 'pitch': rot.pitch})
            if name == 'Land':
                record_check('Jump', max_jump_z - jump_base > 30, {'max_height_delta_cm': max_jump_z - jump_base})
            if name.endswith('Hold'):
                expected = 1 + ('Punch', 'Kick', 'Headbutt').index(name[:-4]) * 2
                record_check(name, len(events) == expected, {'intent_count': len(events), 'held_seconds': duration})
            phase_index += 1
            phase_start = time.monotonic()
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        OUTPUT.write_text(json.dumps(report, indent=2), encoding='utf-8')
        unreal.log_error(report['error'])
        unreal.unregister_slate_post_tick_callback(handle)
    finally:
        busy = False


handle = unreal.register_slate_post_tick_callback(tick)
unreal.log('BCX_PIE_VALIDATION_STARTED')
