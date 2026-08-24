"""Headless map creation: clean DowntownSeattle map with lights + PlayerStart.

Run via -run=pythonscript. Creates /Game/Maps/DowntownSeattle with nothing
else in it - the runtime district spawns itself on Play.
"""
import unreal

MAP_PATH = "/Game/Maps/DowntownSeattle"


def spawn(cls, loc, rot=(0, 0, 0)):
    system = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    location = unreal.Vector(*loc)
    rotation = unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2])
    return system.spawn_actor_from_class(cls, location, rotation)


def main():
    unreal.EditorLevelLibrary.new_level(MAP_PATH)

    # Lighting: directional sun + sky light so the city isn't pitch black.
    sun = spawn(unreal.DirectionalLight, (0, 0, 500000), (0, -40, 35))
    if sun:
        sun.set_editor_property("intensity", 8.0)

    spawn(unreal.SkyLight, (0, 0, 1000000), (0, 0, 0))
    spawn(unreal.SkyAtmosphere, (0, 0, 0), (0, 0, 0))
    spawn(unreal.PlayerStart, (0, 0, 200), (0, 0, 0))

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("[HZMap] created and saved %s" % MAP_PATH)


main()
