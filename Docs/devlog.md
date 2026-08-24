# Devlog — Hometown Zero

Per the roadmap's daily shutdown ritual: what moved, what broke, how fixed, tomorrow's first task. Retroactively seeded 2026-08-23 from session history; newer days append below.

## 2026-08-22
- Pipeline v0.2 → v0.3 (`Scripts/osm_pipeline.py`): `shop`/`amenity`/`office` POI joins, then `healthcare`/`craft`. Pike Place unknowns 139 → 91 of 219 buildings, join rate 331/342 nodes; single live Overpass fetch, etiquette held.
- Migration finished: spike assets relocated into the repo (`Scripts/`, `Docs/`), `phase0-spike/` deleted, sample outputs regenerated.
- Docs suite drafted: `game-design-doc.md`, `ue5-platform-research.md`, `dev-roadmap-4week.md`, `osm-parsing-libraries.md`.
- Broke twice: roadmap bytes briefly overwrote `osm_pipeline.py`, then clobbered `phase0-spike-spec.md`. Fixed: both rebuilt verbatim from session history; script proven via `python -m py_compile` plus a live rerun (219 buildings / 1,183 roads).
- Standing rule adopted (recorded in `AGENTS.md`): never overwrite existing files wholesale — new/dated files or targeted appends only.
- Tomorrow's first task: install Visual Studio + "Game development with C++"; take the first UE 5.8.1 compile.

## 2026-08-23
- Toolchain live: Visual Studio 18 Community + C++ workload installed; first full build green (`HometownZeroEditor` target).
- Git: repo initialized on `main` with LFS tracks, pushed to github.com/CatalystFish/HometownZero, commits through `29616c7`.
- Pike Place in-editor via Interchange glTF: color-coded category buildings, layered road meshes; player walks/climbs the real street grid.
- Collision on the imported city is currently complex-as-simple trimesh — it works, but it violates the box/convex-per-prism DoD rule; conversion still owed.
- First gameplay C++ compiled: `HZCharacter` (third-person walk, WASD/mouse/Space via legacy input ini) + `HZGameMode` replacing the engine DefaultPawn.
- Broke/fixed along the way: engine association (custom launcher `.bat`), bundled .NET 10 SDK environment, rocket-build failure from `BuildSettingsVersion.Latest` (environment mismatch), one-sided trimesh collision on roofs (triangle winding corrected), Landscape eye-icon hide persisting vs delete, DefaultPawn ignoring WASD/Space (it is a fly drone — hence the Character swap).
- In flight, unverified: runtime district loader (Path B), zombie chase AI, pipeline v0.5 (parallel overnight agents) — none count as done until Play-tested in-editor.
- Tomorrow's first task: run `Docs/thursday-brief.md`; verify overnight C++ in-editor before checking any Week 1 · Thu boxes.
