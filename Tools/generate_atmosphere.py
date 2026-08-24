"""Headless atmosphere pass for Hometown Zero (run via -run=pythonscript).

Loads /Game/Maps/DowntownSeattle in place and repaints it as a bleak,
overcast, post-apocalyptic morning: a sickly warm sun, murky fog, and a
desaturated grunge post-process. Every field is set defensively - one bad
knob must never abort the whole run (unsupported names are logged and dropped).

Idempotent: each actor is tuned if already present and spawned only if
missing, so re-running never duplicates actors.

Notes:
- Actors are spawned with spawn_actor_from_class (spawn_actor_from_object
  cannot nativize a Python class in UE 5.8).
- Sun intensity/color and fog density/falloff/color live on their components
  (ULightComponent / UExponentialHeightFogComponent), not the actors.
"""
import unreal

MAP_PATH = "/Game/Maps/DowntownSeattle"
TAG = "[HZAtmos]"


def log(msg):
    unreal.log("%s %s" % (TAG, msg))


def warn(msg):
    unreal.log_warning("%s %s" % (TAG, msg))


def safe_set(target, name, value, label):
    try:
        target.set_editor_property(name, value)
        log("set %s = %s" % (label, value))
        return True
    except Exception as exc:
        warn("unsupported field '%s' (%s): %s" % (label, name, exc))
        return False


def spawn(cls, location=(0, 0, 0), rot=(0, 0, 0)):
    system = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return system.spawn_actor_from_class(
        cls, unreal.Vector(*location),
        unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]))


def find_actor(cls):
    system = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    try:
        actors = system.get_all_level_actors()
    except Exception as exc:
        warn("get_all_level_actors failed: %s" % exc)
        return None
    for actor in actors:
        try:
            if actor.get_class() == cls.static_class():
                return actor
        except Exception:
            continue
    return None


def get_component(actor, property_name, class_substring):
    try:
        component = actor.get_editor_property(property_name)
        if component is not None:
            return component
    except Exception:
        pass
    try:
        for component in actor.get_components():
            try:
                if class_substring in component.get_class().get_name():
                    return component
            except Exception:
                continue
    except Exception:
        pass
    return None


def tune_sun(light):
    component = get_component(light, "light_component", "DirectionalLight")
    if component is None:
        warn("sun light_component not found - falling back to actor")
        component = light
    safe_set(component, "intensity", 2.5, "sun intensity")
    safe_set(component, "light_color", unreal.Color(230, 191, 140, 255),
             "sun light_color")


def tune_fog(fog):
    if fog is None:
        warn("no ExponentialHeightFog")
        return
    component = get_component(fog, "component", "HeightFog")
    if component is None:
        warn("fog component not found - falling back to actor")
        component = fog
    safe_set(component, "fog_density", 0.02, "fog_density")
    safe_set(component, "fog_height_falloff", 0.08, "fog_height_falloff")
    safe_set(component, "fog_inscattering_luminance",
             unreal.LinearColor(0.45, 0.48, 0.45, 1.0),
             "fog_inscattering_luminance")
    safe_set(component, "enable_volumetric_fog", False,
             "enable_volumetric_fog")


def tune_post_process(volume):
    if volume is None:
        warn("no PostProcessVolume")
        return
    settings = unreal.PostProcessSettings()

    safe_set(settings, "scene_color_tint",
             unreal.LinearColor(0.85, 0.90, 0.95, 1.0), "scene_color_tint")
    safe_set(settings, "color_saturation",
             unreal.Vector4(0.6, 0.6, 0.6, 1.0), "color_saturation")
    safe_set(settings, "vignette_intensity", 0.5, "vignette_intensity")
    safe_set(settings, "film_grain_intensity", 0.15, "film_grain_intensity")
    safe_set(settings, "bloom_intensity", 0.3, "bloom_intensity")

    safe_set(volume, "unbound", True, "unbound")
    safe_set(volume, "settings", settings, "settings")


def main():
    level_system = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level_system.load_level(MAP_PATH)

    light = find_actor(unreal.DirectionalLight)
    if light is None:
        log("no DirectionalLight in map - spawning sickly sun")
        light = spawn(unreal.DirectionalLight, (0, 0, 500000), (0, -25, 30))
    if light:
        log("DirectionalLight: %s" % light.get_name())
        tune_sun(light)
    else:
        warn("failed to obtain a DirectionalLight")

    fog = find_actor(unreal.ExponentialHeightFog)
    if fog is None:
        fog = spawn(unreal.ExponentialHeightFog)
    if fog:
        log("ExponentialHeightFog: %s" % fog.get_name())
    tune_fog(fog)

    volume = find_actor(unreal.PostProcessVolume)
    if volume is None:
        volume = spawn(unreal.PostProcessVolume)
    if volume:
        log("PostProcessVolume: %s" % volume.get_name())
    tune_post_process(volume)

    level_system.save_current_level()
    log("done - atmosphere applied and saved")


main()
