# UE5 Platform & Tooling Research (Aug 2026)

*Consolidated findings from web research sessions, Aug 2026. Re-verify version-sensitive items before relying on them in production decisions.*

## Engine & Licensing

- Latest stable: **UE 5.8** (June 23, 2026). Epic states 5.8 is the last planned major UE5 release; UE6 Early Access targeted ~end of 2027.
- Royalty: **5% of gross above $1M lifetime per product, all platforms** ($1M exemption since May 2020). EGS sales royalty-free. **"Launch Everywhere with Epic"** (effective Jan 1, 2025): royalty drops to **3.5%** if the game is on EGS from launch day across platforms. EGS store cut: 100% of first $1M net/product/year (since May–Jun 2025).
- No subscription for game developers. Seat-based Unreal Subscription ($1,850/seat/yr) applies only to non-game companies >$1M annual gross.

## Mesh Import & Scale

- Interchange framework imports **glTF/GLB natively (recommended)**; OBJ also native. FBX still routes through legacy importer (Interchange FBX Experimental through 5.8).
- District scale (200–2000 unique extruded prisms, 15k–150k verts): plain static meshes fine; repeated meshes → HISM. Nanite removes most draw-call concerns for opaque static geometry.
- **Never merge a whole district into one mesh** (loses all culling).
- **Complex-as-simple collision is CPU-expensive**, blocks simulated actors, degrades line traces/AI nav → use auto-generated **box/convex shapes per prism**. (SPEC corrected accordingly.)

## In-Engine Procedural Generation

- ProceduralMeshComponent: prototype-only — no LODs, one material/section, weakens Lumen (no distance fields). Runtime Mesh Component (TriAxis) adds LODs if ever needed.
- PCG production-ready in **5.7** (~2× vs 5.5, GPU compute, editor mode). External data enters via DataTables/CSV or Alembic — **no native JSON ingestion node**.
- Runtime generation works in packaged builds, but editor/offline-baked assets remain the lower-risk path for a solo dev. Project convention already matches: offline generation.

## OSM→UE Tool Landscape

| Tool | Status | Fit |
|---|---|---|
| Cesium for Unreal + Cesium OSM Buildings (Apache-2.0, active) | streams 350M buildings via ion | visualization-grade; no gameplay collision/loot hooks; ion costs at scale |
| StreetMap plugin (Epic's Mike Fricker, MIT) | hobby maintenance; XML only | canonical zero-dep parser reference (`FastXml` FSM) |
| BlenderGIS addon (GPL-3.0, v2.2.15 Dec 2025) | mature QC/manipulation hop | good offline fallback route to FBX/glTF |
| Geopogo OSM City Data Loader (Fab, paid) | requires 5.7+ | spawns thousands of actors; price unverified |
| CityBLD / Houdini Labs OSM nodes | tutorial/free options | alternative pipelines, not needed given custom Python stack |

Conclusion: keep custom pipeline; StreetMap doubles as MIT reference code for in-engine XML parsing.

## Overpass API Etiquette (verified Aug 2026)

- Main instance guideline: **≤10,000 requests/day, <1 GB/day** per user; slot-based concurrency per IP with load-dependent cool-downs; HTTP 429 → pause ~30 s; oversized → HTTP 504. Missing User-Agent has triggered HTTP 406.
- Mirrors: maps.mail.ru/osm/tools/overpass (VK), overpass.private.coffee (ex-kumi.systems), overpass.openstreetmap.ru. Regional instances exist.
- District-scale bbox queries comfortably within norms; ~800 ms spacing between bulk fetches; cache downloads locally. No formal ban system, just throttling/operator discretion.

## ODbL Licensing for Games

- Rendered game levels = **"Produced Work"** → attribution only. Acceptable placements per OSMF guidelines: splash screen, in-game view, credits, or menu; must identify OpenStreetMap + link openstreetmap.org/copyright.
- **Shipping pre-baked OSM-derived vector/geometric data risks "Derivative Database" classification** → share-alike exposure. Gray area (no OSMF ruling on baked meshes). Client-side generation keeps the database out of distribution.
- Precedent: Pokémon GO switched to OSM (2017) with in-app credit. Consider asking OSMF Licensing WG before shipping baked data if that path ever tempts us.

## Source Control (solo dev)

- Default recommendation: **Git + GitHub + LFS**, `.uasset/.umap` marked LFS + lockable; strict .gitignore (Binaries/, DerivedDataCache/, Saved/, intermediate). GitHub free tier: 1 GiB LFS storage/bandwidth-month — budget data packs (~$5/50 GB-mo) once real assets land.
- Alternatives if pain materializes: Perforce Helix Core free ≤5 users (best UE integration, admin overhead), Diversion, Anchorpoint.

## Zombie Horde AI

- Tiers: **BT+EQS** per-agent (workable ~50–100 agents w/ significance throttling) · **StateTree** (lighter; MoveTo overhead documented at 100+) · **Mass Entity/MassAI** (crowd-scale; navmesh experimental; C++ expected; replication awaits Iris work).
- Detour crowd avoidance default limit ~50 agents → enable avoidance near-player only, fake the rest.
- Realistic solo target: several hundred *visual* zombies (ISM/imposters/LOD tiers), **50–150 fully-simulated gameplay agents**. Resources: Epic "Designing Scalable Crowds with Mass AI" (Dec 2025), "Your First 60 Minutes with StateTree", Gianmarco Picarella's Zombie Horde Shooter sample.

## Co-op Networking Scope Check

- Standard path: UE replication + Online Subsystem (Steam) listen servers; EOS/OSS for crossplay later. Listen-server co-op avoids dedicated-server ops burden. *(Research answer truncated here — remaining detail unverified.)*
- Standing decision: **defer co-op until after Early Access**; keep simulation architecture server-authoritative-friendly so retrofit isn't a rewrite.

## Open Questions (follow-up research needed)

1. Comparable games' team-size/time-to-EA benchmarks (Valheim, PZ, V Rising…) — answers truncated in research session.
2. Current Steam Direct mechanics ($100 fee confirmation, Next Fest rules 2026, wishlist milestones).
3. Documented first-time-dev failure frameworks beyond generic advice.
