# Hometown Zero — Game Design Document (v1)

*Status: living draft. Parent design intent: `roads-of-death-v2-one-sheet.md`. Platform research: `ue5-platform-research.md`.*

---

## Pitch

**Hometown Zero** is a single-player zombie survival game set in procedurally generated districts of real American cities, built street-for-street from OpenStreetMap data. Scavenge recognizable neighborhoods, fortify any building, survive escalating hordes, and navigate rival AI survivor factions. Premium Steam Early Access, $20–25.

| | |
|---|---|
| Genre | Open-world survival / sandbox |
| Platform | PC (Steam) |
| Engine | Unreal Engine 5.8 (C++ / Blueprints) |
| Audience | 18–45 core PC; geography nerds & streamers via the real-place hook |
| Team | Solo dev (+ contract art/audio) |

## Design Pillars

1. **Your backyard, overrun.** Real streets and typed buildings drive everything — loot logic players already understand because they've walked these blocks.
2. **Everything from salvage.** No magic shops. Quality comes from materials + skill + process.
3. **Factions are the story; zombies are the weather.** The dead create pressure; the living create drama.
4. **No scripts, no quest log.** Survive and rebuild, your way. Progression is player-authored.

## Core Mechanics

### Movement & Combat
- Third-person, stamina-gated sprint/dodge; noise generated per movement speed.
- Melee-first combat: swing arcs, stamina cost, durability on crafted weapons (nail bat breaks).
- Firearms scarce and loud: shots spike local horde heat; ammo is found, never bought.
- One-hit-down at full health is banned; 3–4 zombie hits down the player, bandages/splints matter.

### Scavenging & Loot (driven by real OSM categories)
Every searchable building carries its pipeline-assigned category → loot table:

| Category | Typical loot |
|---|---|
| medical | bandages, antibiotics, painkillers, surgical tools |
| emergency | body armor, radios, flares, riot gear |
| education | skill books, maps, duct tape |
| food | canned goods, water, seeds (perishables spoil) |
| hardware | nails, planks, tools, wire |
| weapons_outdoors | axes, hunting knives, ammo, bows |
| fuel | gasoline, propane (crafting + horde-night traps) |
| retail | crafting components, clothing, barter goods |
| office | electronics, coffee (stamina buff), documents |
| industrial | scrap metal, machinery parts, welding supplies |
| residential | mixed low-tier everything; safehouse candidates |
| civic | community-blueprint items (farm plans, generator schematics) |
| unknown | junk draws + small chance of anything (exploration reward) |

- Search takes time and makes noise; containers deplete permanently (districts slowly empty — migration pressure).

### Crafting & Quality
- Recipes = component lists + station requirements + optional process minigame.
- Quality tier (Crude/Standard/Fine/Exceptional) = f(material grade, crafter skill, process execution). Not X+Y=Z.
- Repair economy: maintained gear outlives replaced gear; tools degrade with use.

### Safehouse & Base Building
- Claim any building as safehouse (one primary claim early; more later via faction rank).
- Barricade windows/doors (board tiers), assign light/sleep/storage roles per room footprint.
- Noise/light discipline: generators attract hordes; blackout discipline is a strategy.
- Save/load anchored to safehouse state — protected roadmap item, never cut.

### Threat Model (the dead)
- Local heat map: noise events raise heat; wanderers investigate; sustained heat spawns packs.
- **Horde night**: escalating periodic assault targeting claimed safehouse; size scales with wealth/day count.
- Budget: ~50–150 fully-simulated agents (BT+EQS, significance throttling); visual swarm beyond that via ISM/imposters. Never promise more than the sim budget.
- Zombies path streets and breach barricades; interiors of claimed buildings are fight spaces.

### AI Factions (the living)
- MVP: 2–3 rival groups per district with territory zones, needs (they scavenge too), and relations state machine: neutral ↔ trade ↔ hostile.
- Faction raids target resource-rich claims — including yours. Treaties/betrayal emerge from need, not script.
- Faction NPCs use same combat/threat systems as player-facing enemies (no cheating AI).

### Progression
- Skill-box "use to improve": emphasis professions at start (Scavenger, Builder, Medic, Fighter) shape starting kit, not hard limits.
- No XP grind gates on content; skill raises quality ceilings and speeds actions.

## Game Loops

- **Minutes:** move quietly → search → risk/reward push (that pharmacy vs. the noise) → fight or flee.
- **Hours:** plan expedition (map, weight, daylight) → haul home → craft/repair → fortify → spend skill growth.
- **Days/weeks:** horde escalation curve, district depletion forces expansion, faction territory shifts, rebuilding goals (power, water, walls) give long-arc purpose without scripts.

## Asset List (slice-realistic)

- **Building shells:** extruded prisms from pipeline; 1 material per category (~13) + road/terrain/skyline (~5) ≈ **18 flat/stylized materials**. Photorealism banned — stylized diorama look, readable silhouettes.
- **Props:** 10–15 per major category (crates, shelves, med cabinets...) ≈ 60–80 total at slice.
- **Weapons:** ≤15 at slice (bat, nail bat, axe, crowbar, knife, bow, pistol, shotgun, rifle...).
- **Characters:** 1 player rig + 1 zombie rig (variant materials/colors only).
- **VFX:** blood hits, barricade break, muzzle flash, horde-night fog pulse. ≤12 effects.
- **UI screens:** main menu, district select/importer stub, HUD (health/stamina/heat), inventory grid, crafting panel, safehouse panel, faction status, map overlay.
- **Audio:** footstep/noise set, melee/weapon hits, zombie vocal layers (3–4 loops), ambient bed, UI clicks, horde-night sting. No licensed music; contract ambient score post-slice.
- **Attribution asset:** OpenStreetMap splash/credits entry (ODbL obligation) ships in every build.

## Technical Architecture Summary

```
osm_pipeline.py (Python, offline) ──► district.json (v2 schema)
        │                                      │
        ▼                                      ▼
  OBJ/glTF previews              UE5.8 runtime: FJsonSerializer reader
  (Blender/QC/import)            ├─ HISM per category (box/convex collision)
                                 ├─ LootSource{category} components
                                 ├─ NavMesh bounds on street ribbons
                                 └─ BT/EQS zombies (≤150 sim) → MassAI stretch goal
```

- glTF preferred over OBJ once per-category material assignment matters; OBJ fine for slice.
- World Partition OFF at slice scale (~2 km²); revisit at multi-district phase.
- Co-op: deferred until after EA (listen-server replication later; do not architect single-player away from it — keep simulation server-authoritative-friendly).

## Out of Scope for v1

Co-op/multiplayer · building interiors beyond ground-floor fight spaces · vehicles · pets/companions · mod support/Importer plugin API · consoles · photoreal art · MMO anything.

## Risks & Mitigations

| Risk | Mitigation |
|---|---|
| Content treadmill burns solo dev | Typed-building loot tables generate breadth; hand-author depth only |
| Scope creep (v1.1 instincts) | Parking lot rule + weekly DoD gates; cut order protects core loop |
| Zombie fatigue in market | Marketing leads with real-map hook; zombies are the setting, not the pitch |
| Horde perf misses budget | Sim-cap discipline + imposters; profile every Friday |
| Solo burnout | Part-time pacing, shutdown ritual, parking lot captures ideas without derailing |
| ODbL misstep | Attribution in every build; client-side generation; no baked vector distribution |
