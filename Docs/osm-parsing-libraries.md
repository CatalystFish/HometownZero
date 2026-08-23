# OSM Parsing Libraries — Research Notes

*Researched Aug 2026. Question: lightweight C++/Python libraries for parsing OpenStreetMap (.osm XML / .pbf) into raw 2D building footprints, executable client-side inside a UE5 plugin without external runtime dependencies.*

## The three viable stacks

| Approach | Deps | License | Best for |
|---|---|---|---|
| **UE-native `FastXml::Parse`** (SAX streaming) | none — ships with engine | engine | `.osm` XML at district scale |
| **protozero** v1.8.2 (header-only, no protoc) | + zlib (engine has one) | BSD-2 | `.pbf` regional extracts |
| **libosmium** v2.23.1 (header-only) | Expat + threads + exceptions | BSL-1.0 | full processing (overkill here) |

## Key findings

- **Canonical precedent:** Epic TD Mike Fricker's **StreetMap plugin** (MIT, last push May 2024) parses `.osm` XML via hand-written FSM on `IFastXmlCallback` — zero external parser libs. MIT reference code. Limits: XML only, >2 GB crash, single-quote attributes unsupported.
- **PBF vs XML:** PBF ≈ half the size of gzipped XML, ~6× faster to read (planet-scale figures; small extracts unverified). PBF is the format for offline Geofabrik regional extracts.
- **Avoid OSMPBF/OSM-binary**: LGPL-3.0 + Google protobuf runtime — only licensing landmine found. BSL/MIT/BSD/ISC/Zlib all static-link cleanly into UE plugins.
- **Geometry step:** hand-rolled ray-cast point-in-polygon + shoelace area (already implemented in Python pipeline), plus mapbox `earcut.hpp` (ISC, single header). UE built-ins exist too: `FGeomTools::TriangulatePoly`, GeometryCore Delaunay.
- **UE module gotchas:** exceptions/RTTI off by default → set `bEnableExceptions = true`, `bUseRTTI = true` per-module Build.cs (protozero/libosmium throw); vendor headers under `Source/ThirdParty/<lib>/` inside `THIRD_PARTY_INCLUDES_START/END`; isolate parsers in a low-level module with no UObject headers; Linux forbids mixing RTTI across modules.
- **Python side:** pyosmium (`osmium` on PyPI) 4.3.1, BSD-2, actively maintained; wheels bundle libosmium. Use for reproducible pinned regional snapshots during batch asset prep. Overpass stays better for ad-hoc bbox fetches.
- No UE version through 5.8 ingests OSM natively. PCG production-ready since 5.7 but consumes DataTables/Alembic, not geo formats. libosmium requires exceptions (`bEnableExceptions`) — conflicts with UE defaults if used naively.

## Recommended ladder

1. **Now (district scale):** Overpass API → JSON → UE `FJsonSerializer`. Zero new dependencies.
2. **Offline XML (slice phase):** StreetMap-style `FastXml` FSM parser; crib MIT reference code.
3. **Regional extracts (post-EA tooling):** vendor protozero (+ zlib) for PBF.

## Sources checked

docs.osmcode.org, OSM wiki (PBF format, Overpass etiquette), protozero GitHub releases, StreetMap repo (github.com/ue4plugins/streetmap), Epic docs on module Build.cs flags, pyosmium PyPI/changelog, mapbox earcut.hpp.
