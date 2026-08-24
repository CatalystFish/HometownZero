# Hometown Zero — 4-Week Vertical Slice Roadmap

A part-time solo plan (~3-4 hrs Mon–Fri, 4-6 hrs Sat/Sun) that takes the existing `osm_pipeline.py` output to a packaged, externally playtested UE5.8 vertical slice. Days are labeled by weekday, never by date — when life happens, everything simply rolls forward.

## How to use this roadmap

- Work the checkboxes top to bottom. A slipped day pushes everything after it forward one day; each Sunday is the built-in buffer day. **Never compress Week 4 testing to recover lost time** — recover by cutting scope in earlier weeks instead.
- **Daily shutdown ritual:** before you stop, `git commit` (and push) whatever state the project is in — broken states included — then append one line to `Docs/devlog.md`: what moved, what broke, tomorrow's first task.
- **If behind, cut in this order:** (1) crafting recipes beyond the first one, (2) threat-escalation extras, (3) audio polish, (4) HUD flourishes beyond health/stamina/ammo, (5) visual polish passes, (6) second-district variety work.
- **Never cut, at any cost:** collision integrity (box/convex per prism, never complex-as-simple trimeshes), safehouse save/load, the playtest protocol, and the OSM attribution splash (license-required).
- The slice bar, straight from the spec: player crosses the district without falling through geometry; >=30 fps standalone on a mid-range GPU (record the actual number); >=70% of buildings classified non-unknown; 3 external testers complete the scripted tasks; the recognition quiz is answered ("can they identify the neighborhood?"); the go/no-go decision is documented.

## Week 1 — Foundation & First Import

Get UE5.8 running under Git+LFS, pull Pike Place in through Interchange, and walk streets that physically hold you up.

### Week 1 · Mon
- [x] Verify UE 5.8 install; create blank C++ project `HometownZero`
- [x] `git init`, add `.gitignore` plus LFS tracks for `*.uasset` and `*.umap`; create remote; first commit and push
- [x] Build folder skeleton: `/Content/Districts`, `/Content/Player`, `/Content/AI`, `/Source`

**Test:** clone the repo a second time locally and open it — zero LFS pointer-file errors, editor loads.

### Week 1 · Tue
- [x] Run `python osm_pipeline.py --bbox ... --out district.json --obj preview.obj` on the Pike Place bbox
- [x] Confirm `district.json` reports `"version": 2` and 219 buildings across the 13 categories
- [x] Import the OBJ through Interchange into `/Content/Districts`

**Test:** actor/mesh count in the level equals the building count printed by the pipeline (219).

### Week 1 · Wed
- [x] Enable auto-generated BOX/CONVEX collision on every extruded building prism; grep-import settings to confirm no complex-as-simple trimeshes remain
- [x] Add a basic player pawn with movement and a camera
- [ ] Walk the street grid end to end

**Test:** cross the full district without falling through any surface; every building wall blocks the capsule.

### Week 1 · Thu
- [ ] Convert per-category buildings to Hierarchical Instanced Static Meshes (one HISM component per category)
- [ ] Feed transforms from `district.json` so placement stays data-driven
- [ ] Record draw-call count before vs after instancing

**Test:** instanced instance counts sum to 219; draw calls drop measurably versus Wednesday's per-actor scene (note both numbers).

### Week 1 · Fri
- [ ] One base material per category with distinct tints; directional light plus skylight
- [ ] Save a reference screenshot to `Docs/`
- [ ] Commit pipeline outputs pattern: generated files stay in `Scripts/samples/`, never hand-edited

**Test:** screenshot shows visually distinguishable categories (medical vs food vs hardware) from a rooftop view.

### Week 1 · Sat
- [ ] Run a second, different neighborhood bbox through the full pipeline-to-editor path to prove repeatability
- [ ] Fix whatever breaks in the round-trip; script it, don't memorize it
- [ ] Add a startup splash screen carrying the OSM attribution text (license requirement)

**Test:** second district imports with zero manual mesh fixes; launching the game shows the attribution splash before gameplay.

### Week 1 · Sun
- [ ] Buffer: clear any rollover tasks from Mon–Sat before starting anything new
- [ ] Baseline profiling pass: capture `stat unit` and `stat gpu` standing at district center
- [ ] Write the numbers and known issues into `Docs/devlog.md`

**Test:** recorded baseline fps figure exists in the devlog; every Week 1 Definition of Done box below is checked.

### Definition of Done — Week 1
- [ ] Fresh clone opens cleanly with LFS; a commit exists for every working day
- [ ] 219-building district in-engine, counts matching `district.json`
- [ ] Full street-grid walk with zero fall-throughs; all collisions box/convex
- [ ] HISM instancing live per category with before/after draw-call numbers recorded
- [ ] OSM attribution splash appears at startup
- [ ] Tag the final commit `wk1-foundation` and push.

## Week 2 — District Integration & AI Chase

Put navmesh on the streets and up to 100 behavior-tree zombies between the player and the far side of town.

### Week 2 · Mon
- [ ] Place navmesh with bounds covering the street network only; exclude building footprints
- [ ] Cross-check covered streets against the road data in `district.json`
- [ ] Spawn a simple zombie capsule character that wanders

**Test:** navmesh debug view shows polys tracing the roads and none inside buildings; 10 wanderers navigate corners cleanly.

### Week 2 · Tue
- [ ] Build the Behavior Tree: idle wander -> investigate -> chase-on-sight, with Blackboard keys
- [ ] Add sight perception to the AI controller
- [ ] Give zombies a walk animation placeholder

**Test:** a zombie begins chasing inside 20 m line-of-sight and loses interest beyond 40 m or broken LOS.

### Week 2 · Wed
- [ ] Add an EQS query so chasers seek approach points around the player instead of single-file trailing
- [ ] Add contact-range melee with damage and knockback on the player
- [ ] Tune acceleration so crowds feel dangerous but escapable

**Test:** three zombies flank to surround the player; player takes damage on contact and can break away at a sprint.

### Week 2 · Thu
- [ ] Raise the population to 50 agents via spawner config
- [ ] Wire the Significance Manager: distant zombies drop animation and slow their BT ticks
- [ ] Profile frame time at 50

**Test:** 50 agents stay fully functional; toggling significance throttling produces a visible frame-time delta (record both numbers).

### Week 2 · Fri
- [ ] Push to the ~100 fully-simulated agent cap
- [ ] Tune perception and tick distances until stable
- [ ] Capture `stat unit`/`stat gpu` with all 100 active near the player

**Test:** scene holds >=30 fps projected for mid-range GPU with 100 agents converging; no zombie tunnels through a building to reach the player.

### Week 2 · Sat
- [ ] Playability pass: sprint, stamina drain, and whether escapes feel fair across the whole district
- [ ] Fix top three AI-stuck spots flagged during runs
- [ ] Start a running bug list in `Docs/known-issues.md`

**Test:** a chased player crosses the entire district with no zombie stuck longer than 2 seconds on geometry.

### Week 2 · Sun
- [ ] Buffer: clear rollovers first
- [ ] Re-record 50- and 100-agent frame times post-tuning; store screenshots in `Docs/`
- [ ] Review Week 3 scope against remaining energy; adjust task sizes now, not midweek

**Test:** devlog contains updated 100-agent fps number; all Week 2 DoD boxes checked.

### Definition of Done — Week 2
- [ ] 100 simulated zombies chase through streets without clipping buildings
- [ ] Significance throttling demonstrably reduces far-agent cost
- [ ] Frame times at 50 and 100 agents recorded in `Docs/`
- [ ] Player can be cornered, damaged, and legitimately escape
- [ ] Tag the final commit `wk2-ai-chase` and push.

## Week 3 — Core-Loop Alpha

Make the district worth crossing: search, loot, inventory, crafting, safehouse claiming, and noise that wakes the street.

### Week 3 · Mon
- [ ] Interaction trace with prompt ("E: Search") on container points placed inside buildings
- [ ] Generate loot from tables keyed to the 13 `district.json` categories
- [ ] Weight `unknown` low and verify classifier output feeds the tables

**Test:** searching a medical building returns medical-table items; hardware stores yield hardware; sampled unknown share stays <=30% of spawns.

### Week 3 · Tue
- [ ] Build inventory data component plus minimal grid UI
- [ ] Implement pickup, drop, stacking, and weight/capacity limits
- [ ] Reject over-capacity pickups with clear feedback

**Test:** filling past capacity blocks the pickup; stacks merge; dropped items reappear in-world.

### Week 3 · Wed
- [ ] Add a placeholder pistol with ammo drawn from `weapons_outdoors` loot
- [ ] Implement two crafting recipes (bandage from medical scraps, barricade planks from hardware)
- [ ] Craft menu lists requirements and grays out unaffordable recipes

**Test:** both recipes craft from looted ingredients and fail cleanly when short; pistol fires with ammo consumed.

### Week 3 · Thu
- [ ] Emit noise events: gunfire large radius, sprint medium, walk small
- [ ] Route noise into the zombies' BT investigate branch
- [ ] Balance radii against Week 2 chase speeds

**Test:** an unsuppressed shot draws investigators from ~60 m; a walking player passes within 10 m unnoticed.

### Week 3 · Fri
- [ ] Mark one building the claimable safehouse with an interactable claim point
- [ ] Implement claim flow plus door-barricade placement using crafted planks
- [ ] Save claimed state and barricades to disk; load on startup

**Test:** claim a safehouse, place a barricade, close and reopen the PIE/editor — ownership and barricades survive. This item is never cut.

### Week 3 · Sat
- [ ] Integrate the full loop: spawn -> search -> fight -> loot -> craft -> retreat -> claim
- [ ] Play the loop start to finish twice; list every friction point
- [ ] Fix the worst three friction points only

**Test:** one complete loop run finishes in under 20 minutes with zero designer hand-holding or console commands.

### Week 3 · Sun
- [ ] Buffer: clear rollovers first
- [ ] Draft the scripted playtest task sheet (find medical supplies, craft a bandage, claim the safehouse, survive one chase) plus the neighborhood-recognition quiz questions
- [ ] Dry-run the sheet yourself and time it

**Test:** the self-run completes all tasks within 45 minutes using only the written sheet; sheet saved to `Docs/playtest-script.md`.

### Definition of Done — Week 3
- [ ] Full loop completable: search, loot, fight, craft, retreat, claim
- [ ] Loot flows from category tables including `weapons_outdoors`; unknown share <=30%
- [ ] Inventory and at least one recipe work end to end
- [ ] Safehouse claim and barricades persist across a full editor restart (save/load proven)
- [ ] Playtest script and quiz drafted and self-tested
- [ ] Tag the final commit `wk3-core-loop` and push.

## Week 4 — Threat, HUD, Packaging, External Playtest & Go/No-Go

Turn the loop into a game, ship it in a box, hand it to strangers, and make the call.

### Week 4 · Mon
- [ ] HUD: health, stamina, ammo, interaction prompts, objective marker
- [ ] Minimal main menu and pause menu
- [ ] Pause halts simulation including AI ticks

**Test:** HUD is legible at 1080p in a moving firefight; pause freezes zombies mid-chase and resumes cleanly.

### Week 4 · Tue
- [ ] Add threat escalation: periodic horde spawn waves that intensify over session time
- [ ] Essentials-only audio: footsteps, groans, gunshot, UI blips
- [ ] Cut-list check: stop here even if tempted — features freeze after today

**Test:** a 15-minute session sees escalating pressure with at least two horde events; audio cues are positional (groan audible left vs right).

### Week 4 · Wed
- [ ] Package a Windows standalone build
- [ ] Bake the final OSM attribution splash into the shipped boot flow (menu-clickable credits too)
- [ ] Smoke-test the packaged exe cold

**Test:** package boots splash -> menu -> playable district with no missing-asset errors and no editor dependencies.

### Week 4 · Thu
- [ ] Triage crashes and top perf complaints from the package only — no feature changes
- [ ] Record standalone fps at a representative street spot with a horde inbound
- [ ] Repackage and declare the build locked

**Test:** locked build holds >=30 fps standalone on the mid-range reference GPU — write the actual measured number into `Docs/build-notes.md`; 30-minute soak run without a crash.

### Week 4 · Fri
- [ ] Brief all 3 external testers; distribute builds plus `playtest-script.md`
- [ ] Schedule observation slots for the weekend (remote or in person)
- [ ] Prepare a results template: task completion, deaths, confusion points, quiz answers

**Test:** all three testers confirm they have a working build and understand task one without asking questions.

### Week 4 · Sat
- [ ] Run all 3 sessions against the scripted tasks; observe silently, take notes, help only if someone is truly blocked
- [ ] Administer the recognition quiz afterward: "Which neighborhood is this, and how do you know?"
- [ ] Log everything into the results template same-day

**Test:** all 3 testers completed every scripted task; quiz answers captured verbatim for all three.

### Week 4 · Sun
- [ ] Compile results against the six success criteria; write `Docs/go-no-go.md` with the decision and reasons
- [ ] Hold the retrospective: what to keep, change, kill before the next phase
- [ ] Update the Idea Parking Lot below with anything testers begged for
- [ ] Celebrate honestly — four weeks of nights and weekends shipped

**Test:** `go-no-go.md` exists, cites the criteria evidence (fps number, tester count, quiz result), names the decision, and the commit is tagged.

### Definition of Done — Week 4
- [ ] Locked packaged build boots attribution splash -> menu -> district, no editor needed
- [ ] Actual standalone fps number recorded in `Docs/build-notes.md` meeting the >=30 fps bar or documenting why not
- [ ] 3 external testers completed all scripted tasks; notes filed
- [ ] Recognition quiz results captured and assessed
- [ ] Go/no-go decision documented in `Docs/go-no-go.md`
- [ ] Tag the final commit `slice-rc1` and push.

## Idea Parking Lot

Every good idea that arrives mid-sprint goes here instead of into a task list. Review only at phase boundaries.

| Idea | Temptation level | Deferred until |
| --- | --- | --- |
| Driveable vehicles on the street grid | Brutal | After the go decision; requires traffic lanes and AI dodge work |
| Enterable interiors for every building | Extreme | Post-slice; only the safehouse earns an interior this phase |
| Online co-op | Extreme | After slice approval; needs a dedicated networking spike first |
| Weather and day/night cycle | High | Atmosphere pass after the slice, if perf budget allows |
| Dogs as companions or enemies | Medium | After the core loop is validated with human testers |
| Steam Workshop mod support | Low | Post-1.0; the `district.json` contract comes first |
| Nanite/Lumen graphics upgrade pass | High | Only after playtest data proves headroom on the mid-range GPU |
| Shotgun, rifle, and melee arsenal | High | After balancing data exists for the single-pistol loop |
