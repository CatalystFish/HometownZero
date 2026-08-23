# Roads of Death — v2.0 One-Sheet

**Working title:** Hometown Zero *(alternates: Deadblock, Last Street Home)*
**Genre:** Open-world zombie survival (single-player / co-op 1–4)
**Platform:** PC first; console post-launch
**Engine:** Unreal Engine 5
**Business model:** Premium ($20–25), city-pack DLC, UGC hometown importer
**Team target:** 1–3 devs + contract art/audio
**Original design:** "Roads of Death" v1.1 one-sheet, c. 2008 — this is the indie-scale rewrite.

---

## Synopsis

The outbreak happened on your street, not someone else's.

*Hometown Zero* is a sandbox survival game set in real American cities, generated street-for-street from open map data. Players scavenge recognizable neighborhoods, turn any building into a safehouse, craft everything from salvaged materials, and carve out territory against escalating hordes — while AI survivor factions trade, raid, and negotiate around them. No classes, no script, no end-game: survive and rebuild, your way.

## The Hook (why this isn't another zombie game)

Zombie survival is saturated. **Real geography is not.**

Every building in the game is generated from its real-world type: hospitals hold meds, hardware stores hold boards and nails, police stations hold guns. Loot logic players already understand because they've walked these blocks.

Flagship feature: **the Hometown Importer** — players generate a playable survival world from their own town using free OpenStreetMap data. Infinite user-generated maps, infinite streamer moments ("I survived MY house").

## What Changed from v1.1

| v1.1 (2008) | v2.0 |
|---|---|
| MMORPG | Single-player / drop-in co-op (1–4), host-your-own-world |
| Continental US at launch | One district → one city → city packs |
| Player clans & treaty organizations | AI survivor factions with diplomacy, trade, betrayal |
| PvP zones with NPC enforcement | Cut from launch; optional hardcore mode later |
| Google Maps API integration | OpenStreetMap → procedural district generator (offline, no licensing risk) |
| Everything craftable incl. 18-wheelers & houseboats | Focused crafting: weapons, tools, gear, fortifications; one vehicle class at EA |

**Kept intact:** skill-box progression (no classes), player-driven sandbox with no scripted end-game, everything-from-scavenged-materials economy, faction territory politics, the real-world setting.

---

## Core Loop

1. **Scavenge** — building-type-driven loot across real streets
2. **Craft/repair** — quality depends on materials + skill (not X+Y=Z)
3. **Fortify** — claim and reinforce any structure as a safehouse
4. **Defend** — escalating horde pressure; noise and light attract the dead
5. **Expand** — claim territory; negotiate or fight neighboring AI factions

## Play Mechanics

- Skill-box progression: use-to-improve, starting "emphasis" professions only
- Equipment tiers: wearables, weapons (looted modern ↔ improvised), tools, gear
- Structure claiming, repair, and upgrade (safehouses, barricades, watchtowers)
- Faction system: territory control, treaties, raids, migration under resource pressure
- Vehicles at Early Access (one class, e.g., scavenged cars); mounts cut

## Technology

- UE5 World Partition for chunked streaming districts
- OSM → procedural generation pipeline: footprints extruded, tagged by type, loot tables auto-assigned
- Deterministic offline generation per region — no server costs, no streaming dependency, mod-friendly
- Client-side generation sidesteps OSM share-alike distribution concerns

## Target Audience

18–45, core PC gamers; secondary audience of geography nerds, urbanists, and streamers drawn by the real-place hook. Designed for longevity: no perceived end-game, faction politics always shifting, UGC maps endless.

## Key Features

- **Your backyard, overrun.** Real streets and buildings, playable.
- **Player-driven progression.** No scripts, no quest log — survive and prosper.
- **Faction-generated threat.** Zombies are weather; factions are the story. Politics, war, and trade shift the map.
- **Crafting with reputation.** Item quality = materials + crafter skill + process. Nothing is X+Y=Z.
- **Infinite worlds.** City packs at launch-scale cost; the Hometown Importer makes every player's map personal.

## Marketing Summary

Lead with the setting, not the zombies — saturation means "zombie survival" is a category, not a hook. The campaign is built on recognition: side-by-side shots of real landmarks vs. in-game ruins, streamer hometown challenges (#MyStreetChallenge), and the Importer demo as the viral trailer. Wishlist driver: free "one neighborhood" demo generated from a famous district (e.g., downtown Austin or Brooklyn).

## Roadmap

- **Phase 0 — Spike (4–6 wks):** OSM → walkable 3D neighborhood in-engine. Proves the pipeline.
- **Phase 1 — Vertical slice (3–6 mo):** One district, full loop, one faction, no vehicles.
- **Phase 2 — Early Access:** Full city, 3–4 factions, vehicles, skill-box complete.
- **Phase 3:** Co-op, city packs (LA, NYC…), Hometown Importer flagship release.

## Risks

- Zombie fatigue → mitigate by leading all marketing with the map hook
- Scope creep (v1.1 instincts) → district-first phasing is law
- OSM licensing → client-side generation + attribution compliance
- Generation quality variance between cities → curated launch-city list, Importer flagged beta until proven

---

*From the original v1.1: "a chance for players to 'see' their known world thrust into the hands of zombies." That line is still the whole pitch.*
