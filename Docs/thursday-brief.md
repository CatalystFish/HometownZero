# Thursday Brief — Week 1 · Thu (morning test plan)

**Read this first:** overnight agents were mid-flight on three things — the runtime district loader (Path B C++ reading `district.json` at play time), zombie chase AI, and pipeline v0.5. **None of it counts as done until you press Play and see it yourself.** Unverified-in-editor means not done. If their code does not compile or crashes, jump to the Fallback at the bottom and run the original roadmap Thursday instead.

## When you wake up

1. **Read the newest morning report in `Docs/`** (`morning-report-YYYY-MM-DD.md`). Note what the overnight agents claim shipped and anything they flagged as broken or unfinished.
2. **Open the project.**
   - Launch via the custom launcher `.bat` (the engine-association fix), not a stale shortcut.
   - Open `HometownZero.uproject`. If prompted to rebuild modules, accept and wait.
   - Clean rebuild = continue. Compile error on open = the overnight C++ is dead — go to Fallback now.
3. **Test in this order. Do not skip ahead.**

### Step 1 — Character still walks

Press Play. Confirm third-person `HZCharacter`: WASD moves, mouse turns camera, Space jumps. If you are back to floating camera-drone movement, `HZGameMode` regressed — record it and keep testing anyway.

### Step 2 — District spawns from JSON vs imported mesh

Look around the level. Two city sources can now exist side by side:

- the imported glTF city (static meshes, color-coded buildings, layered roads), and
- runtime-spawned buildings generated from `district.json`.

**Both may be present and overlapping** — double walls, z-fighting. Overlap itself is expected, not a bug yet.

- If overlap stops you judging anything: find the imported mesh actor(s) in the World Outliner, click the eye icon to **hide** (do not delete), then re-Play. The runtime boxes should stand alone.
- Compare shapes and category colors against `Scripts/samples/pike_place.*` previews.
- Walk into a few runtime buildings: walls must block the capsule; no falling through floors or roofs.

### Step 3 — Zombies (only if present)

If chase AI shipped: approach one slowly. Chase should start within short line-of-sight and drop when you break distance or break line-of-sight. Stuck-on-geometry, instant aggro through walls, or sliding with no animation are recordable findings, not surprises.

### Step 4 — Record numbers

Standing still at district center with the scene settled:

- Console command `stat unit` — write down Frame / Game / GPU milliseconds.
- Output Log: search for the loader's spawn lines (component/instance counts). Write down the totals; they should reconcile roughly against the pipeline printout of **219 buildings**.

Record raw numbers in `Docs/devlog.md` even if tests failed — negative data is data.

## Pass vs fail

| Signal | Verdict |
|---|---|
| Rebuild succeeds; Play runs; character walks | Baseline holds |
| Runtime buildings spawn from JSON and hold the capsule | Loader works |
| Log counts ≈ 219; frame ms playable at street level | Numbers banked |
| Compile error, crash on Play, empty district, fall-throughs | Fallback |

One hard fail (compile error / crash) sends you to Fallback. Soft fails (overlap, wrong counts, ugly visuals) get logged and fixed after Thursday.

## Fallback — original Week 1 · Thu tasks

If the overnight C++ did not survive contact with the editor, revert to the roadmap's original three boxes using the imported glTF city as the working scene:

1. Convert per-category buildings to Hierarchical Instanced Static Meshes (one HISM component per category).
2. Feed transforms from `district.json` so placement stays data-driven.
3. Record draw-call count before vs after instancing.

Test: instanced counts sum to 219; draw calls drop measurably versus Wednesday's per-actor scene. Write both numbers into the devlog.

## Shutdown ritual

Commit and push whatever state you ended in — broken states included — then append today's devlog entry: what moved, what broke, tomorrow's first task.
