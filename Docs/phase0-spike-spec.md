# Phase 0 Spike Spec — OSM → Walkable District (Hometown Zero)

**Goal:** Prove that a real neighborhood can be pulled from OpenStreetMap and turned into a walkable 3D district in Unreal Engine 5 with a small team in weeks, not years.

**Duration:** 4–6 weeks part-time.
**Kill criterion:** If walkable districts require heavy per-city hand-authoring, the concept needs rethink before any more investment.

---

## Architecture

```
┌──────────────┐   HTTPS    ┌──────────────┐   offline    ┌───────────────┐
│ Overpass API │ ────────►  │ osm_pipeline │ ──────────►  │ district.json │
│  (free, no   │            │     .py      │              │  (the game's  │
│   API key)   │            └──────────────┘              │  map format)  │
└──────────────┘                                          └───────┬───────┘
                                                                  │
                     ┌────────────────────────────────────────────┼─────┐
                     ▼                                            ▼
          Path A: OBJ preview import                    Path B: UE5 runtime reader
          (Blender / Windows 3D Viewer /                (reads JSON at play time,
           drag into UE as static mesh)                  spawns geometry + loot tables)
```

- **district.json** is the single source of truth: local-meter coordinates, buildings classified by real-world type, heights estimated from tags, road ribbons by class.
- **Path A** proves visual/walkability fast. **Path B** is the production direction and proves loot-table linkage.

## What the pipeline already does (Scripts/osm_pipeline.py)

1. Downloads buildings (`way["building"]`), roads (`way["highway"]`) and POI nodes for a bounding box via Overpass.
2. Projects lat/lon → local meters (east/north/up), origin = bbox center.
3. Classifies every building into a loot category from OSM tags:
   `medical, emergency, education, food, hardware, weapons_outdoors, fuel, retail, office, industrial, residential, civic, unknown`
4. Estimates height: `height` tag → `building:levels × 3.2 m` → seeded random range per category (deterministic per building ID).
5. Writes **district.json** and an optional **OBJ preview** (extruded prisms + road ribbons).
6. **v0.2:** also fetches `shop` / `amenity` / `office` nodes and point-in-polygon joins them into parent footprints — a `building=yes` containing a supermarket node becomes `food`. Junk amenities (benches, bins, parking) filtered. Verified on Pike Place, Seattle: `unknown` rate 139→92 of 219 buildings (classified coverage 37%→58%); remaining unknowns are mostly structures with no mapped interior POIs (park garages, towers, vacancies). Future signals: `craft` / `tourism` / `healthcare` nodes, area-POI joins, address inference.
7. **v0.3:** also joins `healthcare` / `craft` node types (junk healthcare like spas filtered; bakery/brewery/winery/distillery crafts → `food`, all other crafts → `hardware`). Re-verified on Pike Place with a single live fetch: unknown 91/219, POI join rate 331/342 — marginal on this shop-dense bbox; the added signals compound in cities with heavier craft/clinic mapping.
8. **v0.5:** area-POI join — Overpass now also returns closed *ways* tagged `shop` / `amenity` / `craft` / `office` / `tourism` / `leisure` / `healthcare`; ways without a `building` tag that aren't already buildings are reduced to a pseudo-POI at their shoelace polygon centroid and joined into parent footprints by the existing rule. Land-cover junk filtered key-aware (parking lots, pitches/tracks/playgrounds, parks/gardens, artwork) so it can't become buildings or POIs; classifier extended additively (hotels/hostels → `residential`, museums/galleries/cinemas/theatres → `civic`, fitness/sports centres → `weapons_outdoors`). Verified on Pike Place with a single live fetch: 234 buildings (up from 219 — shop/amenity areas without building tags now count as structures themselves), unknown 96/234 ≈ 41% vs 91/219 ≈ 42%, POI join rate 336/349 with 47 re-classifications and 7 centroid pseudo-POIs — downtown Seattle maps its retail as shop-ways already, so gains here are mostly the direct area-building additions; leisure/tourism signals should compound in residential/mixed districts.

## Usage

```
python Scripts/osm_pipeline.py --bbox "minlat,minlon,maxlat,maxlon" --out my_district.json --obj preview.obj
```

(run from the `HometownZero\` repo root; pick any bbox on https://www.openstreetmap.org — Export panel shows coordinates)

## UE5 integration steps (Path A — week 3–4)

1. New project: **Third Person C++ or Blueprint template**.
2. Import preview.obj (or per-category OBJs) → Static Mesh; use **auto-generated box or convex collision per building** (complex-as-simple trimeshes are CPU-expensive and degrade line traces/AI nav).
3. Drop mesh into level, add Sky Light + Directional Light + exponential height fog.
4. Add Nav Mesh Bounds Box over the district; verify a simple AI character paths streets.
5. Walk test checklist:
   - [ ] Streets are navigable end to end
   - [ ] Building footprints block movement correctly (no clipping)
   - [ ] Frame time acceptable (>30 fps on mid hardware) at ~2 km²
   - [ ] Recognizable vs. the real map side-by-side

## UE5 integration steps (Path B — week 4–6)

1. `UDistrictReader` Blueprint Function Library: parse district.json (JsonUtilities module).
2. Spawn `InstancedMeshComponent` per category using one box/roof material each → proves instancing perf.
3. Per-building actor component `LootSource{Category}` reading the category field — the hospital-has-meds moment.
4. Runtime Procedural Mesh fallback only if OBJ path underperforms.

## Success criteria

| Metric | Target |
|---|---|
| Fetch + process 2 km² | < 60 s |
| Building classification coverage | ≥ 70% not `unknown` |
| Walkthrough | Player crosses district without falling through geometry |
| Perf | ≥ 30 fps standalone, mid-range GPU |
| Recognition test | 3 people identify the neighborhood from memory |

## Schedule

- **Wk 1–2:** Pipeline hardening (multipolygon relations, height tuning), batch-fetch 3 candidate cities
- **Wk 3–4:** Path A in-engine walkthrough, perf pass
- **Wk 5–6:** Path B category materials + LootSource proof, go/no-go review

## Non-goals (explicitly out of scope)

Interiors, doors, props, zombies, combat, textures beyond flat colors, multipolygon courtyards (v0.2), streaming multi-district worlds.

## Risks

| Risk | Mitigation |
|---|---|
| Overpass rate limits | Mirror endpoints baked into script; cache all downloads |
| Ugly/incorrect footprints | Acceptable for spike — silhouette matters, beauty is Phase 1 |
| Collision cost on imported meshes | Use box/convex per prism (decided Aug 2026); complex-as-simple only as last resort |
| Height guesses look wrong | Only breaks immersion slightly; real heights come from later datasets |

---

*Companion docs: `game-design-doc.md` (GDD), `dev-roadmap-4week.md` (day-by-day slice plan), `ue5-platform-research.md`, `osm-parsing-libraries.md`.*
