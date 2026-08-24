# Morning Report — Aug 24, 2026 (autonomous night shift)

*TL;DR: The game now builds its own city from map data at runtime and zombies chase you through it. Everything below was verified headless — your only job is to press Play.*

## What exists now (all committed through `2bc95ac`+)

### Runtime city (Path B — the production architecture)
- `AHDistrictManager` reads `Scripts/samples/pike_place.json` at Play start and spawns the district itself: 234 instanced buildings grouped by loot category (box collision per building, per spec), **3,716 road segments**, ground plane, navmesh bounds, and **10 zombies** at random street positions.
- Verified headless twice (`UnrealEditor-Cmd -game -nullrhi`): zero crashes, all counts match the JSON exactly. Log lines to look for: `[H District]`.

### Gameplay
- `AHZCharacter`: real third-person walking character — WASD, mouse look, Space jump, **Shift sprint (900 cm/s)**, 100 HP, zombie contact deals 10 dmg/0.6s, death → respawn at PlayerStart.
- Zombies: green cubes, chase within 80m, wander otherwise, pathfind over the district via dynamic navmesh.

### Visuals
- `Tools/generate_materials.py` (headless commandlet) generates 16 solid-color materials (`/Game/HZMaterials/M_HZ_*`) — buildings are color-coded by loot category, roads dark asphalt, ground scrub-green, zombies green. Re-runnable any time.

### Pipeline v0.6
- Area-POI dedupe (no more double-boxing where a shop polygon nests in its building).
- **Multi-city comparison** (for launch-city pick): Portland **314 buildings, 66% classified, 11/13 categories — strongest candidate**; Austin 437 buildings but only 31% classified (weak local POI mapping); Seattle baseline 234/59%.
- All three cities have JSON+OBJ+glTF samples in `Scripts/samples/`.

### Docs
- `Docs/devlog.md` — seeded retroactively (Aug 22–24).
- `Docs/dev-roadmap-4week.md` — Week 1 Mon–Wed boxes ticked (traversal box left open for you).
- `Docs/thursday-brief.md` — your mechanical test plan.

## Your morning test (10 minutes)

1. Open the project (bat file or VS F5). Open any level — the GameMode spawns the district automatically in every map.
2. **Press Play.** Expect: colored box-city + dark streets + ground plane + green zombies. One or two zombies should reach you within ~30s and start hitting you (watch health via log lines `[HZ Player]`).
3. Die on purpose (stand near zombies). Expect respawn at your PlayerStart with full health.
4. `stat unit` numbers → write them in this file.
5. **Known overlap:** if your imported glTF city (the pretty one) coexists with the runtime box city, they'll z-fight — select the imported `pike_place` actor and hide it; the runtime city is the future. (Merging the two — real footprints + your glTF look — is the next big work item.)
6. Try Portland: select the DistrictManager actor (spawned at runtime — easiest: temporarily change the default `DistrictJsonPath` in `HZDistrictManager.cpp` line ~29 to `"Scripts/samples/portland.json"`, rebuild, Play).

## Watch-outs

- **The editor rewrites `Config/DefaultInput.ini` on exit** and may drop hand-added mappings again (it ate WASD once tonight). If movement breaks after an editor session, check that file first. Permanent fix = migrate to Enhanced Input assets (roadmap item).
- Zombie damage is log-only feedback right now — no HUD yet (that's Week 4 scope).
- Runtime buildings are AABB boxes; irregular footprints overhang slightly. Real footprint geometry comes after the spike.

## Suggested next session

1. Your Play test verdict (above)
2. Then Week 1 · Thu proper: draw-call before/after numbers for the HISM city, and the first loot container prototype (hospital → meds) using `LootSource{category}`.
