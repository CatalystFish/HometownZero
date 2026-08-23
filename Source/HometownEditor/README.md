# HometownEditor (editor module)

Editor/client-side pipeline for OSM ingestion: parsing `.osm` XML / Overpass JSON into building footprints, classification, and district asset generation.

Planned parser stack (see `../../Docs/osm-parsing-libraries.md`):
1. Overpass JSON via UE `FJsonSerializer` — zero dependencies
2. Offline XML via `FastXml::Parse` FSM (StreetMap-style, MIT reference)
3. Regional PBF later: vendored protozero (BSD-2) + zlib, isolated low-level module with `bEnableExceptions = true`
