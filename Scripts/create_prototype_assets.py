"""Run inside UE5.6 Editor after the native module has built.

Creates six input assets, one context, two data-only Blueprints and a graybox.
Refuses to overwrite assets or close a dirty map. No standalone Python mock
can validate Unreal asset serialization or Blueprint compilation.
"""

import unreal


ROOT = "/Game/BeatCoreX"
INPUT = ROOT + "/Input"
PLAYER_BP = ROOT + "/Characters/Player/BP_BCX_PlayerCharacter"
MODE_BP = ROOT + "/Blueprints/Core/BP_BCX_PrototypeGameMode"
MAP = ROOT + "/Maps/Map_Prototype_Combat"
ACTION_TYPES = {
    "IA_Move": unreal.InputActionValueType.AXIS2D,
    "IA_Look": unreal.InputActionValueType.AXIS2D,
    "IA_Jump": unreal.InputActionValueType.BOOLEAN,
    "IA_Punch": unreal.InputActionValueType.BOOLEAN,
    "IA_Kick": unreal.InputActionValueType.BOOLEAN,
    "IA_Headbutt": unreal.InputActionValueType.BOOLEAN,
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def save_asset(asset):
    require(unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
            "Failed to save " + asset.get_path_name())


def create_data_asset(name, directory, asset_class):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, directory, asset_class, factory)
    require(asset is not None, "Failed to create " + directory + "/" + name)
    return asset


def create_blueprint(path, parent):
    bp = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(path, parent)
    require(bp is not None, "Failed to create " + path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    require(unreal.BlueprintEditorLibrary.generated_class(bp) is not None,
            "Blueprint has no generated class: " + path)
    return bp


def modifier(context, cls, **properties):
    obj = cls(outer=context)
    for name, value in properties.items():
        obj.set_editor_property(name, value)
    return obj


def create_mappings(context, actions):
    # Build the struct array explicitly: editing map_key's returned Python
    # struct can otherwise edit a copy rather than the context's stored entry.
    mappings = []

    def add(action, key, modifiers=()):
        mappings.append(unreal.EnhancedActionKeyMapping(
            action=actions[action], key=unreal.Key(key_name=key),
            modifiers=list(modifiers), triggers=[]))

    def negate():
        return modifier(context, unreal.InputModifierNegate,
                        x=True, y=True, z=True)

    def swizzle():
        return modifier(context, unreal.InputModifierSwizzleAxis,
                        order=unreal.InputAxisSwizzle.YXZ)

    add("IA_Move", "W", [swizzle()])
    add("IA_Move", "S", [negate(), swizzle()])
    add("IA_Move", "A", [negate()])
    add("IA_Move", "D")
    add("IA_Look", "MouseX")
    add("IA_Look", "MouseY", [negate(), swizzle()])
    add("IA_Jump", "SpaceBar")
    add("IA_Punch", "LeftMouseButton")
    add("IA_Kick", "E")
    add("IA_Headbutt", "Q")
    # No Pressed/Pulse/Hold triggers: Bool uses its default threshold;
    # Started binding records exactly one intention for each press transition.
    context.set_editor_property("mappings", mappings)
    require(len(context.get_editor_property("mappings")) == 10,
            "Mapping serialization did not retain all ten entries")
    save_asset(context)


def create_map(game_mode_class):
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(level_editor.new_level(MAP, is_partitioned_world=False), "Failed to create map")
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    require(cube is not None, "Engine cube asset missing")

    def box(label, location, scale):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        require(actor is not None, "Failed to spawn " + label)
        actor.set_actor_label(label)
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(cube)
        component.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector(*scale))
        return actor

    box("BCX_Floor", (0, 0, -50), (30, 30, 1))
    for label, location, scale in (
        ("BCX_WallNorth", (1500, 0, 100), (1, 30, 3)),
        ("BCX_WallSouth", (-1500, 0, 100), (1, 30, 3)),
        ("BCX_WallEast", (0, 1500, 100), (30, 1, 3)),
        ("BCX_WallWest", (0, -1500, 100), (30, 1, 3)),
        ("BCX_JumpBlock", (300, 200, 35), (2, 2, 0.7)),
    ):
        box(label, location, scale)
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 110))
    require(start is not None, "Failed to spawn PlayerStart")
    start.set_actor_label("BCX_PlayerStart")
    light = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500))
    require(light is not None, "Failed to spawn light")
    light.set_actor_rotation(unreal.Rotator(-45, -30, 0), False)
    light_component = light.get_component_by_class(unreal.DirectionalLightComponent)
    light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    light_component.set_editor_property("intensity", 5.0)

    # GlobalDefaultGameMode in Config is sufficient; set the map override too
    # when WorldSettings is returned by the editor's actor inventory.
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.WorldSettings):
            actor.set_editor_property("default_game_mode", game_mode_class)
    require(level_editor.save_current_level(), "Failed to save populated map")


def main():
    require(unreal.SystemLibrary.get_engine_version().startswith("5.6."),
            "This generator requires UE5.6")
    require(not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages(),
            "Save the current map before creating prototype assets")
    require(not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages(),
            "Save current content before creating prototype assets")
    character_class = unreal.load_class(None, "/Script/BeatCoreX.BCXPlayerCharacter")
    mode_class = unreal.load_class(None, "/Script/BeatCoreX.BCXPrototypeGameMode")
    require(character_class is not None and mode_class is not None,
            "Compile and load BeatCoreXEditor before running this script")
    paths = [INPUT + "/" + name for name in ACTION_TYPES]
    paths += [INPUT + "/IMC_Gameplay", PLAYER_BP, MODE_BP, MAP]
    conflicts = [path for path in paths if unreal.EditorAssetLibrary.does_asset_exist(path)]
    require(not conflicts, "Refusing to overwrite existing assets: " + ", ".join(conflicts))

    actions = {}
    for name, value_type in ACTION_TYPES.items():
        action = create_data_asset(name, INPUT, unreal.InputAction)
        action.set_editor_property("value_type", value_type)
        save_asset(action)
        actions[name] = action
    context = create_data_asset("IMC_Gameplay", INPUT, unreal.InputMappingContext)
    create_mappings(context, actions)

    player = create_blueprint(PLAYER_BP, character_class)
    player_defaults = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(player))
    player_defaults.set_editor_property("gameplay_mapping_context", context)
    for name, prop in (
        ("IA_Move", "move_action"), ("IA_Look", "look_action"),
        ("IA_Jump", "jump_action"), ("IA_Punch", "punch_action"),
        ("IA_Kick", "kick_action"), ("IA_Headbutt", "headbutt_action"),
    ):
        player_defaults.set_editor_property(prop, actions[name])
    unreal.BlueprintEditorLibrary.compile_blueprint(player)
    save_asset(player)
    mode = create_blueprint(MODE_BP, mode_class)
    mode_defaults = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(mode))
    mode_defaults.set_editor_property("default_pawn_class",
                                    unreal.BlueprintEditorLibrary.generated_class(player))
    unreal.BlueprintEditorLibrary.compile_blueprint(mode)
    save_asset(mode)
    create_map(unreal.BlueprintEditorLibrary.generated_class(mode))
    unreal.log("BCX_ASSETS_SAVED: inspect Blueprint compiler results and test PIE; this is not gameplay validation.")


if __name__ == "__main__":
    main()
