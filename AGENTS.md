# AGENTS.md — HometownZero

Unreal Engine 5 game repo root ("Hometown Zero": OSM-generated zombie survival). All spike and design assets are migrated into this folder; the parent `Default Project` directory holds only the workspace-level `AGENTS.md` context and an unrelated Home Assistant YAML.

## Layout

- `Config/` — UE config overrides
- `Source/HometownZero/` — runtime gameplay module (player, AI factions, inventory)
- `Source/HometownEditor/` — editor-side OSM ingestion modules (see its README for parser stack)
- `Scripts/` — Python tools for offline map parsing & testing; cached sample outputs in `samples/`
- `Docs/` — all research and design docs as Markdown; add dated notes here rather than chat-only

## Rules

- No `.uproject` exists yet — engine integration follows `Docs/phase0-spike-spec.md` phases until the spike passes go/no-go.
- Runtime code targets UE5 C++/Blueprints. Asset-prep tooling must be performance-critical Python or C++, favoring offline/batch generation over runtime generation.
- `district.json` schema `"version"` is a versioned contract between the Python pipeline and engine consumers — additive changes + version bump only.
- The 13 building categories are shared vocabulary across classifier and loot tables — change both together.
- Overpass etiquette applies to any new fetch code: identifying User-Agent, ~800 ms between bulk queries, cache downloads locally.
- **Never overwrite existing files wholesale** (owner mandate): create new versioned/dated files or make targeted appends; regenerate artifacts instead of hand-editing them.
