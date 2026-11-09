# SweetSandy Package

Straightforward off-road driving demo: a softly-suspended vehicle on a
heightmap-based terrain.

The name is a nod to GTA San Andreas's Sandking (the game's softest-
suspension off-roader) and to the "Sandy"-style off-road maps in Multi
Theft Auto (weaving between trees, jumping rocks, crossing rivers).
"Sweet" is a compliment aimed at that car. The default vehicle in
`Catalog.cpp` is named `SandDummy` accordingly.

- Phase 1 (current, implemented): `uppsrc/Core`, `CtrlCore`/`CtrlLib`,
  `GLCtrl` (see `reference/OpenGL`) and the system ODE library
  (`<ode/ode.h>`) only. No `uppsrc/Eon`, `uppsrc/api/Graphics`/`Physics`, or
  `uppsrc/Geometry` yet - those pull in a much heavier dependency chain
  (AI/ByteVM/assimp/Eigen) than this phase needs.
- Phase 2 (later, not started): migrate toward the `uppsrc/api/Graphics` +
  `uppsrc/api/Physics` + `uppsrc/Eon` stack that `upptst/Eon*` demonstrates.
- `SweetSandy.h` is the package main header; it wraps the other headers in
  `NAMESPACE_UPP`/`END_UPP_NAMESPACE` (same pattern as `game/Hearts`), so
  every `.cpp` re-opens `NAMESPACE_UPP` for its implementation.
- Screen flow: `AppShell` (`TopWindow`) owns one `TabCtrl` with its header
  hidden (`tabheight = 0`) for screen switching - `MainMenuScreen` ->
  `NewGameScreen` (track/vehicle pick, see `Catalog.h`) -> `PlayScreen`, plus
  `SettingsScreen` (key rebinding, see below) and `PlaceholderScreen`
  stand-ins for Multiplayer/Credits/Tutorial. Per `ai/rules/gui-user-gate.md`,
  TabCtrl is used instead of `Ctrl::Show`/`Hide`.
  - Gotcha: `TabCtrl::SetStyle` stores a pointer to the `Style` you pass it -
    it must have static storage duration (see `AppShell::AppShell`), not be
    a constructor-local variable.
  - Gotcha: `Ctrl::SetTimeCallback(delay, cb, id)`'s `id` is **not** an
    arbitrary integer key - `Ctrl::InitTimer` casts it to `(byte*)this + id`
    and asserts it stays under `sizeof(Ctrl)`. Small sequential values
    (0, 1, 2, ...) are always safe; passing something like 100+i (as a
    first draft of `--menutest` did) trips
    `uppsrc/CtrlCore/CtrlTimer.cpp`'s assert.
  - Gotcha (the big one - caused a fully blank window): `TabCtrl::Item::Slave()`
    adds the child via plain `Ctrl::Add(Ctrl&)`, which does **not** size it -
    a freshly constructed `Ctrl` starts at `rect(0,0,0,0)`
    (`uppsrc/CtrlCore/Ctrl.cpp:Ctrl::Ctrl()`). `AppShell::AddScreen` must
    call `ctrl.SizePos()` before `shell.Add(ctrl, tab_text)` - the pattern
    already exists elsewhere in the repo at
    `uppsrc/CtrlLib/TabCtrl.cpp:TabDlg::Add0`. Every screen was 0x0 and
    painted nothing until this was added.
- `Terrain` (`Terrain.h/.cpp`) loads/generates a heightmap and is the single
  source of truth for both `DrawGL()` and `BuildOdeHeightfield()`. It owns
  the ODE `dHeightfieldDataID` and frees it in its destructor -
  `dGeomHeightfieldDataDestroy` is not automatic when the geom is destroyed.
  - Gotcha (the big physics one - "car explodes on the ground, some wheels
    sink through"): a placeable ODE heightfield geom is **already centered
    on its own position by default** - grid index (0,0) sits at
    `geom_pos - (width/2, depth/2)`, not at `geom_pos` as the header docs'
    "local axis spans [0,width]" phrasing suggests (confirmed empirically
    with `dGeomGetAABB`, see `RunRampHeightfieldTest` in `main.cpp`).
    `BuildOdeHeightfield` used to also call
    `dGeomSetPosition(geom, -world_size/2, 0, -world_size/2)`, double-
    centering it. For the real 200m terrain that put the actual collision
    surface 100m away from where `Terrain::HeightAt()`/`DrawGL()` (and thus
    where the vehicle spawns) think the ground is - each wheel ended up
    colliding against a effectively-uncorrelated patch of terrain, which
    reads exactly like "some wheels stay up, some fall through" before the
    whole thing goes unstable. Fix: don't call `dGeomSetPosition` on the
    heightfield geom at all - the default position already matches this
    package's origin-centered world convention.
- `Vehicle` (`Vehicle.h/.cpp`) is a plain ODE-C-API box-chassis +
  4-cylinder-wheel `dJointCreateHinge2` rig (steering on axis1, spin on
  axis2, soft suspension via `dParamSuspensionERP`/`CFM` on the axis1
  anchor - the classic ODE "buggy" trick). It does not own the ODE
  world/space.
  - Gotcha (the flip-over-under-throttle incident): each wheel's
    `dParamFMax2`/`dParamVel2` drive motor reacts equally and oppositely
    onto the chassis (Newton's third law) about the wheels' *shared* spin
    axis (world X = pitch) - and the motor holds near-max torque for the
    whole spin-up to `max_wheel_spin`, not just an instant kick, so all 4
    wheels sustain a pitch torque together. Enough of that pitches the
    chassis into a backward flip ("wheelie" taken too far). Two changes
    fixed it: `drive_force` came down a lot (3500 -> 80 -> 30, see
    `Catalog.h`), and `Vehicle::Step` now slew-rate-limits the *applied*
    throttle (`throttle_ramped`, ~0.4-0.8s to reach the target) instead of
    slamming the motor target on in one step. Diagnosed and verified with
    `--geomtest`'s `RunVehicleTerrainSelfTest` (added an `upright` metric:
    the chassis's own up-axis Y-component in world space via
    `dBodyGetRotation`, 1.0 = level) and `RunObstacleCourseSelfTest`. Both
    parameters interact with terrain roughness (a bumpy or ramped surface
    needs more margin than flat ground) - re-run `--geomtest` after
    touching either one.
  - Tuning pass ("0-60mph 10-15s, less tippy, 20% longer"): `drive_force`
    went back up to 80 (the earlier `30` was for stability margin on the
    original 2.6m-long, 1.4m-ride-height chassis) once `chassis_size_z`
    grew to 3.12m (+20%, more pitch inertia) and `ride_height` dropped to
    1.1m (-21%, lower center of gravity, less "kiikkerä"/tippy) gave enough
    stability margin back. `max_wheel_spin` also had to go from 40 to 60
    rad/s - at the old value the wheels' own top speed (22 m/s / 49mph)
    made 60mph physically unreachable no matter how much torque was
    available. `--geomtest`'s `RunAccelerationTest` (flat ground, full
    throttle, times 0 -> 26.82 m/s) is the check for this: 13.7s at
    `drive_force = 80` with the current chassis dimensions.
  - Second tuning pass ("2x kiihtyvyys, vielä matalampi/kiikkerämpi"):
    `drive_force` 80 -> 160 (2x -> 5.87s 0-60mph), `wheel_radius` 0.55 ->
    0.45, `ride_height` 0.85 (down from 1.1, -39% from the original 1.4).
    This reintroduced a *different* rollover mode: hard cornering at speed
    (not the earlier straight-line pitch flip) - a lower/faster car with
    unchanged full-lock steering rolled the chassis over within ~1.5s of a
    40km/h hard turn. Fixed with **speed- and stability-sensitive steering**
    in `Vehicle::Step` (per the tuning request: "renkaat voi kääntyä
    vähemmän kovassa vauhdissa" + "kääntyvyyttä voidaan vähentää dynaamisesti
    kiikkeryyden perusteella" - both, not either/or):
    - Speed: `max_steer_angle` is scaled by a smoothstep factor from 1.0
      below `low_speed_full_steer_mps` down to `steer_high_speed_fraction`
      at/above `high_speed_reduced_steer_mps` (`Catalog.h`).
    - Stability: further scaled by a second smoothstep factor based on the
      chassis's current `upright` value (see below) - starts cutting at
      `stability_steer_upright_full`, reaches a floor (never fully zero) at
      `stability_steer_upright_zero`. This is the *reactive* half: an
      already-leaning car gets less steering authority, so a held full-lock
      input can't drive it the rest of the way into a rollover.
    - Both factors multiply, and **update `dParamLoStop`/`HiStop` every
      step**, not just the velocity-servo target - a purely-servo trim
      without also narrowing the hard mechanical limit doesn't actually cap
      the wheel's achievable angle. Verified with `--geomtest`'s
      `RunTurningTest` (40km/h, full lock, 8s - `upright` stays ~1.0
      throughout with the tuned curves; first attempt with a gentler curve
      only delayed the rollover to ~2.9s, confirming the cut needed to be
      much more aggressive, not just present).
  - Third tuning pass ("runko liian korkealla, nopea kääntyminen liian
    jäykkä"): `ride_height` 0.85 -> 0.68 (-51% from the original 1.4) for
    an even lower center of gravity, which bought back enough stability
    margin to loosen the high-speed steering cut that felt too restrictive:
    `steer_high_speed_fraction` 0.18 -> 0.35 (roughly double the turning
    authority at speed), `low_speed_full_steer_mps`/`high_speed_reduced_steer_mps`
    widened to 4.0/11.0 m/s. `RunTurningTest` still holds `upright` at
    0.997-0.999 for the full 8s at 40km/h with these looser curves - the
    lower ride height did the work, not a tighter steering clamp.
  - Two-part body ("korissa pitäisi olla kaksi osaa... eri värillisiksi"):
    the chassis is one rigid body but two visual/mass/collision *parts* - a
    heavy, thin structural tub (part 1, `chassis_part1_height`) and a
    light shell on top (part 2, `chassis_part2_height` - independent
    values now, not fractions of a shared total, per "painava osa
    vieläkin paljon matalammalla, kuoriosa ainakin 2x nykyiseen
    korkeuteen"). **The chassis body's local origin is part 1's bottom**,
    not a box center like the old single-box chassis - `ride_height` is
    therefore now directly the car's ground clearance (wheel-hub to
    body-bottom), not a center height; re-derive it if you change the
    part heights. `Vehicle::Create` combines both parts' mass into the one
    body with `dMassSetBoxTotal` + `dMassTranslate` + `dMassAdd` (exact for
    two pieces with no relative motion - simpler and safer than a second
    body on a fixed joint), and gives **both** parts their own collision
    geom (each `dGeomSetOffsetPosition`-offset from the body origin) -
    part 2 used to be decorative-only and passed through the terrain on a
    rollover ("kuoriosa oli kuin ilmaa, ettei se törmännyt maahan"); now a
    box approximation of its shape collides too. `PlayScreen::GLPaint`
    draws both from the same chassis transform with different colors;
    part 2 is drawn by `DrawWedgeShell` (in `PlayScreen.cpp`) - a low nose
    at the front (+z, matching the front wheels' `hub_offset` in
    `Vehicle::Create`) rising to a taller cabin/roof at the back, "vähän
    kiilamainen ja aerodynaaminen" per the request, not a plain box (its
    collision geom is still a plain box, though - only the drawn shape is
    wedged, matching how e.g. wheel collision uses a cylinder while the
    car overall silhouette is fancier elsewhere too).
  - Brakes: `dParamFMax2` while braking uses a dedicated `brake_force`
    (default `drive_force * 1.33`, declared after `drive_force` in
    `Catalog.h` so the member initializer can read it - "jarrut voisi olla
    33% tehokkaammat myös") instead of reusing `drive_force` directly.
- `PhysicsWorld` (`PhysicsWorld.h/.cpp`) is the shared
  collide+contacts+step helper, used by both the headless selftest and
  `PlayScreen`.
- Gotcha: destroy anything holding live ODE objects (`AppShell`, via
  `PlayScreen`/`PhysicsWorld`) before calling `dCloseODE()` - see the nested
  `{ AppShell win; ... }` blocks in `main.cpp`. Destructing after
  `dCloseODE()` segfaults inside `~dxWorld`.
- `bin/SweetSandy --selftest` runs headless Terrain/Vehicle checks (no
  window). `bin/SweetSandy --playtest` opens the real window, auto-starts a
  run, pumps the GUI loop for a few seconds, and closes - a smoke test for
  GLPaint/Tick that the pure selftest can't reach, but it only checks for
  crashes, not for what actually got drawn.
- `GLDebug.h/.cpp` (`GLReadFrameStats`, `GLLogProjectedPoint`): read the
  framebuffer back with `glReadPixels` and report background-vs-object pixel
  percentage, and `gluProject` a world point and log its screen xy/depth and
  whether it lands inside the viewport. Call only from inside a current GL
  context (i.e. from `GLPaint`, after drawing, before the buffer swap).
  `PlayScreen::GLPaint` logs this for its first 3 frames of every run.
  Use these whenever "nothing renders" is reported again - they turn it into
  numbers instead of a screenshot.
- `bin/SweetSandy --gltest` (`GLDebugQuad`): one quad, one fixed camera, one
  isolated window - checks basic GL (viewport/projection/clear/draw/readback)
  independent of the much more complex `PlayScreen` scene. Run this first
  when rendering looks wrong, before suspecting the real scene.
- `bin/SweetSandy --geomtest`: physics equivalent of `--gltest` - isolated
  `dCollide()` checks against a flat heightfield, a hand-computable "ramp"
  heightfield (used to find the centering bug above), a shape comparison
  (cylinder/capsule/sphere/box wheel geoms vs `Terrain`'s real heightfield),
  and a full `Vehicle` + `Terrain` dynamics run with per-wheel position/
  speed and chassis angular-velocity logging, plus `RunAccelerationTest`
  (flat ground, full throttle, 0 -> 60mph timing), `RunTurningTest` (flat
  ground, accelerate to 40km/h then hold full steering lock for 8s - the
  cornering/rollover check, distinct from the straight-line pitch-flip
  `RunVehicleTerrainSelfTest` catches), and `RunObstacleCourseSelfTest`.
  `--selftest` also runs the dynamics part with real pass/fail assertions
  (angular velocity bounded, chassis never falls below the terrain, settles
  within 5s, stays upright through both straight-line and cornering
  scenarios, 0-60mph within the current tuning target) - run this whenever
  suspension/contact/acceleration/steering behavior is
  suspect.
- `bin/SweetSandy --play [track_id] [vehicle_id]`: skips the menu and
  starts driving immediately (`AppShell::StartRunById`, also used by
  `--playtest`/`AppShell::StartDefaultRun`). With no arguments, uses the
  first catalog entries. Try `--play obstacle_course sand_dummy`.
- `Catalog.cpp` has two tracks: `default` (`TERRAIN_ROLLING`, the original
  gentle bumps) and `obstacle_course` (`TERRAIN_OBSTACLE_COURSE`) - a
  hand-laid-out course along +z (two bumps, then a launch ramp into a real
  jump with air time, a landing pit, and a climb back out), generated by
  `ObstacleCourseHeight()` in `Terrain.cpp`. `Terrain::Load` dispatches on
  `TrackDef::kind`; height is a function of world z only (full-width ramps,
  same across x), matching "drive straight ahead over jumps and height
  differences." `RunObstacleCourseSelfTest` in `main.cpp` drives it at full
  throttle and checks the car stays upright, actually gets airborne over
  the jump (clearance above the ground actually below the chassis, not
  just resting height), and reaches the far end.
- `Settings.h/.cpp` + `SettingsScreen.h/.cpp`: configurable driving keys
  ("asetuksissa pitäisi olla myös konfiguroitavat pelinäppäimet"). One key
  per action (throttle/brake/steer_left/steer_right/back) - not "arrows OR
  wasd" like the old hardcoded `PlayScreen::Key` - defaulting to the
  original arrow-key layout. `GetKeyBindings()` lazily loads from
  `AppRegistry` (see `ai/skills/app-registry.md`) and caches in memory;
  `SaveKeyBindings()` persists after every rebind. `SettingsScreen`: click
  a row's button ("(paina nappainta...)"), then press the key you want -
  captured via the screen's own `Key()` override, which only fires once
  focus has been moved off the clicked button back to the screen itself
  (`StartCapture` calls `SetFocus()` for this). Esc while capturing cancels
  instead of binding Esc. `PlayScreen::Key` now looks up `GetKeyBindings()`
  instead of hardcoded key literals.
- `bin/SweetSandy --menutest`: cycles through every top-level screen
  (menu/new game/settings/multiplayer/credits/tutorial) briefly - added
  when `SettingsScreen` was introduced, since `--playtest`/`--play` only
  ever visit menu -> play and never exercise the other screens'
  construction/layout/paint path. This is also where the
  `Ctrl::SetTimeCallback` `id`-is-a-byte-offset gotcha above was found.
- `bin/SweetSandy --selftest`'s `RunSettingsSelfTest` verifies the key-rebind
  AppRegistry round-trip with a *separate* `AppRegistry` instance (same
  Vendor/AppId/Profile) reading back what `SaveKeyBindings()` wrote - proves
  disk persistence, not just the in-process cache.
- `WorldChunk.h`/`Octree.h`/`Octree.cpp`: World Builder phase, Task 1 - a
  chunk spatial index (`WorldChunkCoord`/`WorldChunk`) with distance-based
  LOD streaming (`WorldOctree::UpdateStreaming`, see the plan under
  `Manager/2-plan/.../002-world-builder/`). Verified headless by
  `RunOctreeSelfTest` in `main.cpp` (wired into `--selftest`): chunks load
  near the viewer, LOD assignment matches distance, a stationary viewer
  causes zero load/unload thrashing, and moving far away unloads the old
  chunks while keeping the loaded count bounded.
  - Gotcha (ODR collision, not a logic bug - cost the most debugging time
    of this class): the class was first named `Octree`, which silently
    collides with the unrelated `Upp::Octree` in
    `uppsrc/Geometry/Octree.h` - a completely different class, same name,
    same `Upp` namespace, linked in transitively (SweetSandy depends on
    GLCtrl -> ... -> Geometry). Two same-named classes with different
    layouts is an ODR violation: it compiled and linked without a single
    warning, but at runtime a call could bind to *either* definition's
    method depending on which translation unit's copy the linker kept,
    reading fields at the wrong offsets. Symptom was a SIGSEGV inside
    `SetLodLevels` with no assertion firing - `gdb`'s backtrace showed a
    `this` pointer holding an obviously-a-double bit pattern (one of the
    `LodLevel::cell_size` values, ~80.0) being dereferenced as a pointer,
    i.e. some caller had passed a `double` where a class's `this` was
    expected - a calling-convention mismatch consistent with two
    different-layout classes sharing one mangled name. `gdb -batch -ex run
    -ex bt` initially showed only `??` frames (stripped/no debug info
    resolved for the app binary in this build), but `info symbol $pc` +
    disassembling around the fault address was enough to see the shape of
    the crash (a trivial getter dereferencing a garbage `this`) even
    without symbols. Confirmed the cause with
    `grep -rn "class Octree\|struct Octree" uppsrc/` before renaming.
    Fixed by renaming the class to `WorldOctree` (kept the file names
    `Octree.h`/`Octree.cpp` since those don't collide - only C++ symbol
    names are namespace-global). Lesson: grep the tree for an exact class
    name before introducing it, especially short/generic ones
    (`Octree`, `Node`, `Grid`, ...) - BLITZ and the shared `Upp` namespace
    make same-named classes from unrelated packages link together instead
    of erroring.
- `uppsrc/Core/DiskBlobCache.h/.cpp` + `WorldChunkCache.h/.cpp`: World
  Builder phase, Task 2 (disk-backed chunk cache, "mielellään eri LOD
  blokit olisi cachettu kovalevylle"). Split deliberately across two
  layers, per "sijoita yleishyödyllinen koodi uppsrc/*-paketteihin":
  - `Core::DiskBlobCache` (in `uppsrc/Core/`, wired into `Core.upp`/
    `Core.h` right after `AppRegistry`) is a generic key -> blob disk
    cache with size-bounded LRU eviction - no notion of chunks, LOD, or
    anything game-specific. Any future package needing "cache some blobs
    on disk, evict the least-recently-used ones under a byte budget" can
    use it directly instead of re-deriving this. LRU is tracked via each
    file's mtime (refreshed on both `Load` and `Save`) rather than a
    separate index file - the filesystem already does that bookkeeping.
    `EvictLru`/`GetTotalSize` walk the cache directory recursively via
    `FindFile` (see `uppsrc/aimk/aimake.cpp` for the same iteration
    pattern) - fine at this task's scale, revisit only if profiling shows
    it matters.
  - `WorldChunkCache` (in `game/SweetSandy/`) is a thin, game-specific
    wrapper: it only knows how to turn a `WorldChunkCoord` + a
    `seed_version` string into a `DiskBlobCache` key
    (`<lod>/<x>_<y>_<z>.bin`) and where the root lives
    (`AppRegistry::GetCacheDir()/world/<seed_version>/`, reusing the same
    Vendor/AppId/Profile identity `Settings.cpp` already established for
    this app). A different `seed_version` is a different cache root, so a
    world-seed or generator-algorithm change auto-invalidates old cached
    content instead of reading it as if it were still valid.
  - Verified headless by `RunWorldChunkCacheSelfTest` in `main.cpp`
    (wired into `--selftest`): byte-identical read-back, LRU eviction
    removes the oldest entry first and stops once the byte budget is met,
    and a different `seed_version` cannot see another version's data.
  - Gotcha found while writing the test itself, not the cache code: the
    first draft called `Load()` on all three test chunks (to check
    byte-identical read-back) *before* calling `EvictLru`, expecting the
    first-saved chunk to still be the oldest and therefore get evicted
    first. But `Load()` intentionally touches mtime (that's what makes it
    real LRU instead of "least-recently-written") - so those verification
    reads silently re-ordered the very mtimes the next assertion depended
    on, and the wrong entry got evicted (`EvictLru freed=200` instead of
    the expected `100`). Fixed by moving the byte-identity check to a
    disposable throwaway key, and never calling `Load()` on the
    LRU-order test's own keys until *after* the eviction being tested has
    already happened. General lesson: in any test of an LRU cache, reads
    are not read-only - don't verify one property of the API using calls
    that silently perturb the property you're testing next.
  - Also needed `Sleep(1100)` between the three test saves: mtime-based
    LRU needs distinguishable mtimes, and typical POSIX filesystems (and
    `Upp::Time`) resolve to whole seconds, so two saves inside the same
    second would tie.
- `WorldGenStages.h/.cpp` + `WorldBuilder.h/.cpp`: World Builder phase,
  Task 3 - wires Task 1's `WorldOctree` and Task 2's `WorldChunkCache` to
  a pluggable generator-stage interface (`ICityLayoutGenerator`), proving
  the streaming pipeline's shape end-to-end before any real generator
  algorithm exists. `WorldBuilder::Update(viewer_pos)`: call
  `WorldOctree::UpdateStreaming`, drop content for chunks that just
  unloaded, and for chunks that are loaded but don't have content yet,
  try `WorldChunkCache::Load` first and only call `Generate()` on a
  cache miss (then `Save()` the result). Stayed within `game/SweetSandy/`
  rather than `uppsrc/` - unlike Task 2's `DiskBlobCache`, nothing here
  is generic: the types (`WorldChunk`, `WorldChunkContent`,
  `ICityLayoutGenerator`) are all world-builder domain concepts, and with
  only one stage interface and stub content, generalizing the "cache-or-
  generate on chunk load" pattern into a reusable template now would be
  the speculative abstraction the project's conventions warn against -
  revisit once a second generator stage (road mesh, vegetation) exists
  and the real shared shape is known, not before.
  - Per the task doc, `IRoadMeshGenerator`/`IVegetationGenerator` are
    *not* stubbed out yet - they have no caller until their own phases
    start.
  - `StubCityLayoutGenerator` (test-only, lives in `main.cpp` next to
    `RunWorldBuilderSelfTest`, not shipped as a real stage): marks
    even/even grid-coordinate chunks as `has_city = true` and counts its
    own `Generate()` calls, which is what lets the self-test prove a
    reloaded chunk came from the cache instead of regenerating - assert
    the call count is unchanged after a chunk unloads and reloads.
  - `WorldBuilder::Create` currently hardcodes the same 3-level LOD
    scheme `WorldOctree`'s own self-test uses (cell 20/dist 60, cell
    40/dist 150, cell 80/dist 400) - real thresholds depend on real
    content cost/size, unknown until a real generator exists.
  - Verified headless by `RunWorldBuilderSelfTest` in `main.cpp` (wired
    into `--selftest`): content matches the stub's deterministic rule for
    every loaded chunk; a chunk that unloads then reloads gets its
    content back with zero additional `Generate()` calls (cache hit,
    proven via the counter); and a ~300-step wandering viewer path
    (sin/cos Lissajous-ish walk, chosen so both x and z cross zero
    repeatedly and force lots of load/unload churn) runs with no crash.
- `Vec2` (`SandyMath.h`) + `CityTensor.h/.cpp` + `CityBasisField.h/.cpp`
  + `CityTensorField.h/.cpp`: City Layout phase, Task 1 - the tensor-
  field foundation for the MapGenerator-derived road-network generator
  (see `Manager/2-plan/.../003-city-layout/001-planning.md` for the
  full algorithm summary, read from MapGenerator's actual source, not
  just its README). `CityTensor` is the "line field" (undirected
  direction) tensor via the standard double-angle trick; `CityGridField`/
  `CityRadialField` are named basis fields with a distance-based falloff;
  `CityTensorField` sums weighted basis fields into one field, whose
  major/minor eigenvectors at a point give the locally-preferred road
  directions. Every new class got a `City` prefix (matching phase 2's
  `World*` convention) and was grepped across `uppsrc/` first, per the
  `WorldOctree` ODR lesson - `Tensor` alone would have been safe (Eigen's
  `Eigen::Tensor`, pulled in transitively via `plugin/Eigen`, lives in a
  different namespace) but `CityTensor` was used anyway, cheap insurance.
  `CityTensorField::fields` uses `Array<CityBasisField>` (`Core/
  Vcont.h`), not `Vector<One<CityBasisField>>` - `Array<T>` is U++'s
  container specifically for an owned, polymorphic collection
  (`Add(T *newt)` takes ownership of a derived-type pointer directly),
  simpler than hand-rolling the same thing with `One`.
  - Verified headless by `RunCityTensorFieldSelfTest` in `main.cpp`
    (wired into `--selftest`) with analytic cases only - no streamlines/
    graph/polygons exist yet (later tasks in this phase): a lone grid
    field's major eigenvector matches its own theta within the field's
    effective falloff radius; summing an identical tensor into itself
    doubles `r` without changing `theta`; `CityTensor::Zero()` and an
    empty `CityTensorField` don't crash or produce NaN.
  - Gotcha (test bug, not a port bug - first test run genuinely crashed
    via a failed `ASSERT_`, not a memory error): the first draft tested
    the grid field's major eigenvector far outside the field's `size`
    falloff radius (points 500-1400 units from a field with
    `size=100`). Outside that radius the field's *weight* drops to 0,
    and - faithfully porting the source - `CityTensor::Add` with
    `smooth=false` hardcodes `r = 2` regardless of whether the summed
    components are meaningfully nonzero, so a fully zero-weight
    contribution still degenerates `theta` to 0 rather than leaving it
    at the field's own angle. Not a bug to fix (it's what the actual
    ported algorithm does - `Add`'s non-smooth branch is a genuine
    source quirk, kept faithfully), just a test written outside the
    property's actual domain. Fixed by widening the test field's `size`
    so every test point stays within its effective radius, and the plan
    doc's verification checklist now says so explicitly.
  - Gotcha (a real misunderstanding, caught by the same test run): first
    draft assumed a lone `CityRadialField`'s *major* eigenvector points
    radially (toward/away from the field's centre). Working through the
    field's actual algebra (`t1 = y^2 - x^2, t2 = -2xy` for the offset
    from centre) shows the opposite: `GetMinor()` is the radial one,
    `GetMajor()` is tangential/circumferential - makes sense once stated
    (a "radial" field is meant to produce ring-road-like major arterials
    circling a centre, with minor streets as the spokes radiating out),
    but was backwards in the first draft's assertion. Fixed in both the
    test and `003-city-layout/001-task1-tensor-field.md`'s verification
    checklist - a reminder that "which eigenvector does what" is worth
    deriving from the source math rather than assuming from a class's
    name.
- `CitySeparationGrid.h/.cpp` + `CityStreamlineIntegrator.h/.cpp` +
  `CityStreamlineGenerator.h/.cpp`: City Layout phase, Task 2 - traces
  road-centerline polylines through Task 1's `CityTensorField`. Ports
  of `grid_storage.ts` (a uniform-grid spatial hash enforcing a minimum
  separation distance between samples), `integrator.ts` (RK4 line
  integration - the source's *simplified* RK4, kept faithfully, see the
  class comment), and `streamlines.ts` (seeding, bidirectional
  integration, stopping rules, circle-joining, dangling-end joining) -
  see `003-city-layout/002-task2-streamlines.md`. Two dead code paths in
  the source itself were *not* ported: `SEED_AT_ENDPOINTS` is hardcoded
  `false` there (its candidate-seed-queue logic never runs), and
  `doesStreamlineCollideSelf` is unused in the source's own words
  ("Didn't end up using"). Polyline simplification (`simplify-js`) is
  also not ported this task - `GetStreamlines()` returns raw,
  unsimplified polylines; a follow-up once graph/polygon code (Task 3+)
  needs vertex-density control.
  - `CitySeparationGrid`/`CityStreamlineGenerator`/`CityTensor` etc. all
    got the `City` prefix and were grepped across `uppsrc/` first,
    continuing the convention from Task 1 (itself continuing
    `WorldOctree`'s ODR lesson from phase 2) - zero collisions found for
    any of this task's names, even unprefixed.
  - Kept faithfully, with a code comment at each site (matching Task 1's
    approach to `CityTensor::Add`'s `r = 2` quirk): `CityStreamlineGenerator::
    CreateAllStreamlines` stops the *entire* alternating major/minor
    process the moment *either* direction fails to find a seed within
    `seed_tries`, even if the other direction still has room left - a
    first read of the source suggests "keep trying the direction that
    still has room", but the actual `while(createStreamline(major))
    major = !major` loop does not do that. `StreamlineIntegrationStep`
    always pushes its failing step's point onto the streamline even when
    *out-of-bounds* is the reason it failed (so a drawn line reaches its
    true exit point instead of stopping one step early) - this is why
    the self-test's in-bounds check needs a margin, not an exact bound.
  - Gotcha - **a likely genuine upstream bug**, distinct from the
    deliberate quirks above: line-by-line tracing of
    `streamlines.ts`'s `pointsBetween` (used only while joining a
    dangling streamline end) shows an off-by-one - a `next` sample point
    is tested against the CURRENT loop index but only recomputed for the
    NEXT index at the very end of the same iteration, using the index
    *before* it increments, so the recompute is a same-value no-op and
    only actually takes effect one iteration late. Net effect: the point
    nearest the start gets inserted twice and the point nearest the
    target is never quite reached. Ported exactly as observed (see the
    comment in `CityStreamlineGenerator.cpp::PointsBetween`) rather than
    silently corrected, consistent with this phase's whole point being
    to port the actual algorithm - but flagged explicitly as "probably a
    bug, not a design choice", unlike the `r = 2` case.
  - Verified headless by `RunCityStreamlineSelfTest` in `main.cpp`
    (wired into `--selftest`): `CitySeparationGrid`'s `dsep` enforcement
    checked directly (not inferred from generator output, since a
    streamline's endpoints no longer equal their original seed after
    joining); a pure grid field (uniform weight across the whole test
    world via `decay=0`) produces streamlines whose every non-degenerate
    segment aligns with the field's `theta` to `|dot| > 0.999` (3211
    segments checked); a world far too small relative to `dsep`
    exhausts `seed_tries` almost immediately, proving
    `CreateAllStreamlines` actually returns (a real infinite loop would
    have hung the test itself, not just failed an assertion) with
    trivially bounded output.
  - Gotcha (test bug, caught immediately - same pattern as Task 1's two
    test bugs): first draft asserted every streamline point sits
    *exactly* within world bounds. Failed instantly against real output,
    for the deliberate-quirk reason above (the terminal overshoot push).
    Fixed with a documented margin instead of a tight bound, and
    `002-task2-streamlines.md`'s verification checklist updated to say
    so explicitly - matching how Task 1's doc got corrected after its
    own test-writing mistakes.
- `CityGraph.h/.cpp`: City Layout phase, Task 3 - turns Task 2's
  streamline polylines into a planar graph (`CityGraphNode`: position +
  neighbor index list). Loosely ports `graph.ts`, restructured to avoid
  both of its external-library dependencies (`isect`'s Bentley-Ottmann
  sweep, `d3-quadtree`) - see `003-city-layout/003-task3-graph.md`.
  Deviates from that task doc's own original design in two ways, both
  found to be simpler once actually implemented and both applied
  consistently with earlier "revisit only if profiling says so" calls
  in this codebase (`WorldOctree::UpdateStreaming`'s own O((range/
  cell)^2) scan):
  - **All-pairs segment intersection** (`SegmentIntersect`, standard
    parametric cross-product formula), not the grid-bucketed approach
    first proposed - simpler, still dependency-free, fine at one
    chunk's scale.
  - **No `getNodesAlongSegment` walk-and-relocate**: that source
    machinery exists to cope with `simplify-js` reducing a streamline's
    vertex density (a simplified point can drift slightly off its
    original segment, so the source re-associates nodes by walking
    `dstep` steps along the segment and fuzzy-matching). Task 2
    deliberately never simplifies streamlines, so nothing drifts -
    each segment just collects its own subdivision points (its two
    endpoints plus any interior intersection found on it), tagged with
    their exact parametric `t` along the segment, sorted by `t`, and
    consecutive points in that order become graph edges. Simpler and
    exact, not an approximation of the source's approach.
  - Fuzzy node merging (`CityGraph::FindOrAddNode`) picks the *nearest*
    existing node strictly inside `fuzzy_radius` (`<`, not `<=` -
    matches `CitySeparationGrid::IsValidSample`'s own boundary
    convention), not just the first one found within radius - avoids
    order-dependent merge artifacts.
  - Verified headless by `RunCityGraphSelfTest` in `main.cpp` (wired
    into `--selftest`) with hand-built inputs, not generated streamlines
    (keeps the expected answer unambiguous, per the task doc): a
    horizontal and vertical line crossing at a known point produce
    exactly 5 nodes, one of them degree-4 at the crossing and four
    degree-1 arm-ends; an isolated 5-point streamline with no
    intersections chains into a simple path (2 degree-1 endpoints, 3
    degree-2 interior nodes); two collinear (and therefore, by
    `SegmentIntersect`'s documented parallel-segment simplification,
    never geometrically-intersecting) segments merge their near
    endpoints into one node when placed inside `fuzzy_radius` and stay
    separate when placed outside it. All three passed on the first
    build with no test-writing mistakes this time - a change of pace
    from Tasks 1 and 2, each of which caught at least one wrong
    assumption in its own first-draft test.
- `CityBlockFinder.h/.cpp`: City Layout phase, Task 4 - extracts closed
  city-block polygons (the planar graph's faces) from Task 3's
  `CityGraph`. Direct port of `polygon_finder.ts`'s `recursiveWalk`/
  `getRightmostNode` - pure graph/geometry, no external-library
  dependency in the source either, so no restructuring needed (unlike
  Tasks 3's `isect`/`d3-quadtree` removal). Water/park filtering
  (`filterPolygonsByWater`) not ported, matching every other water-
  related scope decision this phase (Task 1's `CityTensorField::
  SamplePoint`, Task 2's `CityStreamlineIntegrator::OnLand`).
  - `FindBlocks` builds its own mutable working copy of the adjacency
    lists (directed edges get consumed as faces are found, exactly like
    the source's per-node `adj` arrays) rather than mutating
    `CityGraph`'s own undirected `neighbors` lists. One deliberate
    non-port: the source's outer loop does `for (const nextNode of
    node.adj)` over the SAME live array that a nested call can later
    splice elements out of (if the found face happens to pass back
    through `node` itself) - a real, if incidental, "mutate the array
    you're iterating" pattern that JS's iterator protocol tolerates in
    its own specific way. Iterating a `Vector<int>` in C++ while another
    call removes elements from that same vector is undefined behavior,
    not merely "a JS quirk" - so `FindBlocks` snapshots each node's
    current neighbor list before the inner loop instead. Judged as
    incidental JS runtime behavior, not domain logic worth chasing into
    UB territory, unlike the deliberate quirks kept faithfully elsewhere
    in this phase (`CityTensor::Add`'s `r = 2`, `pointsBetween`'s
    off-by-one).
  - **Corrected before writing any test**: hand-tracing `GetRightmostNode`
    through a plain 4-node unit-square graph (by hand, before touching
    the test file) showed the algorithm finds **two** faces for a
    simple closed cycle, not one - a cycle bounds both a bounded
    interior face and an unbounded exterior face, and the rightmost-turn
    walk finds both (one per direction around the cycle, since each
    directed edge belongs to exactly one face-boundary walk). The task
    doc originally said "exactly one polygon" - fixed in
    `004-task4-polygons.md` before the test was even written, the same
    kind of correction Tasks 1-2 made only after a failing assertion.
  - Verified headless by `RunCityBlockSelfTest` in `main.cpp` (wired
    into `--selftest`), hand-built graphs with known answers: a unit
    square yields exactly 2 faces (both windings, area 1 each,
    vertices matching the 4 known corners); the same square plus a
    dangling 5th node does not crash and no returned face touches the
    dangling node's position; a 20-node ring finds 0 faces when
    `max_walk_length` is smaller than the ring (every attempt truncated
    before closing) and 2 faces when it's larger (proving the cap is
    what's truncating, not some other bug, and that `FindBlocks`
    returns promptly either way - no hang). All three passed
    immediately.
  - Gotcha (not a Task 4 bug - a **pre-existing flaky test from Task 2**,
    surfaced by chance while stress-testing this task's own build):
    `RunCityStreamlineSelfTest`'s grid-field alignment check occasionally
    failed with "a grid-field streamline segment is not aligned with
    theta" on an unrelated rebuild, non-deterministically (`Randomf()`
    has no fixed seed, so every run explores a different random seed
    layout). Root cause: `JoinDanglingStreamlines`'s `GetBestNextPoint`
    has an early-accept branch (a candidate merely *close* to the
    endpoint, within `sqrt(2)*dstep`) that skips its own angle filter
    entirely - so a spliced join segment's direction isn't bounded by
    `joinangle` at all in that case, only by proximity. Widening the
    test's tolerance to `cos(joinangle)` was not enough (confirmed by a
    20-run stress test still failing intermittently) because the real
    bound is unrelated to `joinangle`. Fixed properly by testing what
    the check actually meant to prove - that `CityStreamlineIntegrator::
    Integrate` follows the field - *directly* against the integrator at
    several sample points, decoupled from the full seed/trace/join
    pipeline; the full pipeline is still checked for bounds and
    non-empty output, just not per-segment angle. Confirmed stable with
    a 20-run stress test after the fix (zero failures). `002-task2-
    streamlines.md`'s verification checklist updated to match.
- `MapGeneratorCityLayoutGenerator.h/.cpp`: City Layout phase, Task 5 -
  the real `ICityLayoutGenerator`, replacing phase 2's stub. Wires
  Tasks 1-4 together per chunk: a `CityTensorField` covering the
  chunk (one `CityGridField`, `decay=0`, sized to comfortably cover the
  chunk's diagonal - the same uniform-weight setup this phase's own
  tests use), `CityStreamlineGenerator` traced over the chunk's local
  bounds, `CityGraph::Build`, `CityBlockFinder::FindBlocks` - filling
  `WorldChunkContent::road_centerlines`/`blocks`. Each `WorldChunk` gets
  its own independent run (the task doc's "lean toward this for an
  initial working version" option) - visible seams where streamlines
  don't connect across a chunk boundary are accepted, not fixed, per
  that doc's own scoping.
  - `WorldChunkContent` grew from a single `bool has_city` to
    `Vector<Vector<Vec2>> road_centerlines`/`blocks` - "did anything
    generate here" is now just `!road_centerlines.IsEmpty()`, no
    separate flag. `Serialize`/`Deserialize` no longer hand-roll a byte
    format: `Vec2` grew a `Serialize(Stream&)` method (`s % x % y`)
    which plugs it into U++'s generic `Stream::operator%` framework, so
    `Vector<Vec2>`/`Vector<Vector<Vec2>>` become serializable for free
    through `Vector<T>`'s own existing container-serialization support
    - no manual point-counting/byte-packing code needed at all, much
    less than the earlier single-byte bool encoding might suggest would
    be needed for real geometry.
  - **Determinism**: `Generate` reseeds the shared global RNG
    (`SeedRandom`, `Randomf()` - the same RNG `CityStreamlineGenerator`
    already used throughout) from a hash of `(world_seed, chunk coord)`
    before doing anything else, so a chunk's content depends only on
    those two things, never on what order chunks happened to load in -
    required for a streamed world where load order follows player
    movement unpredictably. This reseeding is process-wide, not scoped
    to city generation - harmless while nothing else in the game
    consumes `Randomf()` concurrently with world generation (true
    today), called out explicitly in the code as something to revisit
    if that ever changes.
  - `world_seed` (constructor parameter, drives *what* generates) is
    kept deliberately separate from `WorldChunkCache`'s own
    `seed_version` (drives only *cache invalidation*, per phase 2's
    Task 2) - same string can be passed to both at the call site for
    "changing the seed regenerates everything" (the normal case), but
    they answer different questions and don't need to be coupled at the
    type level.
  - Verified headless by `RunMapGeneratorCityLayoutSelfTest` in
    `main.cpp` (wired into `--selftest`): a normal-sized chunk produces
    non-empty content; the same `world_seed` + chunk coordinate produces
    byte-identical output across independent `Generate` calls
    (determinism); two different `world_seed`s produce different output
    for the *same* chunk coordinate; `Serialize`/`Deserialize` round-
    trips real generated geometry byte-identical (not just a bool, per
    phase 2 Task 2's cache test pattern); a degenerate (zero-size)
    chunk generates nothing and does not crash. Also a new `--citytest`
    CLI mode (`RunCityDump`) dumps one chunk's road/block counts and
    endpoint coordinates - run once by hand during this task and it
    produced a visibly sensible grid-like layout (11 roads, 20 blocks,
    block areas 280-500 for a 100x100 chunk) without needing a
    screenshot, matching the project's established headless-first
    verification approach.
  - `RunWorldBuilderSelfTest` (phase 2, `002-world-builder`) updated for
    the new `WorldChunkContent` shape: `StubCityLayoutGenerator` now
    adds a trivial 2-point marker line instead of setting `has_city`,
    and assertions check `!road_centerlines.IsEmpty()` instead - same
    deterministic even/even rule, same cache-hit-vs-regenerate proof via
    the `generate_calls` counter, unchanged in spirit.
  - Confirmed with a 10-run `--selftest` stress-test loop (zero
    failures) that the new test is genuinely deterministic, not merely
    passing by chance the way Task 2's now-fixed flaky test did.
  - Not done in this task (out of the task doc's own scope): actually
    *rendering* the generated roads/blocks into the 3D scene (`PlayScreen`
    currently still only draws `Terrain` and the vehicle) - `--citytest`
    proves the data is real and sensible without needing that yet. A
    natural next step, not scoped into this phase's 5 tasks.
- `PlayScreen`'s city visualization (City Layout phase, "Task 6" - not
  one of the phase's original 5, a direct follow-up once Task 5 proved
  real content; see `003-city-layout/006-task6-playscreen-
  visualization.md`): `PlayScreen` owns a `WorldBuilder city`, created
  in `StartRun` with the selected track's `id` as both the cache's
  `seed_version` and `MapGeneratorCityLayoutGenerator`'s `world_seed`,
  streamed every `Tick()` via `city.Update(vehicle.GetChassisPosition())`,
  drawn by `DrawCityGL` (roads as dark line strips, block outlines as
  light cyan line loops, both `Terrain::HeightAt`-following, unlit -
  see that task doc for why wireframe, not filled polygons, and why
  streaming runs synchronously on the main thread).
  - `WorldBuilder::Create`'s hardcoded LOD defaults (`cell 20/dist 60,
    40/150, 80/400`) were only ever a self-test-scale placeholder (own
    comment said so) and would dwarf `Terrain::world_size=200` -
    `WorldBuilder` gained a `SetLodLevels` passthrough so a caller with
    real world-scale knowledge (`PlayScreen`) can override them (`cell
    20/dist 40, 40/80, 80/140`) without changing the shared defaults
    every existing self-test's exact expected chunk/event counts still
    depend on.
  - `WorldOctree` gained `Clear()`; `WorldBuilder::Create` now calls
    `octree.Clear()` + `content.Clear()` unconditionally at the top -
    found necessary because `PlayScreen::StartRun` (picking a different
    track from the New Game screen) can call `city.Create()` a second
    time on an already-used `WorldBuilder`, and without this, the
    *previous* run's stale chunks/content would silently linger,
    invisibly mixed into the new run's own generation. `Create()` on an
    already-empty instance (every existing self-test's usage) is
    unaffected - `Clear()` on empty state is a no-op.
  - Verified via `--playtest` (already exercises `StartRun`/`Tick`/
    `GLPaint`, now exercising `city.Update()`/`DrawCityGL` too) still
    exiting 0, plus its existing first-3-frames debug dump gaining a
    loaded-chunk/road/block count line (`loaded_chunks=44 roads=436
    blocks=689` on one run - streaming genuinely happened, in a
    plausible, non-exploding range for the tightened LOD distances).
    Full regression suite (`--selftest`, `--geomtest`, `--gltest`,
    `--menutest`) re-run and confirmed unaffected by the `Clear()`
    change to `WorldBuilder::Create`.
  - Known, accepted limitation (not addressed this task, per its own
    doc's scope): immediate-mode `glVertex3d` calls for several hundred
    roads/blocks per frame is a lot of draw-call overhead by the
    standards of a "real" renderer - no batching/VBO/display-list
    optimization exists yet. Consistent with this whole project's
    stated "simple demo, not deep frameworks" stance and the
    established "revisit only if profiling says so" pattern (`WorldOctree::
    UpdateStreaming`, `CityGraph`'s all-pairs intersection check) -
    not fixed speculatively without evidence it's actually a problem in
    practice.
- `GLDebug.h/.cpp`'s `GLSaveFramebufferPNG` + `main.cpp`'s `--citypic`
  mode: lets a human (or this assistant) actually *see* a rendered
  frame without breaking the project's no-screenshot policy. That
  policy exists because an earlier full-screen/desktop capture showed
  unrelated, possibly sensitive content that happened to be on the X
  display at the time - `GLSaveFramebufferPNG` reads only the current
  GL context's own framebuffer via `glReadPixels` (same call
  `GLReadFrameStats` already used, uneventfully, throughout this whole
  project), which by construction can only ever contain pixels
  SweetSandy itself just drew - it cannot see anything else on the
  desktop, so the original risk does not apply. `PlayScreen` gained a
  one-shot `RequestFramebufferDump(path)`; `AppShell` gained
  `GetPlayScreen()` (defined in the `.cpp`, not inline in the header -
  `AppShell.h` only forward-declares `PlayScreen`, so dereferencing
  `One<PlayScreen>` needs the complete type, which that header
  shouldn't have to include). `--citypic [path]` drives the same
  menu->newgame->play path as `--playtest`, waits long enough for
  nearby city chunks to stream in, requests the dump, then closes.
  Confirmed working: captured a real frame showing the vehicle on
  terrain with the generated road/block wireframe crossing it (used to
  answer "aja peliä ja katso miltä kaupunki näyttää" - visually
  confirmed for the first time that roads and block outlines nearly
  coincide, since block boundaries trace very close to the road
  centerlines that bound them - a real observation about current visual
  clarity, not a bug, and a natural thing to improve if/when this gets
  revisited (distinct colors help only so much when the lines
  themselves nearly overlap; a filled/inset block outline would read
  more clearly than tracing the same edge roads already occupy).
- **Fixed** (user feedback after actually looking via `--citypic`):
  block outlines now visually separate from their bounding roads, and
  the city's scale relative to the car was corrected - both real bugs,
  not just polish.
  - `DrawCityGL` (`PlayScreen.cpp`) now draws each block's vertices
    nudged 12% toward the polygon's own vertex average
    (`CityPolygonAverage`) instead of at their literal (road-bounding)
    positions - a block's true boundary traces the same edge as the
    road that bounds it, so drawn at the literal vertices the two line
    sets nearly coincided. Deliberately a simple per-vertex shrink, not
    a true parallel/uniform polygon inset (real offset geometry is
    awkward for a possibly non-convex face - in particular the graph's
    "outer"/unbounded one, see `CityBlockFinder.h`) - good enough to
    visually separate the two line sets in this wireframe preview, not
    a claim about where a real building footprint would start.
  - **Root cause of the scale bug**: `MapGeneratorCityLayoutGenerator`
    set `params.dsep = min_dim / 6` - street spacing as a *fraction of
    the chunk's own size* rather than any fixed real-world value. Every
    other number in this game (`VehicleDef::chassis_size_z`,
    `Terrain::world_size`, ...) is already in meters; city street
    spacing was the one thing silently scaling off arbitrary
    `WorldOctree` cell sizes instead - at a 20m LOD0 chunk (the
    pre-fix `PlayScreen` default) that gave ~3.3m street spacing,
    roughly the length of the ~3m car itself, exactly matching the
    reported "kaupungin koko suhteessa autoon vaikutti väärältä."
    Fixed by making `dsep` a fixed `45` (meters, documented explicitly
    as such) independent of chunk size, with `dtest`/`dstep`/etc.
    derived from that fixed value instead of the chunk. The other half
    of the fix: chunk sizes (`PlayScreen`'s `WorldOctree::LodLevel`s)
    were *also* bumped up (`90/90, 140/140, 200/200` instead of
    `20/40, 40/80, 80/140`) so each chunk is comfortably bigger than
    one street spacing instead of smaller than it - otherwise most
    chunks would contain at most one lonely street. `WorldBuilder::
    Create`'s own hardcoded defaults (used only by this phase's self-
    tests) were left untouched, per the same reasoning `SetLodLevels`
    was added for in the first place.
  - `MapGeneratorCityLayoutTest`'s counts for its own fixed 100x100
    test chunk dropped from `road_centerlines=10 blocks=16` (old,
    chunk-relative scaling) to `road_centerlines=4 blocks=1` (new,
    fixed 45m scaling) - expected and correct: a 100x100m area now
    genuinely only fits about 2 streets each direction, matching real
    city-block scale, not a regression (the self-test only asserts
    non-empty/deterministic/round-trips, not exact counts, so this
    didn't need a test change).
  - Confirmed visually via `--citypic`: block outlines now read as
    clearly distinct from roads, and the nearest street intersection
    sits a believable "city block" distance from the car instead of
    right on top of it.
- **Further tightened + a real caching gotcha found** (more user
  feedback: 45m still "looked like over 5x" too big relative to the
  car): `CityStreamlineParams::dsep` reduced `45 -> 15` (meters -
  ~4.8x `VehicleDef::chassis_size_z`, a tighter/denser grid).
  - **The gotcha**: after rebuilding with the new `dsep`, `--citypic`
    showed the *exact same* image as before the change - not a
    rendering bug, a stale disk cache. `WorldChunkCache`'s key was just
    `track.id`, and `MapGeneratorCityLayoutGenerator`'s *algorithm
    parameters* changing doesn't change `track.id` - so the cache kept
    serving chunks generated under the old `dsep=45` forever, exactly
    the failure mode Task 2's cache design was supposed to prevent (its
    own doc: "A world-seed/version stamp invalidates the whole cache
    when the generation algorithm or seed changes") but nothing was
    actually wired to *bump* that stamp when the algorithm changed.
    Fixed with `MapGeneratorCityLayoutGenerator.h`'s new
    `CITY_GEN_VERSION` macro, folded into `PlayScreen::StartRun`'s
    cache key (`track.id + "_" + CITY_GEN_VERSION`, kept separate from
    the generator's own `world_seed` constructor argument, which still
    stays just `track.id` - `world_seed` is about content variety,
    `CITY_GEN_VERSION` is about cache correctness, different concerns).
    **Bump `CITY_GEN_VERSION` any time `MapGeneratorCityLayoutGenerator`
    changes** - parameter tuning included, not just Tasks 1-4 algorithm
    changes - or this recurs. `--citytest`'s standalone `RunCityDump`
    and `RunMapGeneratorCityLayoutSelfTest` both call `Generate()`
    directly, bypassing `WorldChunkCache` entirely - neither was
    actually affected by this staleness bug, which is exactly why
    `--citytest` (used for the very first `--citypic` look) never
    caught it: it doesn't go through the code path that has the cache.
  - Confirmed via a fresh `--citypic` capture after the fix: visibly
    different (denser) content than before - proof the regeneration
    actually happened this time, not just a parameter that silently had
    no effect.
- **`--cityscale` diagnostic** (`RunCityScaleDiagnostic`, `main.cpp`):
  the user asked for actual measured numbers instead of another
  screenshot ("diagnosoi katujen todelliset mitat ja auton todelliset
  mitat ja tarkista että ne on samassa spacessa"). Checks three things a
  screenshot can't:
  1. Vehicle size, both from `VehicleDef` *and* cross-checked against
     the live ODE chassis geom's own AABB (`Vehicle::GetChassisGeom`,
     new small accessor) - so a `Vehicle::Create` bug translating
     catalog numbers into actual geometry couldn't hide behind "the
     catalog says 3.12". Confirmed identical: `1.6 x 3.12` both ways.
  2. Real *measured* street spacing, not the `dsep` input parameter
     read back - `dsep` is only a lower bound `CityStreamlineGenerator`
     enforces between same-family seeds, never a promise about the
     resulting density (random rejection sampling can place streets
     much farther apart than `dsep`, especially under `seed_tries`
     pressure). Measured by taking one representative point per
     *distinct* street at its closest approach to a z=0 cross-section,
     not naively binning "every point within N meters of z=0" - a
     street running roughly parallel to that cross-section contributes
     many consecutive same-street points only `dstep` (1m) apart, which
     would swamp genuine inter-street gaps with same-street noise. This
     was a real bug in the diagnostic's first draft: every "gap" it
     reported was suspiciously exactly ~1.00m (dstep, not a street-to-
     street distance at all) - caught immediately from the numbers
     themselves looking wrong (matches `dstep`, not `dsep`), not from
     re-eyeballing another image.
  3. Distance from the car's spawn point to the nearest generated road
     point, in the same meters both systems claim to use - a direct
     same-coordinate-space sanity check.
  - **Result with the corrected methodology**: 6 distinct streets cross
    a representative chunk's z=0 line, measured gaps 10.6-24.2m
    (average ~17m) - `3.4x` to `7.8x` the 3.12m chassis length,
    averaging `~5.5x`. Confirms `dsep=15` is working as intended (a
    lower bound around/above `dtest=11.25`) and lands close to the
    user's own stated "should be around/under 5x" reference point from
    their 45m feedback - not the runaway mismatch the very first
    (chunk-relative) version had.

## Road Network phase (`004-road-network`)

New phase: turns City Layout's flat wireframe road centerlines into a
real drivable road network - the `IRoadMeshGenerator` stage named (not
yet built) back in `002-world-builder/001-planning.md`, "RoadCreator-
derived". See `Manager/2-plan/.../004-road-network/001-planning.md` for
the full research summary (`RoadCreator`'s actual source read in full,
not just skimmed - `Misc.cs`, `RoadSegment.cs`, the mesh-building core
of `Intersection.cs`) and the "what gets simplified, and why" reasoning:
RoadCreator is almost entirely a Unity **editor** authoring tool (mouse
drag, undo/redo, curve-point dragging), not a runtime algorithm library
- the genuinely reusable core is the ribbon-mesh-from-centerline
construction (`RoadSegment.GenerateMesh`) and the general *idea* of
junction meshing (not `Intersection.CreateMainMesh`'s own implementation,
which is deeply bound to RoadCreator's own interactive curve-point data
model that has no equivalent here).

- `RoadMesh.h` + `RoadMeshBuilder.h/.cpp`: Task 1 - turns one road
  centerline (`Vector<Vec2>`) into a real triangulated ribbon mesh
  (`RoadMesh`: vertex pairs left/right per centerline point, standard
  quad-strip triangulation). Deliberately 2D, no height stored - every
  other generated-content drawing in this game (`PlayScreen`'s City
  Layout wireframe) already resolves ground height at *use* time via
  `Terrain::HeightAt` rather than baking it into stored data, and this
  follows the same convention (Task 2 will flatten `Terrain`'s own
  heightmap under a road, so `Terrain::HeightAt` along it already
  returns the correct height with no separate storage needed).
  - One deliberate deviation from the source, not a shortcut: local
    "forward" direction at interior centerline points uses a **centered**
    difference (`points[i+1] - points[i-1]`) instead of RoadCreator's
    forward-only difference at every point - smoother, still simple,
    documented as an intentional improvement.
  - No width tapering (RoadCreator's `startRoadWidth`/`endRoadWidth`) -
    City Layout's streamlines carry no per-point width concept to taper
    from, so there's nothing to interpolate; width is constant per
    ribbon. No texture/material system consumes the computed UVs yet
    (this game's rendering is flat-colored immediate-mode GL throughout,
    not textured) - computed anyway, cheap and future-proofs the mesh.
  - Verified headless by `RunRoadMeshSelfTest` in `main.cpp` (wired into
    `--selftest`): a straight 2-point centerline gets exact hand-
    computable vertex positions; a multi-point path with a turn is
    checked via invariants instead of hand-derived coordinates
    (left/right vertex distance equals width, their midpoint equals the
    original centerline point, their direction is perpendicular to the
    independently-recomputed local forward direction) plus exact vertex/
    triangle counts. Passed on the first build.
- `Terrain::FlattenUnderRibbon` (`Terrain.h/.cpp`): Task 2 - flattens
  `Terrain`'s own heightmap grid under one generated road's footprint,
  so the ground actually follows the road instead of the road floating
  disconnected above/through a bumpy heightmap. Port of `RoadSegment.
  TerrainDeformation`'s `deform` mode, precise rather than approximate
  since `Terrain` already owns a directly-writable heightmap grid (no
  raycasting needed, unlike the source's Unity-`Terrain`-component
  approach). Road height along its own length is sampled from the
  *original* (pre-deform) terrain at each centerline point, then
  linearly interpolated between consecutive points and held flat across
  the road's width at any given point along it - roads still follow the
  general lay of the land instead of being perfectly level end to end,
  matching the task doc's own framing.
  - **Scope boundary, deliberate**: this task only gives `Terrain` the
    flattening *primitive* and proves it works via `HeightAt`/ODE
    collision sampling - it does **not** wire this into the live
    `PlayScreen`/`WorldBuilder` game loop yet. That's a real, separate
    problem: `Terrain::BuildOdeHeightfield` copies the grid into ODE's
    own storage once (`bCopyHeightData=1`), and roads stream in
    incrementally per-chunk as the vehicle drives (well after
    `PlayScreen::StartRun`'s one `BuildOdeHeightfield` call) - so
    flattening a later-generated chunk's roads would need the ODE
    heightfield geom actually rebuilt (or the growing chunk of Terrain
    grid re-synced some other way) to have any effect on collision.
    Left to Task 4 ("WorldBuilder integration"), which is where real
    ODE road collision gets wired up anyway - solving both problems
    together there is more coherent than half-solving the sync problem
    here with nothing yet driving it.
  - Gotcha found and fixed via this task's own self-test, not
    guessed up front: a first version flattened only grid points whose
    position fell *exactly* inside the ribbon's true footprint. Sampling
    `Terrain::HeightAt` near (but still inside) the road's edge returned
    a value blended with the *original* bumpy terrain, failing the
    "flat across width" check - not a logic bug, a grid-resolution
    effect: `HeightAt`'s bilinear interpolation between the 2x2
    surrounding grid points can reach one grid point just outside the
    exact footprint when the road's width is only a few grid spacings
    (the default rolling terrain's grid spacing is ~3.1m, and the test
    road was 6m wide - well within "a few"). Fixed by flattening a
    footprint widened by one full grid spacing (`cell_size`) on each
    side beyond the ribbon's true edge - the *target height* computation
    (which segment, what `t`) is untouched, only *which grid points get
    written* changes. Confirmed by the same self-test: the three
    across-width query points became bit-identical, not just within
    tolerance.
  - Verified headless by `RunTerrainFlattenSelfTest` in `main.cpp`
    (wired into `--selftest`), per the task doc's checklist: flat across
    the road's width (now exactly, after the margin fix); smoothly-
    varying along its length, matching the original terrain at the
    road's own start/end; points far outside the footprint left exactly
    (bit-identical) unaffected; and - the check that actually matters
    for "is this really one shared grid" - `BuildOdeHeightfield` called
    *after* flattening produces a real ODE collision contact at the
    expected flattened height (a probe sphere dropped at the flattened
    centerline height, `dCollide` against the built heightfield geom,
    contact depth matches expectation almost exactly:
    `0.05005` vs. an expected `~0.05`).
- `RoadJunctionMesh.h/.cpp`: Task 3 - fills the gap at a junction where
  roads meet. Deliberately **not** a port of RoadCreator's `Intersection.
  CreateMainMesh` (its curve-point-blended pad is deeply bound to
  RoadCreator's own interactive per-connection "curve point", which a
  `CityGraph` node generated from streamlines has no equivalent of - see
  the phase's planning doc). A simpler, purely-geometric fan instead:
  collect every connected road's near-junction edge point pair
  (`RoadJunctionApproach{left, right}`), sort all of them by angle
  around the junction centre, and add a fan triangle to the centre for
  every *consecutive* pair of points that belong to *different*
  approaches - the gap between two different roads is empty ground the
  junction needs to fill; the gap between one road's own left/right pair
  is that road's own width, already covered by its own ribbon (Task 1),
  so it's deliberately skipped rather than double-meshed.
  - Degree-1 (dead end) gets a dedicated special case - one closed cap
    triangle - since the general "skip same-approach gaps" algorithm
    would produce *zero* triangles for a single approach (both of its
    own points, sorted, are adjacent to each other and get skipped).
  - Degree-2 (a road passing straight or gently bending through a node
    that isn't really an intersection - most `CityGraph` nodes, since
    only crossing/dangling-join points get degree >= 3) needs no special
    case at all: the general algorithm naturally produces exactly 2
    small triangles connecting each road's near side to the other's,
    seamlessly continuing the ribbon rather than leaving a gap - verified
    directly in the self-test, not just assumed.
  - Verified headless by `RunRoadJunctionSelfTest` in `main.cpp` (wired
    into `--selftest`), hand-built approaches per the task doc's
    checklist: a 4-way crossing produces triangles that use *every*
    approach's edge point at least once, with no degenerate (near-zero-
    area) triangles; a single dead-end approach gets exactly one closed
    cap triangle with real area, not an empty/degenerate fan; a 2-way
    pass-through produces exactly 2 triangles (the reasoned expectation,
    confirmed exactly, not just "some" triangles). All three passed on
    the first build.
- `RoadCreatorMeshGenerator.h/.cpp`: Task 4 - the real `IRoadMeshGenerator`,
  wiring Tasks 1-3 together per chunk. Runs one ribbon per original
  `content.road_centerlines` entry (not per graph edge - graph edges are
  typically only ~1m long, one per streamline sample point; a ribbon per
  *edge* would mean hundreds of tiny ribbons plus a junction fan at every
  one of their ordinary interior points), flattening `Terrain` under each
  as it goes (Task 2). Rebuilds a `CityGraph` from `content.road_centerlines`
  (cheap and deterministic at this chunk's scale) rather than reusing
  `MapGeneratorCityLayoutGenerator`'s own internal, unexposed graph
  instance - a documented "recompute once more" tradeoff, not an oversight.
  Skips degree-2 graph nodes entirely (an ordinary point along one
  continuous streamline, already seamlessly covered by its own ribbon -
  see `RoadJunctionMesh.h`'s own note) and builds junction fans for
  degree-1/degree>=3 nodes using **synthetic**, direction-based approach
  points (`node_pos + dir*width` then perpendicular offset), not real
  ribbon-vertex lookups - `CityGraph` doesn't retain which original
  streamline contributed which edge (it subdivides and fuzzy-merges that
  away), so exact provenance isn't available; "good enough to drive over
  without a gap" is the same standard Task 3's own junction fan was
  already built to.
  - Collision mechanism decided at this task's start: reuse `Terrain`'s
    own ODE heightfield (already flattened under every road by Task 2)
    rather than a separate `dCreateTriMesh` from the road mesh's own
    triangles - "driving on a flattened strip of the same heightfield"
    already behaves exactly like the rest of the terrain, and building
    a whole second collision representation for the same geometry would
    be duplicate machinery solving a problem that doesn't exist.
  - **Found and fixed a real bug in `Terrain::BuildOdeHeightfield`**
    while designing this task, before any test caught it: the existing
    implementation implicitly assumed "called once per session, with any
    earlier geom already gone because its whole ODE space was destroyed
    between sessions" - true for every existing caller (`PlayScreen::
    StartRun`, the various self-tests), but this task needs to call it
    *again*, mid-session, whenever a newly-streamed chunk's roads flatten
    `Terrain`'s grid further (the ODE heightfield only copies height data
    once, at build time - `bCopyHeightData=1` - so later grid mutations
    have zero effect on an already-built geom without an explicit
    rebuild). Calling it twice in the same still-live space would have
    left the *old* geom dangling in that space while pointing at
    `hf_data` this call had just destroyed. Fixed by adding a `dGeomID
    hf_geom` member: reset to 0 (without touching ODE) in `Load()`
    (a fresh session/space boundary), and explicitly `dGeomDestroy`'d in
    `BuildOdeHeightfield()` before creating the new geom, if one is
    already live. `PlayScreen::Tick` now calls it again whenever
    `WorldOctree::GetLoadedChunkCount()` changes after `city.Update()` -
    over-approximates "did anything actually change" (a loaded chunk
    without any roads still triggers a rebuild) but that's just wasted
    CPU, not incorrect, and simpler than threading a real "did any
    terrain height actually change" signal back out of `WorldBuilder`.
  - **Found and fixed a second real bug via this task's own integration
    self-test**, not guessed up front: `Terrain::FlattenUnderRibbon`
    (Task 2) only widened its flattened footprint *perpendicular* to
    each centerline segment (the fix for the grid-resolution bleed bug
    documented under Task 2 above) - never *along* each segment's own
    length. Task 2's own self-test never caught this because its hand-
    built centerline was a straight line, where consecutive segments'
    quads already meet flush at their shared vertex either way. A real
    generated road is a curved streamline, and on a curve, two
    consecutive segments' perpendicular-only-widened quads only touch at
    their shared vertex - leaving a wedge-shaped gap of *un*-flattened
    original terrain on the *outer* edge of the bend, which
    `RunRoadCreatorMeshSelfTest` caught immediately (a ~6cm height jump
    sampling across a real road's width at an interior point, first
    found at a road's own endpoint, then confirmed still present after
    switching to an interior point - ruling out "just a junction-overlap
    seam" before concluding it was this). Fixed by widening each quad
    along its own forward direction too (extending the "a" edge backward,
    "b" edge forward, by the same one-`cell_size` margin already used for
    the perpendicular direction) so adjacent quads overlap near their
    shared vertex instead of only touching it. Confirmed by the same
    self-test: the across-width heights at a real curved road's interior
    point became bit-identical, not just within tolerance.
  - **Found and fixed a third, very familiar bug immediately after**:
    a `--citypic` capture right after this task's self-tests all passed
    showed *no visible road mesh at all* - just the flattened-terrain
    strip's subtly different lighting, no asphalt color. Root cause was
    the exact same class of bug documented under `CITY_GEN_VERSION`'s own
    comment (`MapGeneratorCityLayoutGenerator.h`) from the City Layout
    phase: `WorldChunkCache`'s on-disk cache (`~/.cache/.../world/
    default_cgv2/`) still held `WorldChunkContent` blobs serialized
    *before* this task added the `road_mesh` field and wired
    `RoadCreatorMeshGenerator` in, and `CITY_GEN_VERSION` hadn't been
    bumped - so cache hits kept serving that pre-Task-4 content straight
    through, and `city_gen`/`road_gen` never ran again to populate
    `road_mesh` at all. Fixed by bumping `CITY_GEN_VERSION` to `"cgv3"`
    and widening its own doc comment to cover the *whole* per-chunk
    pipeline (both `ICityLayoutGenerator` and `IRoadMeshGenerator`
    output land in the same cached blob under one key), not just this
    file's own algorithm. Confirmed by re-capturing `--citypic`: real
    dark-asphalt road geometry with proper width, branching, and
    junctions is now clearly visible, not just a faint lighter strip.
  - `PlayScreen` changes: `StartRun` now also calls
    `city.SetRoadMeshGenerator(...)` right after `SetCityLayoutGenerator`;
    `DrawCityGL` draws `content->road_mesh` as flat dark-gray
    `GL_TRIANGLES` (replacing the old wireframe centerline lines it drew
    before this task - block outlines are unchanged, still wireframe,
    since no building mesh exists yet); `Tick` compares
    `WorldOctree::GetLoadedChunkCount()` before/after `city.Update()` and
    calls `terrain.BuildOdeHeightfield(pw.space)` again if it changed, so
    newly-streamed-in roads' flattening actually reaches collision, not
    just the render.
  - Verified headless by `RunRoadCreatorMeshSelfTest` (non-empty mesh
    from a real generated chunk; every triangle index in range; vertex
    count at least the ribbons-alone minimum; flat across a real curved
    road's width at an interior point, bit-identical; a point far from
    any road untouched; `Serialize`/`Deserialize` round-trips `road_mesh`
    byte-identical via a new `RoadMeshesEqual` helper) and
    `RunVehicleOnRoadSelfTest` (spawns the real `Vehicle` directly above
    a real generated road point, using the real `RoadCreatorMeshGenerator`
    + rebuilt ODE heightfield, and confirms it settles near the road's
    own height instead of falling through - same style/thresholds as
    `RunVehicleTerrainSelfTest`), both wired into `--selftest`. Also
    confirmed visually via `--citypic` (see above) and the full existing
    regression suite (`--selftest`, `--geomtest`, `--gltest`,
    `--menutest`, `--playtest`) still passes.

## Vegetation phase (`005-vegetation`)

New phase: procedural trees, ported from `dgreenheck/ez-tree` (JS/
Three.js) - the `IVegetationGenerator` stage named (not yet built) back
in `002-world-builder/001-planning.md`. See `Manager/2-plan/.../
005-vegetation/001-planning.md` for the full research summary (`src/lib/`
read in full - `rng.js`, `branch.js`, `options.js`, `tree.js` (the
1200-line core), `trellis.js`, plus two presets) and the "what gets
simplified, and why" reasoning: reuse this project's own `SeedRandom`/
`Randomf()` instead of porting ez-tree's own RNG class (nothing needs
bit-identical output to an ez-tree demo tree); no PBR materials/wind
shader (flat tinted colors, same simplification Road Network already
made for asphalt); no trellis (vine-training structure, no gameplay
need here); per-instance `THREE.LOD` deferred in favor of reusing the
existing per-chunk octree LOD for a tree's detail level.

- `VegQuat.h/.cpp`: Task 1 - a rotation-composing type for branch
  orientation (axis-angle construction, Hamilton-product `operator*`,
  `Apply` to rotate a `Vec3`, `Slerp`, `RotateTowards`). Ported to match
  `THREE.Quaternion`'s own semantics exactly (composition order,
  `rotateTowards`'s angle-*limited*-step-not-a-fraction behavior) since
  Task 2's skeleton-growth math (not yet written) is authored against
  those semantics.
  - **Naming caught at planning time, not mid-implementation**: `Quat`/
    `Quaternion` are confirmed unsafe - `uppsrc/Geometry/Matrix.h`
    already declares `struct Quaternion` at the same BLITZ-merged `Upp`
    namespace scope (`Geometry` links in transitively), the exact
    `WorldOctree` ODR-collision class of bug. `VegQuat` grepped clean at
    both planning and implementation time.
  - Verified headless by `RunVegQuatSelfTest` in `main.cpp` (wired into
    `--selftest`): 90-degree rotations around each principal axis
    applied to a known perpendicular vector give the hand-computable
    result (pins down this port's actual handedness/composition
    convention, since every later use of `VegQuat` must agree with it);
    `Slerp` endpoint exactness (`t=0`/`t=1` return the inputs exactly,
    not just approximately); `RotateTowards`'s clamped-step semantics
    (a huge step reaches the target exactly, a zero step doesn't move);
    composition identity (`q * Identity() == Identity() * q == q`).
    Passed on the first build, matching the exact result hand-derived
    during planning (`FromAxisAngle((0,1,0), 90deg).Apply((0,0,1))`
    gives `(1,0,0)`).
- `VegMesh.h`: Task 1 - a generic 3D triangle mesh buffer (position +
  normal + uv per vertex, flat index list). `RoadMesh` (Road Network)
  doesn't fit: it's deliberately 2D with no normals (height resolved at
  *use* time via `Terrain::HeightAt`), while a branch's cylinder-ring
  geometry and a leaf's billboard quad are inherently 3D with real
  per-vertex normals. Kept SweetSandy-local rather than moved into
  `uppsrc/`, matching `RoadMesh`'s own precedent - a genuinely generic
  container in the abstract, but with only this phase's own meshing
  (Task 3, not yet written) as a consumer so far, the extra indirection
  isn't earning its keep yet.
  - `Vec3` (`SandyMath.h`) grew a `Serialize` method (mirroring `Vec2`'s
    existing one) so `VegMesh`'s `Vec3`-valued vertices/normals can
    round-trip through `WorldChunkContent::Serialize` once Task 4 wires
    them in.
  - Verified headless by `RunVegMeshSelfTest` in `main.cpp` (wired into
    `--selftest`): `Serialize`/`Deserialize` round-trips a small
    hand-built mesh byte-identical, same pattern `RoadMesh`'s own
    round-trip test (Road Network Task 4) already established.
- `VegTreeOptions.h`, `VegSkeleton.h`, `VegTreeSkeletonGenerator.h/.cpp`:
  Task 2 - the real tree-growing algorithm, port of ez-tree's
  `Tree#generateSkeleton`/`#growBranch`/`generateChildBranches`/
  `generateLeaves`/`#recordLeaf`. A BFS work queue of pending branches
  (`VegPendingBranch`, file-local to `VegTreeSkeletonGenerator.cpp`, not
  exposed - distinct on purpose from `VegBranchOptions`/
  `VegBranchSection`/`VegSkeletonBranch`, all of which mean something
  else), starting from one trunk, growing each branch's section frames
  (gnarliness perturbation, growth-force bending, deciduous-terminal-
  continuation vs. evergreen-taper-to-a-point), then stratified-sampling
  child branches/leaves along it. RNG: this project's own
  `SeedRandom(options.seed)`/`Randomf()`, not a ported RNG class - see
  the phase's `001-planning.md`.
  - **Deliberate deviation**: gnarliness perturbation is composed
    directly as `VegQuat` rotations (two independent small local-frame
    tilts around the section's own current X then Z) instead of
    replicating the source's per-section Euler round-trip
    (`sectionOrientation.x/z += ...` on a persistent `THREE.Euler`,
    converted to/from quaternion every section). Task 1 built `VegQuat`
    specifically so this task wouldn't need a full order-dependent Euler
    type just for a randomized noise step - same statistical character
    (small, RNG-scaled perturbation every section, larger for thinner
    sections via `max(1, 1/sqrt(radius))`), not a bit-exact reproduction
    of the source's specific Euler-axis-order coupling. Twist and
    growth-force bending port directly via `VegQuat`, no deviation
    needed there (`operator*`'s composition order and `premultiply`-style
    `qForce * qSection` already match `THREE.Quaternion`'s own semantics
    per Task 1's own design).
  - **Two suspected-but-unconfirmed upstream bugs found while reading
    the source closely enough to port it, both kept faithfully rather
    than silently "fixed"** - this project's standing policy for this
    exact situation (see the `CityTensor::Add` `r=2` hardcode and the
    `pointsBetween` off-by-one from earlier phases):
    1. `#growBranch`'s own `sectionLength` divisor compares
       `this.options.type === 'Deciduous'` (capital D) against a value
       that is only ever the lowercase `'deciduous'`/`'evergreen'`
       (`enums.js`) - the comparison can never be true, so the
       `(levels-1)` divisor is permanently unreachable dead code in
       ez-tree itself (almost certainly a typo for `TreeType.Deciduous`,
       used correctly a few lines below in the same function for the
       radius taper, and again correctly as the lowercase literal
       `'deciduous'` later in the same function for the terminal-branch
       check - three different comparison styles for the same field in
       one method). Ported as written: `section_length =
       branch.length / branch.section_count` unconditionally.
    2. `generateChildBranches`/`generateLeaves` interpolate a child's
       *origin* from section A toward section B as `alpha` goes 0->1
       (`lerpVectors(sectionA, sectionB, alpha)`), but interpolate its
       *orientation* the other way, from B toward A
       (`qB.slerp(qA, alpha)`) - confirmed by working through
       `THREE.Quaternion.slerp`'s own in-place semantics (`qb.slerp(qa,
       t)` mutates `qb` toward `qa`). Nothing else in either function
       treats "A" and "B" with reversed roles - ported faithfully as
       `VegQuat::Slerp(section_b.orientation, section_a.orientation,
       alpha)` in `SampleAlongSections`, flagged rather than "corrected"
       to whatever the presumably-intended A->B direction would have
       been, since this project's own presets (Task 4) get tuned against
       ez-tree's *actual* behavior, not a guessed intended one.
  - The terminal continuation branch (deciduous only) inherits its
    *parent's own* `section_count`/`segment_count`, not
    `options.branch.sections/segments[level+1]` - ported exactly per
    the source's own comment ("must be same as parent branch since the
    child branch is growing from the end of the parent branch") and
    directly verified by the self-test below, not just trusted from
    reading the comment.
  - Verified headless by `RunVegSkeletonSelfTest` in `main.cpp` (wired
    into `--selftest`), against branch/leaf counts hand-traced from
    ez-tree's own control flow *before* writing any assert (the City
    Layout Task 4 lesson): `levels=0` gives exactly 1 branch, with
    leaves = `leaves.count` (evergreen) or `leaves.count + 1` (deciduous
    - the extra terminal tip leaf); `levels=1, children[0]=N` gives
    `1+N` branches/`N*leaves.count` leaves (evergreen) or `1+(N+1)`
    branches/`(N+1)*(leaves.count+1)` leaves (deciduous - both the
    ordinary children *and* one terminal continuation spawn together);
    the terminal continuation branch is directly distinguished from the
    ordinary level-1 children by its inherited (parent-shaped) section/
    segment count, using a fixture with deliberately different counts
    per level so the two cases can't be confused; determinism (same
    seed -> byte-identical section origins/radii, not just counts) and
    a different seed producing a different skeleton. Every hand-traced
    count matched the actual logged output exactly on the first build -
    no test-writing mistakes this round. Full existing regression suite
    (`--selftest`, `--geomtest`, `--gltest`, `--menutest`, `--playtest`)
    re-run and confirmed still green.
- `VegMeshBuilder.h/.cpp`: Task 3 - port of `#meshSkeleton`/
  `#meshBranch`/`#meshLeaf`, turning Task 2's skeleton into real
  triangulated `VegMesh` geometry. Consumes no RNG, so it can run
  repeatedly against one skeleton at different `VegMeshDetail` levels -
  the LOD reduction mechanism (`section_stride`/`segment_factor`/
  `leaf_stride`/`leaf_scale`/`force_single_billboard`, ported from
  ez-tree's own `LODDetail`) is built alongside the base meshing, not
  bolted on after. Branches and leaves stay two separate `VegMesh`
  outputs (bark tint vs. leaf tint need two different flat fill colors,
  no material system to carry the distinction instead).
  - Branch meshing: an open tube per branch (no end caps - a tip is
    either covered by a child branch's own base ring, or for a true
    terminal tip just isn't capped in the source either), rings sampled
    every `section_stride`-th section while always keeping the last one
    (so branch endpoints/junctions stay put across detail levels), each
    ring's first vertex duplicated at the end for a clean UV seam.
    `wrapsX` (texture-wrap count around the circumference) always uses
    ez-tree's own default multiplier (`1.0`) - this port has no
    `bark.textureScale.x` option to scale it by (no texture system,
    same decision Task 2's `VegTreeOptions` already made).
  - Leaf meshing: one or two (double-billboard) perpendicular quads per
    leaf. The pre-rotation + orientation composition
    (`v.applyEuler(Euler(0,rotation,0)).applyEuler(orientation)` in the
    source) ports as a single composed `VegQuat` (`orientation *
    FromAxisAngle(Y, rotation)`) applied once - `VegQuat::operator*`'s
    composition order already matches this exactly, no deviation needed
    (unlike Task 2's gnarliness step). The rounded-normals trick blends
    each corner's normal toward the direction from the leaf's own origin
    to that corner. **Ported an easy-to-miss detail exactly as written**:
    the flat/base normal (`orientation.Apply((0,0,1))`) does **not**
    depend on the per-quad `rotation` parameter - both quads of a double
    billboard share the same base outward direction, which combined with
    rounded normals gives a consistent "puffy" canopy look across both
    quads instead of each quad's own flat, 90-degrees-apart normal
    (deliberate in the source, not a bug - confirmed by the formula
    genuinely not referencing `rotation` at all, not assumed from a
    comment).
  - Verified headless by `RunVegMeshBuilderSelfTest` in `main.cpp`
    (wired into `--selftest`): a hand-built straight (unperturbed)
    trunk-only skeleton gives every ring vertex at the exact expected
    radius/height (isolates meshing from growth, per the task doc's own
    call); exact vertex/triangle-index count formulas (`S*(N+1)`
    vertices, `6*N*(S-1)` triangle indices - the task doc itself
    originally mis-stated the index count as `2*N*(S-1)`, corrected
    during this task's own implementation, see `003-task3-meshing.md`);
    `section_stride` reduction keeps the first/last ring bit-identical
    while dropping triangle count; `segment_factor` reduction hits the
    expected count and respects the `max(3, ...)` floor; leaf billboard
    modes give exactly 8/4 vertices (double) or 4/2 vertices (single),
    confirmed for both the base option and the per-level override;
    `leaf_stride` keeps *the right* leaves, not just the right count
    (checked by reconstructing which original leaf each kept quad came
    from, not just counting); a real `VegTreeSkeletonGenerator`-produced
    skeleton meshes with every triangle index in range for both
    branches and leaves (same basic mesh-integrity check Road Network
    Task 4's `RunRoadCreatorMeshSelfTest` used). Every hand-computed
    number matched the actual logged output exactly on the first build.
    Full existing regression suite (`--selftest`, `--geomtest`,
    `--gltest`, `--menutest`, `--playtest`) re-run and confirmed still
    green.
- `VegTreeInstance.h`, `VegTreePresets.h/.cpp`, `VegChunkGenerator.h/.cpp`:
  Task 4 - wires Tasks 1-3 into the real pipeline. `VegTreeInstance` is
  one placed, already-built tree (position/yaw/trunk sizing/tints/the
  two `VegMesh`es); it grew `bark_tint`/`leaf_tint` fields the task doc's
  own sketch didn't have, since PlayScreen's rendering needs a color at
  *draw* time and the instance is all it retains (the `VegTreeOptions`
  used to build it isn't kept anywhere past generation). `Color` has no
  `Stream::operator%` in this U++ tree (only a legacy `operator/` -
  `Core/Color.h` - deliberately not used, to avoid mixing two different
  serialization operators in one `Serialize` method), so tints
  round-trip via `Color::GetRaw()`/`FromRaw()` instead. `WorldChunkContent`
  grew `Vector<VegTreeInstance> vegetation`, folded into its existing
  `Serialize`/`Deserialize` (same pattern `road_mesh` was added in Road
  Network Task 4) - stored materialized, not regenerated from a seed on
  every cache hit, consistent with how `road_mesh` is already cached.
  `IVegetationGenerator` (name reserved since `002-world-builder`) added
  to `WorldGenStages.h`; `WorldBuilder` grew `veg_gen` +
  `SetVegetationGenerator`, run in `Update()` after `city_gen` (needs
  `road_centerlines` to avoid placing trees on roads).
  - `VegTreePresets`: this game's own two species (`Oak` deciduous,
    `Pine` evergreen), real meter-scaled from a target height picked
    first (not ported from ez-tree's own `oak_medium.json`/
    `pine_medium.json`, whose trunk lengths alone are 37-50 "units" -
    meaningless against this world's ~4m road width/~15m street
    spacing - see `001-planning.md`'s named scale-calibration risk).
  - `VegChunkGenerator`: jittered-grid candidate placement across a
    chunk, rejecting any candidate within a road-avoidance radius of any
    `road_centerlines` segment (plain point-to-segment distance, same
    style already used elsewhere - e.g. `Terrain::FlattenUnderRibbon`'s
    own segment math). Reseeds this project's own RNG from a *distinctly
    salted* per-chunk value (`DeterministicVegSeed`, not
    `MapGeneratorCityLayoutGenerator`'s own `DeterministicChunkSeed`) so
    vegetation placement doesn't start at the exact same RNG state city
    layout already reseeded to for the same chunk. Placement decisions
    (position/yaw/species/per-tree seed) happen in one clean pass before
    any tree is actually grown, in a second pass - deliberately *not*
    interleaved, since each `VegTreeSkeletonGenerator::Generate` call
    reseeds the shared RNG independently per tree (Task 2's own
    self-contained convention); interleaving would still be
    deterministic but would needlessly entangle one candidate's
    placement with how many RNG draws an *earlier* tree's own skeleton
    happened to consume.
  - `RandomRange` (a `RandomRange(lo, hi)` one-liner) promoted from a
    `VegTreeSkeletonGenerator.cpp`-local helper (Task 2) into
    `SandyMath.h` once this file needed the exact same thing - the
    "wait for a second real consumer" version of avoiding premature
    abstraction, not written speculatively up front.
  - Chunk LOD -> `VegMeshDetail`: a whole-chunk-granularity proxy for
    tree render detail (coarser chunk LOD gets a more reduced tree
    mesh), not true per-instance camera-distance LOD - the "per-instance
    LOD deferred" call `001-planning.md` already made.
  - `PlayScreen`: `DrawVegetationGL` draws every loaded chunk's trees as
    lit (unlike `DrawCityGL`'s unlit wireframe/flat road mesh - `VegMesh`
    carries real per-vertex normals, so lit rendering is a strict
    upgrade here), flat-tinted `GL_TRIANGLES`, translated/yawed via
    `glPushMatrix`/`glTranslated`/`glRotated` per tree. Real ODE
    collision: one static `dCreateCylinder` per tree trunk (rotated
    local Z -> world Y via `dQFromAxisAndAngle`, no body - matches
    `Terrain`'s own static heightfield geom), lifecycle tracked
    incrementally per chunk (`PlayScreen::veg_colliders`, a
    `VectorMap<WorldChunkCoord, Vector<dGeomID>>`) rather than
    `Terrain::BuildOdeHeightfield`'s own destroy-everything-and-rebuild
    pattern (Road Network Task 4) - `WorldOctree` already reports
    per-chunk load/unload individually, so a full rebuild on every
    single chunk change would scale badly once per-chunk tree counts add
    up. Created when a chunk's colliders are first needed, destroyed
    when `WorldOctree` reports that chunk unloaded - piggybacks on the
    same loaded-chunk-coord set `WorldBuilder::Update` itself already
    prunes `content` against.
  - **`CITY_GEN_VERSION` bumped again (`"cgv3"` -> `"cgv4"`)**,
    preemptively this time - adding `vegetation` to the cached
    `WorldChunkContent` shape is exactly the class of change that macro's
    own comment already names, and this is the third time it's mattered
    (`dsep` tuning, Road Network's `road_mesh` field, now this) - bumped
    before ever capturing a first `--citypic`, specifically to skip a
    fourth repeat of the same "generator changed, cache silently didn't"
    debugging session.
  - **Placement density and preset complexity tuned against real
    `--vegtest` measurements, not guessed**: the first values tried
    (20m candidate grid spacing, 6m road-avoidance radius) placed only
    1 tree in a 90x90m chunk - City Layout's own street spacing
    (`dsep`=15m) means a 6m avoidance radius around *every* road
    excludes most of a chunk's area, and a 20m grid left too few
    surviving candidates to begin with. Retuned to a 14m grid / 4.5m
    avoidance radius (10-16 trees per chunk in testing) and trimmed
    preset leaf/child counts (Oak `children[1]` 3->2, `leaves.count`
    10->5; Pine `children[0]` 30->18, `leaves.count` 20->12) after
    measuring a single tree's own triangle cost (~1800 triangles) was
    heavier than intended for "low-poly, modest budget" - re-measured
    after each change via `--vegtest` rather than eyeballing a render.
  - Verified headless by `RunVegChunkGeneratorSelfTest` (measured preset
    heights fall in the intended 4-18m real-world range for both
    species; every placed tree clears the road-avoidance radius, checked
    numerically against every road segment, not eyeballed; determinism;
    `Serialize`/`Deserialize` round-trip of `WorldChunkContent.vegetation`)
    and `RunVehicleTreeCollisionSelfTest` (a vehicle at full throttle
    aimed straight at a tree trunk never tunnels through it and gets
    stopped well short - same style as Road Network's own
    `RunVehicleOnRoadSelfTest`; a tree placed well off to the side of
    the vehicle's own path leaves it completely unaffected, proving the
    collider isn't accidentally oversized), both wired into `--selftest`.
    `--vegtest` (new CLI mode) dumps real per-chunk tree/triangle counts
    for the draw-cost risk. Confirmed visually via `--citypic`: real,
    species-distinguishable low-poly trees lining the streets, clear of
    the road surface, at a believable scale next to the vehicle. Full
    regression suite (`--selftest`, `--geomtest`, `--gltest`,
    `--menutest`, `--playtest`) re-run and confirmed still green.

Vegetation phase: complete. All four tasks (math+mesh foundation,
skeleton generation, meshing, WorldBuilder integration) implemented and
verified. A generated city now has real, low-poly, species-
distinguishable trees along its streets, streamed in per-chunk, cached
to disk, with real ODE trunk collision. Next phase per the project's
own scoping (not started, no plan written yet): AI traffic and
pedestrians on the generated road graph.

## Post-Vegetation follow-up: octree LOD retuning + scope clarification

User report after driving around the finished Vegetation build: "vain
korttelin ääriviivat maassa, ei rakennuksia" (only block outlines, no
buildings); "octree ei vaikuta striimaavan eri LOD:lla pelaajan
sijainnin mukaan" (octree doesn't seem to stream different LOD by
player position); "puu-objektit eivät näytä saavan low-poly versiota
kauempana/matalammalla LOD:lla" (trees don't seem to get their low-poly
version farther away / at a lower LOD).

**Buildings: not a bug, never scoped.** Confirmed by grepping every
planning doc and `AGENTS.md` for "building" - City Layout's own
planning doc explicitly named block polygons as *future* building
placement, and every later phase's own comments ("no building mesh
exists yet") treat this as known, deferred scope, not an oversight. No
phase ever planned a building-mesh generator; the block outlines
(`DrawCityGL`, City Layout Task 6) are exactly what they say they are -
a wireframe preview of where a footprint would go, nothing more. Would
need its own future phase (its own `IBuildingGenerator`-shaped stage,
matching `ICityLayoutGenerator`/`IRoadMeshGenerator`/
`IVegetationGenerator`'s already-established pattern) - not started, no
plan written.

**Octree LOD streaming: a real scale mismatch, not a bug in
`Octree.cpp`.** Added `--worldscale` (`main.cpp`) to diagnose with real
numbers instead of guessing - reports `Terrain::world_size`/corner
distance against `WorldOctree`'s configured LOD levels, and which LOD
levels actually get loaded at a range of viewer distances from the
origin. Confirmed: `Octree.cpp`'s own `UpdateStreaming` logic is
correct (each finest-grid sample point gets assigned to the *nearest*
LOD level whose `load_distance` covers it, exactly as designed) - the
actual problem was `PlayScreen::StartRun`'s own LOD distances (90m/
140m/200m), which an *earlier* fix had already "tightened... to fit
Terrain::world_size instead of dwarfing it" but not far enough: even
the *outermost* ring (200m) still exceeded `Terrain`'s own centre-to-
corner radius (~141m for `world_size=200`), so the *entire* drivable
area sat inside a single LOD ring's radius at all times - every
position, including the spawn point, already saw all three LOD levels'
content simultaneously (confirmed: 8/14/14/12/14/14/12/14 loaded chunks
across viewer x=0..161m, essentially flat regardless of position).
Retuned to 35m/55m/90m (still `>=2x` `MapGeneratorCityLayoutGenerator`'s
own 15m street spacing, same design constraint the original tuning
respected) - now the outermost ring sits safely inside `Terrain`'s own
100m half-width, so driving from the centre toward the edge crosses all
three LOD boundaries for real, and which chunks are loaded genuinely
depends on where the vehicle is (re-confirmed with `--worldscale` after
the change).
- **This is a scoped fix, not a solution to the original "GTA-style
  open world" ambition.** `Terrain` itself is still the small, fixed
  ~200m heightmap from phase 1 - retuning the LOD *rings* to fit inside
  it makes streaming genuinely observable while driving around the
  existing small map, but doesn't create new, ever-changing distant
  content the way an actually large or unbounded streamed terrain
  would. That would be a much bigger future phase (`Terrain` itself
  becoming tiled/streamed/procedurally generated, not just one fixed
  heightmap) - not started, no plan written, a real scope decision for
  the user to make separately.
- **`CITY_GEN_VERSION` bumped again (`"cgv4"` -> `"cgv5"`), a new kind
  of trigger this time**: not a generator-algorithm change, but a
  `WorldOctree` cell-size change - a chunk coordinate's `(lod,x,y,z)`
  tuple is only meaningful relative to the `cell_size` that produced it
  (derived at runtime, never stored in the cache key), so changing
  `cell_size` means the *same* coordinate tuple now refers to a
  completely different physical area. Old cached content under that
  coordinate would be silently wrong for the new layout if served again
  - squarely the class of bug that macro's own comment already
  documents, just triggered by chunking-scheme config this time instead
  of a generator's own algorithm.

**Tree LOD reduction: was already wired correctly** (`VegChunkGenerator::
DetailForLod`, Vegetation Task 4) - verified directly, independent of
the streaming-radius issue above, by generating the same chunk at
`lod=0` vs `lod=2` and confirming `VegMeshBuilder` actually produces
fewer vertices/triangles at the higher lod (already covered by Task 3's
own `RunVegMeshBuilderSelfTest`, which tests `VegMeshDetail` reduction
directly). The reported symptom was a consequence of the streaming-radius
issue above, not a separate bug: since the drivable area barely left
`lod=0`'s own ring before this fix, reduced-detail trees rarely
generated at all during normal play - now that `lod=1`/`lod=2` chunks
(and their correspondingly-reduced trees) actually load as intended
while driving, this should already be resolved as a side effect; worth
a fresh look next time the game is played to confirm visually.

### Visual confirmation: `--lodpic` (new CLI mode)

Added to *look at* the retuned LOD streaming while actually driving,
not just read `--worldscale`'s own chunk-count numbers. Drives the
vehicle outward from spawn via a synthetic held throttle key
(`PlayScreen::Key(K_UP, ...)` - the exact same code path a real held
key takes, not a physics-only shortcut, so the real `Tick`/`GLPaint`
pipeline runs including `city.Update`'s own streaming), capturing a
framebuffer plus a logged position/loaded-chunk-LOD breakdown at four
points along the way. `PlayScreen` grew two small read-only accessors
(`GetVehiclePosition`, `GetCity`) for this logging - the first time
anything outside `PlayScreen` itself has needed to inspect its own
live state, so these didn't exist before.

Confirmed with real captures: loaded-chunk composition changes visibly
as the vehicle moves (`lod0=4,lod2=4` at spawn -> `lod0=4,lod1=2,lod2=4`
at 3.8m -> `lod0=4,lod1=4,lod2=4` at 16m -> `lod0=4,lod1=3,lod2=6` at
37.9m), and the captured frames themselves show it: more distant,
denser road/tree detail enters frame as the vehicle drives out, not a
static unchanging view. Driving speed reached under full throttle in
this real, timer-driven run was noticeably slower than
`RunVehicleTerrainSelfTest`'s own idealized fixed-dt loop (37.9m by
~6.7s of held throttle here vs. ~45m by 4.67s there) - expected, not a
bug: this run competes for real wall-clock time with actual per-frame
rendering and synchronous chunk generation, which the self-test's tight
physics-only loop never has to share time with.

**Two things found while looking, neither addressed yet - both worth a
decision, not silently fixed or ignored**:
- **A vehicle-tree collision happened unprompted** during this capture
  (visible in `lodpic_3.png` - the vehicle stopped and turned against a
  tree trunk mid-drive) - good incidental real-gameplay confirmation
  that Vegetation Task 4's collision (previously only checked via a
  hand-placed self-test scenario) also works when a tree just happens
  to be in a moving vehicle's own path.
- **Independent per-LOD city layouts visibly overlap at LOD boundaries**
  (`lodpic_4.png`'s distant background shows extra, thin, unrelated-
  looking road lines crossing through the normal road network). Root
  cause: `MapGeneratorCityLayoutGenerator` seeds and generates a
  *completely independent* tensor-field layout per chunk coordinate
  (City Layout's own "each chunk gets its own independent run...
  visible seams... accepted, fixable-later look" design, from that
  phase's own planning doc) - and different LOD levels use different
  `cell_size`, so a `lod=1`/`lod=2` chunk covering roughly the same
  physical area as several `lod=0` chunks generates its *own*,
  unrelated street network for that same space, rather than a coarser
  version of the same streets. This was always true in principle since
  City Layout shipped, but the pre-retuning LOD distances made it much
  less visible in practice (barely any `lod>=1` content ever loaded
  nearby). **Fixed in the very next session turn** - see "Coarser LOD
  now derives its content from the finest level" below.

### Coarser LOD now derives its content from the finest level

Direct follow-up to the overlap found above, per the user's own explicit
request ("aja karkeampi LOD sama layout kuin hienoin" - make the
coarser LOD [use] the same layout as the finest). `MapGeneratorCityLayoutGenerator`
no longer runs its own independent tensor-field/streamline pass for a
`coord.lod > 0` chunk - instead it figures out which `finest_cell_size`
(lod-0-sized) cells the coarse chunk's own bounds cover, generates each
one *exactly as if it were really being requested as its own lod-0
chunk* (same `DeterministicChunkSeed` input - the cell's own coordinate,
always `lod=0`), and merges their `road_centerlines` into one
`WorldChunkContent`, then builds the graph/blocks once over the merged
set. Not an approximation of the finest layout - literally built from
the same per-cell generation, so two LOD levels covering the same area
now show byte-identical streets there by construction, not just
"similar".

- `WorldChunk.h` grew two small shared helpers (`WorldChunkCoordAt`,
  `WorldChunkCenterAt`) matching `WorldOctree::UpdateStreaming`'s own
  chunk-coordinate-from-world-position math exactly - extracted (not
  written speculatively) because this fix is now a *second* real
  consumer of the identical computation, and letting the two drift out
  of sync would silently break "derive from the same cells WorldOctree
  itself would use". `Octree.cpp` refactored to use them too, so both
  sides are now provably the same math, not just coincidentally matching
  literals.
- `MapGeneratorCityLayoutGenerator`'s constructor grew a
  `finest_cell_size` parameter (defaulting to this project's own current
  lod-0 value, `30.0`, so every existing test - all of which only ever
  request `coord.lod == 0` - needed zero changes: the new subdivide-and-
  merge logic only runs for `coord.lod > 0`, and a real `coord.lod == 0`
  request still generates directly over its own bounds, unchanged in
  substance from before this fix). `PlayScreen::StartRun` passes its own
  `levels[0].cell_size`, not a separately-picked number.
- **Also retuned `PlayScreen`'s own LOD cell sizes to exact integer
  multiples of the finest level** (30/60/90, was 35/55/90) while already
  touching this code - not required by the fix itself, but eliminates a
  real ambiguity the fix would otherwise have: with non-multiple cell
  sizes, a coarse chunk's own bounds don't align to whole finest cells,
  so two neighbouring coarse chunks could each compute a finest cell
  that straddles their shared boundary as "theirs", generating the same
  small area's content twice (harmless in practice - just a redundant,
  overlapping line at that one seam - but avoidable for free by keeping
  the sizes as exact multiples, so it was). Bumped `CITY_GEN_VERSION`
  again (`"cgv5"` -> `"cgv6"`) - this time a real generator-algorithm
  change (coarse chunks produce entirely different content now), not
  just a chunking-scheme config change like the previous bump.
- Verified headless by `RunCityLodDerivationSelfTest` in `main.cpp`
  (wired into `--selftest`): the *same* finest cell, reached three
  different ways (a real lod-0 request, as the first constituent of a
  lod-1 chunk, as the first constituent of a lod-2 chunk), produces
  byte-identical `road_centerlines` every time; a coarse chunk's total
  count is compatible with genuinely aggregating multiple cells (not
  just repeating the one already checked); regenerating the same
  `coord.lod > 0` chunk twice is still deterministic. All passed on the
  first build. Confirmed visually by re-running `--lodpic`: no more
  extra crossing lines in the distance (compare the fixed `lodpic_4.png`
  against the pre-fix capture described above) - the same coherent
  street network extends from near to far now. Full existing regression
  suite (`--selftest`, `--geomtest`, `--gltest`, `--menutest`,
  `--playtest`) re-run and confirmed still green.

Same-LOD neighbouring-chunk seams (each lod-0 cell is still its own
independently-seeded run - only *cross-LOD* consistency was ever the
ask here) remain the same accepted, documented limitation City Layout's
own planning doc already named - not addressed by this fix, not
supposed to be.

## AI traffic and pedestrians phase (`006-ai-traffic-and-pedestrians`)

New phase: the last piece explicitly deferred back in
`002-world-builder/001-planning.md`'s own phase-2 scoping. Unlike City
Layout/Road Network/Vegetation, no single reference project exists to
port - see `Manager/2-plan/.../006-ai-traffic-and-pedestrians/
001-planning.md`'s own "no reference project this time" section. Also
this project's first genuinely *dynamic runtime* system - AI agents are
stateful, moving things, not a deterministic function of `(world_seed,
chunk coord)` the way every earlier phase's generated content is, so
they're owned directly by `PlayScreen` (like `Vehicle`/`PhysicsWorld`
already are), not routed through `WorldBuilder`/`WorldChunkCache` at
all. Two architecture decisions made in the plan itself rather than
per-task: kinematic movement (no per-agent full ODE suspension sim,
just a repositioned static collider) to keep draw/physics cost down at
scale; spawn/despawn tied to the same per-chunk load/unload events
Vegetation Task 4's own `veg_colliders` lifecycle already uses, not a
separate radius check.

- `AIPathFollower.h/.cpp`: Task 1 - the kinematic stepper both AI
  traffic (Task 2, not yet built) and AI pedestrians (Task 3, not yet
  built) will use: given a `Vector<Vec2>` path and a speed, produces a
  position/heading each tick. Heading is constant along one straight
  segment and jumps at each vertex (not smoothly interpolated) -
  matches how every polyline elsewhere in this project already gets
  treated as a chain of exact straight pieces.
  - **A real design bug found and fixed before it ever reached a task
    consuming it**: a closed loop's own implicit closing segment (the
    last path point back to the first - `path` holds each vertex once,
    it does not repeat the first point at the end) was, in the first
    draft, silently *skipped* rather than walked - `Advance()` reset
    straight to segment 0 once past the last real segment, without ever
    treating "last point back to first" as a segment of its own. Caught
    by hand-tracing the task doc's own "verify across at least two full
    wraps, to catch an off-by-one that only shows up the second time
    around" instruction *before* writing the test - a loop with `N`
    points has `N` segments, not `N-1`. Fixed with two small helpers
    (`SegmentCount()` - `path.GetCount()` for a closed loop, `GetCount()
    - 1` for an open one; `PointAt(i)` - modulo-wrapped indexing) used
    throughout `Advance`/`UpdatePose` instead of raw array indexing, so
    both path kinds share one implementation rather than needing a
    special case for the closing segment. `001-task1-path-follower.md`
    updated to match the corrected design, not left describing the
    buggy first draft.
  - Verified headless by `RunAIPathFollowerSelfTest` in `main.cpp`
    (wired into `--selftest`): exact hand-computable position/heading on
    a straight path; heading changes exactly at a turn's own vertex, not
    before or after; an open path clamps at its own exact last point and
    reports it can't advance further, rather than overshooting; a closed
    loop (a 10x10 square, 40m perimeter *including* the closing segment)
    returns to the exact start after one full lap *and* after a second
    full lap (the two-lap check is what actually would have caught the
    skipped-closing-segment bug, not the one-lap check alone), and a
    third partial advance lands exactly on the closing segment's own
    midpoint - the specific segment the fix was about. One test-writing
    mistake caught and fixed along the way: an `ASSERT_` missing its
    required message argument (a straightforward compile error, fixed
    immediately) and a genuinely wrong hand-derived expected distance
    for the closing-segment-midpoint case (`perimeter*0.5+5` computed
    25m, but landing on the closing segment's own midpoint after 3 full
    sides actually needs `perimeter*0.75+5` = 35m - caught by the
    assertion failing with a real, informative mismatch, not a passing-
    but-meaningless test). Determinism (same path + `Advance` sequence
    gives byte-identical results) also checked. Full existing regression
    suite (`--selftest`, `--geomtest`, `--gltest`, `--menutest`,
    `--playtest`) re-run and confirmed still green.
- `TrafficManager.h/.cpp`, `TrafficCar`: Task 2 - spawns kinematically-
  moving AI cars onto a newly-loaded chunk's own roads, routes them by
  random-walking a `CityGraph` rebuilt from that chunk's own
  `road_centerlines`, moves them with `AIPathFollower` (Task 1), gives
  each a static ODE box collider (repositioned every tick, no `dBody` -
  matches `001-planning.md`'s own kinematic-movement decision), and
  draws a simplified flat-colored car (4.2m x 1.8m x 1.4m - a "regular
  car", bigger than the player's own 1.6m x 3.12m off-road buggy).
  Routing: a fixed-length (6-hop) random walk per laid-out route
  segment, re-extended from wherever the car currently is once
  `AIPathFollower::Advance` reports the end reached - never crosses into
  a different chunk's own road network (`TrafficManager`'s own
  `chunk_graphs` holds one independent `CityGraph` per spawning chunk).
  Spawn count: 1-2 cars per chunk with roads, speed 4-8 m/s - real
  tuning against a measured diagnostic is Task 4's own job, per the
  phase's own named risk, not guessed here.
  - `TrafficManager::PickNextNode` (the actual random-walk routing
    decision) made public specifically for direct testing, the same
    call already made for City Layout's own `CityStreamlineIntegrator::
    Integrate` when a test needed to isolate one algorithm step from a
    whole pipeline's emergent behavior.
  - `DrawUnitBox` promoted from a `PlayScreen.cpp`-local `static`
    function into a new shared `GLPrimitives.h/.cpp` once AI traffic
    became a second real consumer (the player's own chassis box was the
    first) - the same "wait for a second real consumer" call already
    made for `SandyMath.h`'s own `RandomRange`, not written speculatively.
  - **`CityGraph` needed a `Moveable<CityGraph>` tag it didn't have
    before** - found immediately by the build, not guessed: every
    earlier consumer (`MapGeneratorCityLayoutGenerator`,
    `RoadCreatorMeshGenerator`) only ever built one local instance and
    read it directly, but `TrafficManager` is the first to store a
    `CityGraph` *inside a container* (`VectorMap<WorldChunkCoord,
    CityGraph>`, one kept alive per chunk with live cars) - U++'s own
    container relocation machinery refused to compile without the tag.
    Safe to add: `CityGraph`'s only data is one already-relocatable
    `Vector<CityGraphNode>`.
  - **A real, previously-undocumented fact about this project's own ODE
    build surfaced by this task's own collider-position test**: `dReal`
    is single-precision `float` here, not `double` - this project links
    the plain `ode` library (`SweetSandy.upp`: `library(!WIN32) ode;`),
    and `/usr/include/ode/precision.h` defaults to `dSINGLE` unless
    `dIDEDOUBLE` is defined, which nothing in this build does. A first
    draft of the "collider follows the kinematic pose" check used a
    `1e-9` tolerance (appropriate for double-precision exactness) and
    failed - not because of an actual repositioning bug, but because
    `dGeomSetPosition`/`dGeomGetPosition` round-trip every position
    through `float` storage, whose rounding noise at this game's own
    coordinate magnitudes is closer to `1e-5`/`1e-6`. Fixed by widening
    the tolerance to `1e-4` (comfortably above that noise, still tight
    enough that a real bug - a stale geom, a swapped axis - would show
    up as an unmistakable, meters-scale mismatch). `002-task2-ai-
    traffic.md` updated to explain this rather than leave the original
    "exactly, not just close" wording standing uncorrected. Worth
    remembering for every future task in this phase (and beyond) that
    compares an ODE-derived value against a computed one: this project's
    `dReal` is `float`, so any such comparison needs a `float`-precision-
    appropriate tolerance, not `double`-scale exactness.
  - Verified headless by `RunTrafficManagerSelfTest` in `main.cpp`
    (wired into `--selftest`): `PickNextNode` never reverses back to
    `came_from` across 200 repeated calls on a known degree-4 crossing
    (reusing the exact same hand-built graph `RunCityGraphSelfTest`
    itself already uses) when another neighbor exists, and always
    returns the only neighbor at a genuine dead end; every point of
    every spawned car's own route lies within a generous tolerance of
    its spawning chunk's own `road_centerlines` (proves the graph-per-
    chunk scoping is actually respected, not just assumed); determinism
    (same seed+chunk spawns the same car count at the same starting
    positions, checked via an independent `TrafficManager`/`PhysicsWorld`
    pair to rule out any cross-instance interference); the collider's
    own live position tracks the kinematic pose (within the float-
    precision tolerance above) after 30 ticks; `DespawnForChunk` frees
    every geom it owned, checked directly against the ODE space's own
    `dSpaceGetNumGeoms` count before/after, not just this project's own
     bookkeeping. Full existing regression suite (`--selftest`,
     `--geomtest`, `--gltest`, `--menutest`, `--playtest`) re-run and
     confirmed still green.

- `PedestrianModel.h/.cpp` (Tesseract integration, Task 3D3): the shared
  skinned character - Tesseract's own ogre
  (`packages/models/ogro2/ogro.iqm`, skin `green.jpg`, clips `idle.iqm` /
  `run.iqm`) loaded **once**, skinned on the CPU by Rt3D's `Rt3SkelModel` and
  drawn with this project's own immediate-mode GL. `uses Rt3D` is what pulls
  the engine package in; `<Rt3D/Rt3D.h>` in `SweetSandy.h`.
  - `Load(target_height, error)` is data-only (no GL), so `--modeltest` and
    `PedestrianManager::Create` can call it; the **texture** is created on the
    first real `DrawGL`, because that is the first moment a GL context exists.
  - Scale and ground offset come out of the bind pose, not out of constants:
    the model's own height is 2.25 units and 4.18 units across (the weapon),
    so a hardcoded scale would be a second, wrong copy of the asset.
  - The model's forward is **+z**, taken from the bind pose: over the lowest
    fifth of the model the spread along z (0.65) dwarfs the spread along x
    (0.28) - feet are longer front-to-back than side-to-side - and the tail of
    that distribution (the toes) is at +z. `kModelYawCorrection` is the one
    place that assumption lives.
  - **`GL_LIGHT_MODEL_TWO_SIDE` while drawing the character**, and it is not
    decoration: the IQM y-flip reverses the winding, so from outside the model
    the visible triangles are back faces, and a back face under GL's one-sided
    lighting is black however bright the light is. Tesseract's own model
    shaders do the per-fragment equivalent (`if(!gl_FrontFacing) N = -N`);
    there is no shader here. Without it the character draws as a correct black
    silhouette - which is exactly what the first capture showed.
  - `--modeltest` is the headless twin: loads the model, drives both clips
    through 40 phases each, and pins that the character's height stays within
    a factor of two of its own (idle 1.43-1.76 m, run 1.62-2.01 m at a 1.70 m
    load height), that the bounding box actually moves, and that **every
    skinned normal is a unit vector** (a degenerate one is a silent "draws
    black" failure, so it is checked where it can be seen, not in the
    renderer).
  - `--pedpic` is the visual one, and its own trick is worth knowing: it
    captures the frame **twice from inside one `GLPaint`** - the same camera,
    the same matrices, the same everything - once with the character drawn and
    once with `SetDrawCharacter(false)`. Two *runs* would not do: the car has
    driven on between them, and the first attempt at this diffed whole
    shifting scenery instead of the character. `PlayScreen::DrawSceneGL` exists
    so a frame can be re-rendered from a clean buffer. The pair is 5.7% of the
    frame, and the diff is clean humanoid silhouettes where the flat figures
    were. The same block projects the nearest on-screen pedestrian and logs the
    pixel height it spans against the pixel height a standing 1.7 m figure
    spans at that distance (88.5 px vs 77.3 px at 15 m - the idle clip
    crouches and bobs, so it is above 1 by design).
  - The character's own texture is *dark* (`green.jpg` averages RGB 32,43,18,
    half its pixels in the darkest bucket), so "the character is nearly black"
    is partly the asset. What the diff proves is that the character's mean
    pixel colour tracks the texture's own histogram, i.e. the skin is applied
    and lit rather than drawn as an untextured silhouette.

- `PedestrianManager.h/.cpp`, `Pedestrian`: Task 3 - spawns kinematically-
  moving AI pedestrians onto a newly-loaded chunk's own block perimeters,
  walks each forever around a block's own inset polygon ring with
  `AIPathFollower` (closed_loop=true), gives each a static ODE capsule
  collider (0.3m radius, 1.7m tall - person-sized, distinct from
  TrafficManager's own car boxes), and draws a simplified flat-colored
  stick-figure. The stick figure is the **fallback** now (Tesseract Task 3D3):
  within `kPedMeshDistance` (60 m) of the camera a pedestrian is the real
  skinned character, further away it is the flat figure - the level of detail
  is a distance because there are ~185 of them and CPU-skinning 539 vertices
  each is not something to do for the ones behind you. Reuses `DrawCityGL`'s own inset construction (`CityPolygonAverage`
  + `inset_fraction=0.12`) exactly - verified by checking every path point
  lies within tolerance of an independently-computed inset ring vertex,
  proving the "walks the same line already drawn" claim directly.
  - No graph/routing needed: a block polygon is already a single closed
    loop, and `AIPathFollower`'s own closed_loop=true mode handles wrapping
    forever with zero extra logic.
  - Spawn count: 1-2 pedestrians per block, each starting at a random point
    along the ring with a random direction (CW/CCW), speed 0.8-1.5 m/s
    (leisurely walking, slower than TrafficManager's 4-8 m/s city driving).
  - The per-pedestrian animation phase comes from a **hash**, never from
    `Randomf()`. Two extra `Randomf()` calls per pedestrian shift every later
    random choice in the chunk and silently change the deterministic world -
    the traffic diagnostic's `pedestrians=185` became 182 the moment this was
    a `Randomf()` pair, which is how it was caught. It is back to 185.
  - `DrawGL(eye)` takes the camera position because the LOD split is a
    distance; the split counters are `mutable` because the draw is `const`.
  - Same float-precision tolerance (1e-4) for the collider-follows-pose
    check as TrafficManager - this project links the plain `ode` library,
    so `dReal` is `float`.
  - Task 4 integration: `PlayScreen` now owns both managers, creates them
    after the city generators, spawns/despawns them from the real
    `WorldBuilder::Update` installed/unloaded chunk set, ticks them every
    frame, and draws them in `GLPaint`. `StopRun` despawns their geoms
    before destroying the physics space.
  - Verified through `--playtest`, `--traffictest`, and
    `RunTrafficIntegrationSelfTest`; the real generated diagnostic reported
    12 loaded chunks, 12 cars, 185 pedestrians, and 197 collision geoms.

## Physics Abstraction phase (`007-physics-abstraction`)

Inserted between AI Traffic and Pedestrians Tasks 2 and 3, on the
user's own explicit request: game code should never call ODE directly,
but through a game-specific wrapper (`PhysBackend.h/.cpp`) whose public
API is precision-agnostic (`double`/`Vec3` throughout, never `dReal`) -
done now, before AI Pedestrians (Task 3) adds even more code that would
need migrating later. Full reasoning and the 4-task design in
Manager/2-plan/.../007-physics-abstraction/001-planning.md.

- **Task 1: World, space, stepping, joint groups** - `PhysBackend.h`
  declares opaque handle types (`PhysWorldId`/`PhysSpaceId`/
  `PhysJointGroupId`/forward-declared `PhysGeomId`/`PhysBodyId`/
  `PhysJointId`) and the world/space/joint-group lifecycle + stepping
  API; `PhysBackend.cpp` is the only file that includes `<ode/ode.h>`
  so far (Task 4's own completion check - `grep -rl "ode/ode.h"
  *.h *.cpp` naming exactly `PhysBackend.cpp` - doesn't hold yet, since
  Tasks 2-4 haven't migrated their own consumers).
  - `PhysicsWorld::Create`/`Destroy`/`Step` now call `PhysCreateWorld`/
    `PhysWorldSetGravity`/`PhysWorldSetCFM`/`PhysCreateSpace`/
    `PhysCreateJointGroup`/`PhysJointGroupEmpty`/`PhysSpaceCollide`/
    `PhysWorldQuickStep`/`PhysDestroy*` instead of the raw ODE
    equivalents; `PhysicsWorld.h`'s own `world`/`space`/`contacts`
    members are now `PhysWorldId`/`PhysSpaceId`/`PhysJointGroupId` -
    `PhysicsWorld.h` no longer includes `<ode/ode.h>` (confirmed by
    direct grep). `PhysicsWorld.cpp`'s own `NearCallback` is
    deliberately **not** fully migrated: it's reached through
    `PhysSpaceCollide` (so its signature already takes `PhysGeomId`s,
    not raw `dGeomID`s) but its body still casts straight back to
    `dGeomID`/`dBodyID` and calls `dCollide`/`dGeomGetBody`/
    `dJointCreateContact`/`dJointAttach`/`dAreConnectedExcluding`
    directly - contacts are Task 3's own API, not Task 1's, and
    `PhysicsWorld.cpp` is commented explaining exactly why this is
    sequenced work, not an oversight.
  - `main.cpp`'s own single `dInitODE2(0)`/`dCloseODE()` pair (global
    init/close, one call site of each pattern repeated across every
    CLI mode) is now `PhysInit()`/`PhysClose()`.
  - **A design tension the task doc didn't fully anticipate, found only
    by actually compiling**: `PhysicsWorld::world`/`space` becoming
    opaque `Phys*` types broke every *other* file that still passes
    `pw.world`/`pw.space` straight into a raw-ODE-typed parameter
    (`Terrain::BuildOdeHeightfield(dSpaceID)`, `Vehicle::Create(dWorldID,
    dSpaceID, ...)`, `TrafficManager::Create(dSpaceID, ...)`, and every
    self-test in `main.cpp` that calls `dCreateSphere`/`dCreateCylinder`/
    `dCreateCapsule`/`dCreateBox`/`dCreatePlane`/`dCreateHeightfield`/
    `dSpaceGetNumGeoms` against `pw.space` directly) - none of those
    consumers migrate until Tasks 2-4. Fixed with an explicit, commented
    `(dSpaceID)pw.space` / `(dWorldID)pw.world` cast at every such call
    site in `main.cpp` and `PlayScreen.cpp` - a deliberate, temporary
    bridge (removed piece by piece as each consumer's own task lands),
    not a leak of the wrapper's own opacity. Worth remembering for
    Tasks 2-4: as each consumer (`Terrain`, `Vehicle`, `TrafficManager`,
    then the remaining self-tests/`PlayScreen.cpp` bits) migrates its
    own signature onto `Phys*` types, its own casts disappear - by
    Task 4's end there should be none left anywhere outside
    `PhysBackend.cpp` itself.
  - New isolated self-test `RunPhysBackendSelfTest` in `main.cpp` (wired
    into `--selftest`, right after `RunAIPathFollowerSelfTest`):
    exercises `PhysBackend`'s own world/space/joint-group/stepping API
    directly, without going through `PhysicsWorld` - confirms an empty
    space's `PhysSpaceCollide` never invokes the near-callback, and
    that repeated `PhysJointGroupEmpty`/`PhysSpaceCollide`/
    `PhysWorldQuickStep` cycles on an empty world/space/joint-group
    don't crash or assert, proving the wrapper's own lifecycle works in
    isolation before trusting `PhysicsWorld`'s own migration (which
    reuses these same calls) to prove it indirectly.
  - Verified: full existing regression suite (`--selftest`,
    `--geomtest`, `--gltest`, `--menutest`, `--playtest`) re-run and
    confirmed still green, including `VehicleTerrainTest`/
    `ObstacleCourseTest`/`TrafficManagerTest`'s own logged numbers
    (unchanged - a purely mechanical call substitution, no math
    touched); `RunPhysBackendSelfTest` itself logs OK; `grep "ode/ode.h"
    PhysicsWorld.h` finds nothing (Task 1's own literal completion
    check, distinct from Task 4's project-wide one above).

- **Task 2: Geoms and mass** - `PhysBackend.h/.cpp` extended with geom
  creation (box/sphere/cylinder/capsule/plane/heightfield),
  position/rotation/attachment, AABB queries, and `PhysMass`
  construction (`PhysMakeBoxMass`/`PhysMakeCylinderMass`/
  `PhysMassTranslate`/`PhysMassAdd`) - `PhysMass` is plain `double`
  fields (`mass`/`cx,cy,cz`/`I[9]`), not an opaque handle, converted
  to/from a real `dMass` only inside `PhysBackend.cpp`. `Terrain.h/.cpp`
  fully migrates: `hf_geom` is now `PhysGeomId`, `BuildOdeHeightfield`
  takes/returns `Phys*` types, and `hf_data`/`~Terrain()` are **removed
  entirely** - see below for why that's actually safe, not a regression.
  - **`PhysBackend.h` needed to move earlier in `SweetSandy.h`'s own
    include list**: it was originally placed right before
    `PhysicsWorld.h` (matching Task 1's own narrow scope at the time),
    but `Terrain.h` - included much earlier - now needs `PhysGeomId`/
    `PhysSpaceId` too. Moved to right after `SandyMath.h`, before every
    other game header, since nothing in `PhysBackend.h` itself depends
    on anything else in this project.
  - **A real design question Task 1's own "PhysDestroyGeom handles
    both" framing glossed over**: a heightfield geom's own height data
    is not freed by ODE when its geom is destroyed - the original
    `Terrain` handled this with a `hf_data` member and a manual
    "previous session's data" cleanup branch, because a heightfield's
    *owning space* can be destroyed (`PlayScreen::StopRun`/`pw.Destroy`)
    without ever calling `Terrain`'s own cleanup path, leaving `hf_geom`
    a now-dangling ID that must never be touched again (`Load()`'s own
    long-standing comment). Simply making `PhysDestroyGeom` free a
    heightfield's paired data doesn't cover that path at all, since
    `dSpaceDestroy` destroys every geom in a space directly, without
    ever calling `PhysDestroyGeom` for each one.

    Fixed at the wrapper level, not worked around in `Terrain` again:
    `PhysCreateHeightfield` pairs a geom with its own height data via
    `dGeomSetData`/`dGeomGetData` (ODE's own generic per-geom user-data
    slot), and **`PhysDestroySpace` itself now sweeps every geom still
    in a space before destroying it**, freeing any attached heightfield
    data first (`dGeomGetClass(g) == dHeightfieldClass`). This fully
    closes the leak for *any* heightfield in *any* space, generically -
    not just `Terrain`'s own - and means `Terrain` itself no longer
    needs to know or care about this at all: `hf_data` and the custom
    `~Terrain()` destructor are both simply gone, not replaced by an
    equivalent wrapper-side workaround. A better design than the
    original, made possible specifically by owning both `PhysDestroyGeom`
    *and* `PhysDestroySpace` in the same wrapper.
  - `PhysGeomSetBody`'s only consumer today is Task 3 (not yet
    implemented) - declared now since the header already forward-
    declares `PhysBodyId`, matching the task doc's own design.
  - Extended `RunPhysBackendSelfTest` (`main.cpp`, still wired into
    `--selftest`) with Task 2's own checks: hand-computable box/sphere
    AABB at known positions; a non-symmetric box's AABB actually
    shifting after `PhysGeomSetRotationAxisAngle` (proves rotation is
    applied, not silently ignored); `PhysMakeBoxMass`/
    `PhysMakeCylinderMass` combined via `PhysMassAdd` summing to the
    expected hand-computed total; a flat heightfield's AABB centered on
    its own position (the Road Network phase's own original bug),
    a real collision against it via raw `dCollide` (still Task 3's own
    API, cast at the call site per the established temporary-bridge
    pattern), then an explicit `PhysDestroyGeom`+rebuild within the same
    space (the same-session rebuild-safety path `Terrain::
    BuildOdeHeightfield` itself relies on) - and the rebuilt geom is
    deliberately left live when the test's own space is destroyed at
    the end, exercising `PhysDestroySpace`'s own sweep for real, not
    just reasoned about.
  - Verified: full existing regression suite re-run and confirmed still
    green, including `TerrainFlattenTest`'s own contact depth
    (`0.05007553`, matching its pre-migration `~0.05` expectation) and
    `--geomtest`'s `FlatHfTest`/`RampHfTest` numbers (both migrated to
    call `BuildOdeHeightfield` with no cast needed now, since it takes
    `PhysSpaceId` directly - one of the temporary Task 1 casts removed
    as its own consumer migrated, exactly as planned); `grep
    "dGeomID\|dSpaceID\|dHeightfieldDataID\|dReal" Terrain.h Terrain.cpp`
    finds nothing.

- **Task 3: Bodies, hinge2 joints, contacts** - the largest task:
  `PhysBackend.h/.cpp` extended with body lifecycle, the full hinge2
  joint API (`PhysHinge2Set*`/`PhysHinge2GetAngle1`, deliberately named
  after ODE's own axis-numbered hinge2 concept - `Motor1`/`Motor2`/
  `Stops1`, not car-specific names - keeping the wrapper a faithful,
  thin joint wrapper, not a car-physics abstraction of its own), and
  the contact/collision layer (`PhysCollide`/`PhysCreateContactJoint`/
  `PhysBodiesConnectedByNonContactJoint`). `Vehicle.h/.cpp` - this
  project's single most complex ODE consumer - migrates fully, and
  `PhysicsWorld.cpp`'s own `NearCallback` (left partially on raw ODE by
  Task 1, deliberately) finally finishes migrating here too.
  - `PhysCollide` does **not** reuse the original `NearCallback`'s own
    `dContact`/`dContactGeom` memory-layout trick (`&contact[0].geom,
    sizeof(dContact)`, letting `dCollide` fill each `dContact`'s
    embedded `geom` sub-struct while leaving room for `surface` right
    after it in the same array). Since the wrapper translates into its
    own plain `PhysContactGeom` array anyway (not reusing ODE's own
    struct layout for anything downstream), there is no reason to
    replicate that trick internally either - `PhysCollide` just uses a
    plain local `dContactGeom[64]` buffer (`dCollide`'s stride
    parameter accepts any stride `>= sizeof(dContactGeom)`, ODE's docs
    don't require it to specifically be `sizeof(dContact)`) and copies
    each result into the caller's `PhysContactGeom` array. Simpler than
    the original code, not just an equivalent restatement of it.
  - `PhysContactSurface` intentionally has no `mode` field - every
    contact-creating call site in this project (still just the one, in
    `PhysicsWorld.cpp`'s own `NearCallback`) has always used exactly
    the same surface mode (`mu` + `slip1`/`2` + soft ERP/CFM +
    `dContactApprox1`), so `PhysCreateContactJoint` hardcodes it -
    matching the task doc's own design rather than exposing a knob
    nothing currently needs.
  - `Vehicle::Step`'s own "upright" check (`chassis_rot[5]`, ODE's raw
    row-major rotation matrix element `R[1][1]`) has no dedicated
    single-element wrapper accessor - reusing `PhysBodyGetGLMatrix`
    (already needed for `PlayScreen`'s own chassis rendering) to get the
    same value is the closest available API, and its own column-major
    conversion happens to place that same element at `m[5]` too (traced
    by hand from `PhysBodyGetGLMatrix`'s own conversion:
    `m[4]=r[1]; m[5]=r[5]; m[6]=r[9]` - `m[5]` is `r[5]` unchanged), so
    the substitution is exact, not merely equivalent. A little wasteful
    (builds a full 4x4 to read one element) but correctness-preserving,
    which is this task's own stated priority - not something to
    micro-optimize mid-migration.
  - Every remaining consumer of `Vehicle::GetChassisBody`/
    `GetWheelBody`/`GetChassisGeom` (a dozen-plus call sites across
    `main.cpp`'s own vehicle self-tests, plus `PlayScreen.cpp`'s
    `BodyGLMatrix` calls) got the same explicit, commented
    `(dBodyID)`/`(dGeomID)` temporary-bridge cast established in Task 1
    - these getters now return `Phys*` types, and none of those
    consumers migrate until Task 4 (`main.cpp`'s own self-tests) or
    stay as `PlayScreen.cpp`'s own local `BodyGLMatrix` helper (deleted
    in favor of `PhysBodyGetGLMatrix` in Task 4, per the task doc).
  - Verified: `RunVehicleSelfTest`/`RunVehicleTerrainSelfTest`/
    `RunAccelerationTest`/`RunTurningTest`/`RunObstacleCourseSelfTest`
    all produce **bit-identical** logged numbers to before this task -
    directly diffed against a saved pre-Task-3 log, not just "still
    passes" (e.g. `VehicleTerrainTest[selftest full throttle]: final
    chassis=(-1.03319430351257, 7.90055322647095, 52.3865432739258)
    angvel=0.104901432991028 upright=0.9947097` and `ObstacleCourseTest:
    final z=100.03148651123 max_clearance=3.31479002434207 at
    z=71.9887008666992`, both identical down to the last printed
    digit). `grep -nE '\b(dWorld|dSpace|dBody|dGeom|dJoint|dMass|
    dQuaternion|dReal|dContact|dCollide|dAreConnected)[A-Za-z]*'
    Vehicle.h Vehicle.cpp` and the same against non-comment lines of
    `PhysicsWorld.cpp` both find nothing (a naive `d[A-Z]` grep gives
    false positives against `Phys*Id`/`Phys*` identifier substrings -
    e.g. `PhysWorldId` contains `dId` - so the real check needs ODE's
    own actual prefixes, not a bare `d[A-Z]` pattern; Task 4's own
    "no bare `d[A-Z]` symbol" check will need the same refinement, not
    the literal pattern as first written in `004-task4-remaining-
    consumers.md`). Full existing regression suite still green.

- **Task 4: Remaining consumers + completion check** - the last task of
  this phase. `PlayScreen.cpp`/`.h` (`veg_colliders` -> `Vector<PhysGeomId>`,
  its own `BodyGLMatrix` deleted in favor of `PhysBodyGetGLMatrix`, the
  tree-trunk collider lifecycle in `Tick()` migrated), `TrafficManager.h/.cpp`
  (car colliders - same mechanical substitution already done for
  `Vehicle.cpp` in Task 3), and every remaining direct-ODE self-test in
  `main.cpp` (including the heightfield-centering tests and
  `RunGeomCollisionTest`, whose entire *purpose* is diagnosing low-level
  collider behavior - they migrate too, since `PhysBackend` is meant to
  be a thin, faithful wrapper, not a behavior-altering one) all moved
  onto `PhysBackend`.
  - **`PhysSpaceGetNumGeoms` added** - not part of Tasks 1-3's own
    design (no consumer needed it yet), but `RunTrafficManagerSelfTest`'s
    own geom-leak checks (`geoms_before`/`geoms_after_spawn`/
    `geoms_after_despawn`) needed it to finish migrating - added when
    the actual consumer needed it, not spec'd speculatively earlier.
  - **The global `#include <ode/ode.h>` had to move out of `SweetSandy.h`
    into `PhysBackend.cpp` itself** to make the completion check's own
    `grep -rl "ode/ode.h"` literally true. `SweetSandy.h` included ode.h
    once, globally, at the very top (before any per-file header, outside
    `NAMESPACE_UPP`) - every other file always got it transitively and
    never needed its own `#include`. Once every other `.h`/`.cpp` in the
    project was confirmed to reference zero real ODE symbols (verified
    with a precise, comment-excluding scan - the naive `d[A-Z]` grep's
    own false-positive problem, found in Task 3, applies here even more:
    trailing comments like `// dParamFMax ceiling` on an already-migrated
    code line also need excluding, not just whole-line comments), moving
    the include was safe. `PhysBackend.cpp` now has its own explicit
    `#include <ode/ode.h>` at the top (still outside `NAMESPACE_UPP`,
    matching where it always lived) instead of relying on the
    since-removed transitive one.
  - **Both completion checks, run for real, not reasoned about**:
    `grep -rl "ode/ode.h" *.h *.cpp` names exactly `PhysBackend.cpp` and
    `PhysBackend.h` (the latter only via its own explanatory *comment*
    saying it does NOT include ode.h - a harmless false positive from a
    plain substring grep, confirmed by checking for a real `#include`
    line specifically). The word-bounded ODE-prefix scan (Task 3's own
    corrected version, not the original naive `d[A-Z]` pattern) finds
    zero real code matches anywhere outside `PhysBackend.cpp` - every
    remaining hit is a trailing comment (e.g. `// dParamFMax ceiling`)
    or a string literal inside a `LOG()` call, both harmless.
  - **Honest accounting of a small, expected precision-format change**
    (not claiming false bit-identical results here, unlike Task 3):
    a handful of `main.cpp` self-test log lines that print a derived
    quantity (velocity magnitude, an "upright" rotation-matrix element)
    computed from body rotation/velocity now show *more* decimal digits
    than before - e.g. `VehicleTerrainTest[...]: final chassis=(...)
    angvel=0.104901432991028 upright=0.9947097` (pre-Task-4) vs
    `angvel=0.104901430965104 upright=0.994709730148315` (post-Task-4).
    Root cause, traced by hand: these tests used to read a raw `dReal*`
    (float) straight out of `dBodyGetRotation`/`dBodyGetAngularVel` and
    do their own `sqrt(x*x+y*y+z*z)` magnitude arithmetic *in float
    precision* before the final result got implicitly widened to double
    for logging; `PhysBodyGetGLMatrix`/`PhysBodyGetAngularVel` now widen
    each component to `double` *before* handing it to the test, so the
    same magnitude arithmetic now happens in double precision throughout
    - strictly more precise, not a bug, and the differing digits are
    all *below* every `ASSERT_` threshold in these tests (all generously
    toleranced, e.g. `av_mag < 20.0`, `upright > 0.25`) by many orders
    of magnitude. The underlying simulated physics (chassis position,
    step-by-step `Format("%.3f", ...)`-truncated log lines) is confirmed
    bit-identical via the same saved-log diff used in Task 3 - only
    these specific full-precision "final" summary lines' own *last few
    printed digits* differ, and only because they now print a double
    instead of implicitly stream-formatting a raw float. Worth recording
    precisely rather than either (a) falsely claiming bit-identical
    across the board, or (b) treating a expected, harmless precision
    change as a regression needing a workaround.
  - Verified: **both completion checks above, run for real** - the
    actual point of this whole phase. Vegetation tree collision
    (`RunVehicleTreeCollisionSelfTest`) and AI traffic collision (part
    of `RunTrafficManagerSelfTest`) - the two "static geom, externally
    repositioned every tick, no body" patterns this phase's own API
    needed to support cleanly - both migrated with a plain mechanical
    substitution, no workarounds needed, confirming `PhysBackend`'s own
    Tasks 2-3 design already covered what they needed. Full existing
    regression suite (`--selftest`, `--geomtest`, `--gltest`,
    `--menutest`, `--playtest`) re-run and confirmed green - the last
    time this phase needs to say so, since every future physics-touching
    task (AI Traffic and Pedestrians Task 3, AI pedestrians, onward) is
    now written directly against `PhysBackend` from the start.

## Buildings phase (`008-buildings`)

User redirect: world-building content is more relevant right now than
AI work - AI Traffic and Pedestrians Task 3 (AI pedestrians) is paused,
not abandoned. Confirmed which specific gap to fill: buildings for city
blocks (`DrawCityGL` draws every block as a bare wireframe outline -
explicitly flagged "no building mesh exists yet - that's a later
phase" back in `002-world-builder/001-planning.md`, never spun into
its own phase until now). 3-task plan; simple rectangular box massing
(not extruding the raw, road-curvature-driven block polygon directly -
see Manager/2-plan/.../008-buildings/001-planning.md for the measured-
area reasoning) placed via a jittered-grid-plus-rejection pass, the
same structural pattern `VegChunkGenerator` already uses for trees.

- **Task 1: Building placement** - `BuildingPlacement.h/.cpp`:
  `PlannedBuilding` (position/yaw/footprint/height/roof-style/tints)
  and `PlanBuildingsForBlock(block, road_centerlines, base_seed)`, pure
  geometry with no meshing yet.
  - **Two geometry helpers promoted into `SandyMath.h`** on their
    second real (non-test) consumer, matching `RandomRange`'s own
    established promotion precedent: `CityShoelaceArea` (was `static`
    in `main.cpp`, test-only) and `DistancePointToSegment`/
    `DistanceToNearestRoad` (were anonymous-namespace-local in
    `VegChunkGenerator.cpp`) - both kept their exact existing names and
    behavior (no renaming churn), with `main.cpp`/`VegChunkGenerator.cpp`
    updated to call the shared versions instead of keeping duplicate
    copies. Confirmed behavior-preserving by re-running every existing
    self-test that already exercises them and diffing logged numbers
    against a saved pre-promotion log - byte-identical.
  - **A new `PointInPolygon`** (ray-casting/crossing-number, handles
    non-convex polygons) added alongside them - no existing equivalent
    (`Terrain.cpp`'s own `PointInTriangle`/`PointInQuad` are fixed-
    arity, road-ribbon-specific). Grepped `uppsrc/` first (standing
    ODR rule): one hit, a `static` (internal-linkage, no real collision
    risk) function of the same name in the unrelated `ModelerApp`
    package.
  - **A real placement gap found and fixed before it ever shipped**:
    an earlier draft of this design (grid spacing + per-candidate
    jitter, mirroring tree placement) had no check for two *accepted*
    buildings' own footprints overlapping each other within the same
    block - unlike a small tree canopy, a 7-16m building footprint is
    a meaningful fraction of the placement grid's own spacing, and
    worst-case jitter can push two adjacent candidates together. Found
    by hand-tracing what a block big enough to fit two buildings could
    actually produce, not just assumed safe because tree placement's
    own analogous grid never needed this. Fixed with a conservative
    footprint-bounding-circle overlap check against every already-
    accepted building in the same block (reject, don't shrink/deform -
    matches this project's own placement philosophy throughout) -
    `001-task1-building-placement.md` updated in place to describe this
    before implementation, not left describing the gap as if it never
    existed.
  - **A real self-test-authoring bug, caught immediately by the test
    itself, not silently wrong**: the overlap and distribution self-
    tests originally used synthetic blocks (120x120m, 400x400m) far
    larger than `kMaxBlockArea` (5000m² - a deliberate defensive guard
    against an implausibly large block polygon, sized against real
    measured block areas of ~240-700m² via `--citytest`). Both blocks
    were silently rejected by that guard before placement ever ran,
    producing 0 planned buildings - caught by the overlap test's own
    `ASSERT_(planned.GetCount() >= 2, ...)` failing loudly, not a
    passing-but-meaningless test. Fixed by sizing the overlap test's
    own block just under the cap (65x65m) and changing the distribution
    test to aggregate 200 independently-seeded calls on a modest block
    instead of expecting one huge block to produce ≥20 buildings in a
    single call (which the defensive cap makes structurally impossible)
    - a better test design anyway, not just a workaround.
  - Verified headless by `RunBuildingPlacementSelfTest` (wired into
    `--selftest`): `PointInPolygon` against both a square and a hand-
    built concave (notched/L-shaped) polygon - specifically distinguishes
    a correct crossing-number test from a naive bounding-box-only check,
    which would wrongly accept a point sitting in the L-shape's own
    notch; every planned building's own 4 corners clear both the block
    boundary and a road running along one edge by the full setback
    distance; no two accepted buildings' own footprint-bounding circles
    overlap (a large-enough block deliberately used to make this a real
    test, not a vacuous one - the assertion itself requires ≥2 buildings
    or fails loudly); determinism (same seed/block reproduces identical
    buildings); tall-building fraction across 200 seeded runs landed at
    15.2%, matching the ~15% design target closely. Full existing
    regression suite (`--selftest`, `--geomtest`, `--gltest`,
    `--menutest`, `--playtest`) re-run and confirmed green.

- **Task 2: Building mesh generation** - `BuildingMeshBuilder.h/.cpp`:
  `BuildingMeshBuilder::Build(b, ground_y, walls, roof)` turns one
  `PlannedBuilding` into two separate `VegMesh` values (walls, roof -
  not one merged mesh, since they get different tints at draw time,
  matching `VegTreeInstance`'s own separate branches/leaves meshes for
  the identical bark-vs-leaf-tint reason). Walls: 4 flat-normal
  vertical quads. Roof: a flat quad, or a gabled roof (ridge along the
  *longer* footprint axis, pitch a fixed fraction of the shorter axis).
  - **A latent cross-file naming collision, exposed (not caused) by
    this task's own new files**: `RoadCreatorMeshGenerator.cpp` and
    `TrafficManager.cpp` each independently declared their own
    anonymous-namespace `const double kFuzzyRadius = 2.0` (both for
    "rebuild a `CityGraph` from `road_centerlines`" - the exact same
    purpose, coincidentally the same value, previously safe only
    because BLITZ's own merge grouping happened to keep these two
    files apart). Adding two new files shifted that grouping enough
    that BLITZ merged both into the same translation unit, turning a
    latent landmine into a real `redefinition of 'kFuzzyRadius'`
    compile error. Fixed by promoting a single
    `kCityGraphRebuildFuzzyRadius` into `CityGraph.h` (not just
    renaming one copy to dodge the collision) - the two were always
    conceptually the same constant serving the same purpose, so
    keeping them independently editable was the actual latent bug,
    this project's own established "static-in-two-BLITZ-merged-files"
    lesson applying to an anonymous-namespace `const` this time,
    not a class.
  - **Gabled-roof slope normals: a first draft got these wrong, caught
    by working through the geometry by hand before trusting the code**
    - an initial implementation composed `Vec3::Cross` calls ad hoc
    without verifying the result against physical intuition. Redone
    properly: for each roof slope quad, computed the *actual* triangle
    normal via `(p1-p0)×(p2-p0)` on its real vertices, checked the sign
    against what "outward and upward" should mean for that specific
    slope (a nearly-flat roof should have an almost-vertical normal; a
    steep roof should tilt toward horizontal - a dimensional sanity
    check, not just a sign flip), and used the resulting hand-verified
    closed-form expressions
    (`Vec3(0, hd, y1-ridge_y)`/`Vec3(0, hd, ridge_y-y1)` for the
    ridge-along-x case, the width/depth-swapped equivalents for
    ridge-along-z) directly rather than trusting an unverified cross-
    product composition. `RunBuildingMeshSelfTest` exercises *both*
    ridge orientations (width>depth and depth>width) specifically to
    catch a first draft that only handled one axis correctly.
  - Verified headless by `RunBuildingMeshSelfTest` (wired into
    `--selftest`): hand-computable wall vertex positions/normals for a
    known footprint; flat roof at exactly `y=height` with normal
    straight up; gabled roof (both axis orientations) has a vertex at
    the exact documented ridge height and its own two gable-end
    triangles are genuine vertical planes (all 3 vertices share one
    coordinate); exact triangle counts (10 flat, 14 gabled - an earlier
    draft of the task doc itself mis-arithmetic'd the gabled count as
    18, corrected before implementation); no degenerate (near-zero-
    area) triangles across both roof styles at a non-zero yaw. Full
    existing regression suite re-run and confirmed green, including
    `RoadCreatorMeshGenerator`/`TrafficManager`'s own tests through the
    newly-shared fuzzy-radius constant - unchanged (same `2.0` value).

- **Task 3: WorldBuilder integration** - the phase's last task.
  `BuildingInstance.h` (mirrors `VegTreeInstance.h`'s own shape exactly,
  including the Color-via-raw-dword `Serialize` trick), `IBuildingGenerator`
  (`WorldGenStages.h`, alongside the other three stage interfaces),
  `BuildingChunkGenerator.h/.cpp` (the real generator - per-block seed
  derived from a per-chunk seed, since a chunk can hold several blocks
  and reusing one seed across all of them would correlate their own
  jitter), `WorldBuilder`'s own `building_gen`/`SetBuildingGenerator`
  slot wired into `Update()` after `city_gen` (needs `blocks`/
  `road_centerlines`), `PlayScreen` integration (`SetBuildingGenerator`
  call, `building_colliders` - the exact same per-chunk incremental
  lifecycle `veg_colliders` already established, box colliders via
  `PhysCreateBox`/`PhysGeomSetRotationAxisAngle`, written directly
  against `PhysBackend` from the start - and `DrawBuildingsGL`,
  `DrawTreeMeshGL` renamed to `DrawFlatTintedMeshGL` on its own second
  real consumer). `CITY_GEN_VERSION` bumped `cgv6`→`cgv7` (content
  shape changed - a new `buildings` field - so every previously-cached
  chunk needs invalidating, matching this constant's own documented
  purpose exactly).
  - **A real placement-yield bug found only by testing against the real
    generator, not the hand-built self-test blocks** - by far this
    task's own biggest finding: the first real `--playtest` run placed
    **zero** buildings across 12 loaded chunks despite Tasks 1-2's own
    self-tests all passing. Diagnosed with temporary per-candidate
    rejection-reason counters in `PlanBuildingsForBlock` (removed once
    the cause was found, not left in as permanent debug noise): real
    blocks are frequently narrow, curvy-street-following strips (their
    own *area* says nothing about their own *width* at any point),
    unlike the hand-built 60-65m square test blocks that happened to
    mask this. Retuned against the real pipeline, not re-guessed:
    `kGridSpacing` 20→8, footprint range 7-16m→5-11m, `kBuildingSetback`
    3.5m→2.2m (still comfortably clears Road Network's own
    `kRoadWidth`-implied ~2.0m road half-width) - went from 0 to 27
    buildings across the same 12 chunks, confirmed both by the logged
    count and visually via `--buildingpic`. `001-task1-building-
    placement.md` and `003-task3-worldbuilder-integration.md` both
    updated in place with the full story. **The standing lesson,
    recorded for every future placement/generation task in this
    codebase**: a hand-built, deliberately-regular, generously-sized
    synthetic test block is not a substitute for validating against the
    real generator's own output before calling a placement task done -
    this project's first miss of exactly that kind.
  - **A stale on-disk cache actively hid the bug during initial
    debugging** - worth recording since it cost real diagnostic time:
    `WorldChunkCache::Open` keys its own cache directory by
    `seed_version` (`track.id + "_" + CITY_GEN_VERSION`), so the very
    first `--playtest` run after wiring `SetBuildingGenerator` cached
    its (zero-building) result under the `cgv7` directory - every
    subsequent `--playtest` run then silently loaded that same stale
    cached content instead of regenerating, even after the generator
    code itself was fixed, until the `~/.cache/AiUpp/SweetSandy/*/world/
    default_cgv7` directory was manually cleared. A real bump to
    `CITY_GEN_VERSION` (as already planned for the schema change itself)
    resolves this for actual players; it's specifically a *development-
    time* trap worth knowing about - a code fix alone doesn't show up
    in a re-run without clearing (or re-bumping past) the cache.
  - Verified: `RunVehicleBuildingCollisionSelfTest` (straight-into-
    building must not tunnel through; near-but-not-through must be
    unaffected - mirrors `RunVehicleTreeCollisionSelfTest` exactly);
    `RunBuildingChunkGeneratorSelfTest` (determinism through the *full*
    city-layout+building pipeline, not just `PlanBuildingsForBlock` in
    isolation; `WorldChunkContent::Serialize`/`Deserialize` round-trip
    including both mesh vertex counts and tints); a new `--buildingpic`
    diagnostic (mirrors `--citypic`'s own shape) captured and viewed via
    the Read tool - shows a real building with a visibly distinct wall
    tint and a gabled roof with a real ridge, alongside roads/block
    outlines/trees/the vehicle, confirming buildings render as intended
    content, not just passing assertions. Full existing regression
    suite (`--selftest`, `--geomtest`, `--gltest`, `--menutest`,
    `--playtest`) green - the last time this phase needs to say so.

## Unified Ground phase (`009-unified-ground`)

User redirect: several in-game visual/gameplay gaps reported together -
grass and asphalt as separate overlapping objects (a `y_offset=0.08`
hack hides the z-fighting), trees rendering inside buildings, a small
fixed-origin terrain patch while the city streams further out, and
flat-colored (untextured) leaf quads. Confirmed via clarifying
questions: use `poly2tri` (available, unused until now) for the ground/
asphalt boundary rather than a simpler grid-classification approach;
leaf textures come from ez-tree's own real assets (the reference
project already ported from) rather than a synthesized placeholder.
4-task plan - see Manager/2-plan/.../009-unified-ground/001-planning.md.

- **Task 1: Leaf texture rendering** - real `oak.png`/`pine.png` leaf
  sprite textures (1024x1024, real alpha transparency) fetched directly
  from `github.com/dgreenheck/ez-tree` (MIT license, confirmed via its
  own repo `LICENSE`) - the same reference project `VegTreeSkeletonGenerator`/
  `VegMeshBuilder` already port algorithms from, matching this game's
  own existing `VegTreePreset::Oak`/`Pine` exactly.
  - **A real download pitfall, caught immediately**: the first fetch
    attempt (plain `raw.githubusercontent.com` URL) produced a 131-byte
    text file, not an image - the ez-tree repo stores its own texture
    assets via Git LFS, so the "raw" URL returns an LFS *pointer* file
    (`version https://git-lfs.github.com/spec/v1\noid sha256:...`), not
    the actual binary. Fixed by fetching via GitHub's own LFS media
    endpoint (`media.githubusercontent.com/media/<owner>/<repo>/<branch>/<path>`)
    instead - confirmed real PNG data both ways (`file` command, then a
    `PIL`-based alpha-channel check) before trusting the download.
  - **Asset path convention corrected before it could bite**: `Catalog.h`'s
    own established `"share/sweetsandy/..."` path convention is relative
    to the U++ *nest root* (`/i/Dev/ai-upp/share/...`, the same
    directory every other package's own shared assets - fonts, icons,
    models - already live under), **not** `game/SweetSandy/share/...`
    as first (wrongly) assumed by habit from where the game's own
    source lives. Caught by checking for an existing top-level `share/`
    convention before trusting the guess, not discovered later as
    "the texture never loads at runtime."
  - **U++'s own `RGBA` byte layout matters for the GL upload**:
    `uppsrc/Core/Color.h` defines `RGBA` as `{b,g,r,a}` in memory
    outside `PLATFORM_MACOS` (not `{r,g,b,a}`) - `LoadGLTexture`
    (`GLPrimitives.h/.cpp`, this project's first real texture-mapped GL
    content) uploads with `glTexImage2D(..., GL_BGRA, ...)`, matching
    `Image::Begin()`'s own real byte order directly, instead of
    manually re-swizzling every pixel - caught by reading `Color.h`
    before writing the upload call, not discovered later as a
    color-channel-swapped-leaves bug.
  - `VegTreeInstance` gained a `preset` field (which leaf texture to
    bind - not derivable from `bark_tint`/`leaf_tint` alone, which are
    randomized *within* a species, not a species identifier) -
    `CITY_GEN_VERSION` bumped `cgv7`→`cgv8` (another `WorldChunkContent`-
    nested schema change, same treatment every earlier one got).
    `SweetSandy.h`'s own include order needed `VegTreePresets.h` moved
    before `VegTreeInstance.h` (the enum default member initializer
    needs the full enum definition, not just a forward declaration) -
    a real, if small, ordering fix caught immediately by the build.
  - `DrawFlatTintedMeshGL` (Buildings phase's own shared draw helper)
    gained an optional `GLuint tex` parameter - `GL_ALPHA_TEST` (hard
    cutout, `glAlphaFunc(GL_GREATER, 0.5f)`) rather than `GL_BLEND` -
    no back-to-front sort needed for overlapping leaf quads in this
    immediate-mode renderer, simpler and more robust. `tint` still
    multiplies the texture's own color (default `GL_MODULATE` texture
    env), so per-tree tint variety survives on top of the real texture.
    Buildings/branches keep passing no texture (`tex=0`) - unaffected.
  - Verified: `RunLeafTextureSelfTest` (wired into `--selftest`, no GL
    context needed - pure `StreamRaster` decode) confirms both PNGs
    decode with real alpha (`0` and `255` both present, not a
    fully-opaque fallback silently masking a failed alpha decode);
    `VegTreeInstance::preset` round-trips through
    `WorldChunkContent::Serialize`/`Deserialize`; a framebuffer capture
    viewed directly shows visibly more detailed leaf-cluster silhouettes
    than the flat green blobs before. Full existing regression suite
    green.

- **Task 2: Tree/building placement avoidance** - fixes trees rendering
  inside buildings. `WorldBuilder::Update`'s own generator call order
  changed from `city_gen → road_gen → veg_gen → building_gen` to
  `city_gen → road_gen → building_gen → veg_gen` (buildings only ever
  read `blocks`/`road_centerlines`, never vegetation, so the reorder
  changes nothing about buildings themselves) - `VegChunkGenerator`
  gained a `kBuildingAvoidClearance` rejection check against
  `content.buildings`, the same conservative footprint-bounding-circle
  test `PlanBuildingsForBlock`'s own building-to-building overlap check
  already uses (Buildings phase, Task 1). `CITY_GEN_VERSION` bumped
  `cgv8`→`cgv9` (old cached chunks may have trees placed inside
  buildings - the exact bug being fixed - so must not be served again).
  - **A real move-semantics bug, caught immediately by the build**: a
    new self-test's own hand-built scenario did
    `hand_content.buildings.Add(pick(b))` (matching `BuildingChunkGenerator`'s
    own established pattern - `BuildingInstance` is not copyable, since
    it transitively holds `Vector<VegMeshVertex>` members through its
    own `VegMesh walls`/`roof` fields, and `Vector<T>`'s own copy
    constructor is deleted in this U++ tree) but then kept reading
    `b.width`/`b.depth`/`b.position` *after* the `pick()` move - a
    real use-after-move bug, caught by the compiler on the first
    attempt (`Vector<T>::Add(const T&)` needing a copy ctor that
    doesn't exist), fixed by capturing the needed values into plain
    local variables *before* the `Add(pick(b))` call, not by reading
    the moved-from object afterward.
  - Verified: a hand-built scenario (one large, known building placed
    dead center in an otherwise-plain chunk) - every resulting tree
    clears it, a direct, non-vacuous demonstration of the rejection
    rule itself, not just "no violations happened to occur" on real
    data; a real-pipeline check (`MapGeneratorCityLayoutGenerator` →
    `BuildingChunkGenerator` → `VegChunkGenerator`, matching the real
    `WorldBuilder` order) confirms no real generated tree lands inside
    or flush against any real generated building; the existing
    determinism check's own second (`content2`) pipeline run updated to
    include the same `building_gen2` step, or it would have failed
    spuriously (content2 never rejecting any tree, since it never saw
    any buildings) - caught before it became a flaky/wrong test, not
    after. A framebuffer capture viewed directly confirms trees
    visually clear of a nearby building's own footprint. Full existing
    regression suite green.

- **Task 3: Ground-fill mesh generation** - `GroundMeshBuilder.h/.cpp`:
  a grass mesh that shares vertices with the real asphalt edge, using
  `poly2tri` (`uppsrc/plugin/poly2tri`, this project's first real use
  of it) for constrained Delaunay triangulation of each city block,
  inset inward by half `kRoadWidth` (promoted into `RoadMesh.h`, same
  "second real consumer" pattern used throughout this whole phase),
  with interior Steiner points sampling `Terrain::HeightAt` so a large
  open block still shows real hill shape. `CityPolygonAverage` promoted
  from `PlayScreen.cpp` into `SandyMath.h` alongside the phase's own
  earlier promotions.
  - **A real correctness bug, caught by this task's own hand-computable
    self-test, not shipped**: the first `InsetBlockPolygon` used one
    averaged tangent per vertex (the same centered-difference idea
    `RoadMeshBuilder::BuildRibbon` already uses for its own *open*
    ribbon, which never needs a real corner) offset straight by
    `inset_distance` - undershoots badly at every real corner (for a
    square's own 90-degree corner, the averaged tangent points along
    the diagonal, so the offset only moves each axis by
    `inset_distance/sqrt(2)`, not the full amount). A 20x20 square
    inset by 2m must land at exactly `(2,2)`-`(18,18)` - it didn't,
    caught immediately. Fixed with the standard mitered polygon inset:
    per-*edge* inward normals, per-*vertex* offset along the bisector
    of its two adjacent edge normals, magnitude
    `inset_distance/cos(half the angle between them)` - re-derived by
    hand for the same square case afterward and confirmed exact.
  - **A real crash, not just a wrong result, found by this task's own
    "against real generated blocks, not just hand-built ones" check**:
    feeding real 18-43-vertex curvy blocks through the pipeline
    **segfaulted** inside `poly2tri` - it assumes a clean simple
    polygon and doesn't defend itself against near-duplicate consecutive
    points (a real possibility - `CityGraph` nodes can legitimately sit
    very close together) or genuine self-intersection at a sharp/reflex
    corner (which the original area-floor-only check didn't actually
    detect - a bowtie-shaped result can still have "reasonable" area).
    Fixed with two checks running *before any `p2t::Point` is
    allocated*, in both `InsetBlockPolygon` and defensively again at
    `Build`'s own entry point: dedup consecutive points closer than
    `0.1m`, and a real `O(n^2)` simple-polygon test
    (`IsSimplePolygon`) over all non-adjacent edge pairs - not just the
    area floor (kept as a cheap early-out, no longer the only gate). A
    rejected block now falls through cleanly to Task 4's own
    outside-blocks flat fallback instead of crashing the process.
  - **Another BLITZ anonymous-namespace naming collision, same category
    as the Buildings phase's own `kFuzzyRadius` one**: adding
    `GroundMeshBuilder.cpp` shifted BLITZ's own merge grouping again,
    turning `VegChunkGenerator.cpp`'s own `kGridSpacing` (tree grid,
    `14.0`) and `BuildingPlacement.cpp`'s own `kGridSpacing` (building
    grid, `8.0`) into a real compile error - unlike `kFuzzyRadius`
    these are genuinely different constants, so the fix is renaming
    both (`kVegGridSpacing`/`kBuildingGridSpacing`), not merging.
    Worth remembering as this project keeps growing: a short, generic
    constant name reused across separate anonymous namespaces is a
    latent BLITZ-merge risk regardless of whether today's values
    happen to agree.
  - Verified: hand-computable block inset (exact, re-derived by hand
    after the bug fix); a concave L-shape inset without self-
    intersecting; a thin-spike polygon and a hand-built bowtie quad
    both correctly rejected (not crashed); poly2tri triangulation area
    sanity (sums to the boundary's own known area); real, non-trivial
    height variation from `Terrain`'s own procedural fallback across
    interior Steiner points; real generated blocks (10/16 in one real
    chunk produce a valid ground mesh, 6 cleanly rejected, no crash, no
    degenerate triangles). Full existing regression suite green.

- **Unified Ground, Task 4 (WorldBuilder/PlayScreen integration, phase
  complete)**: `WorldChunkContent.ground` (`Vector<VegMesh>`, one per
  block plus a fallback fill mesh) and a new `IGroundGenerator`/
  `GroundChunkGenerator` (`WorldBuilder::Update`'s own generator order:
  `city_gen -> road_gen -> building_gen -> veg_gen -> ground_gen`, last
  because it's the only one that needs `content.blocks` *and* doesn't
  feed anything downstream). `PlayScreen::StartRun` no longer calls
  `terrain.BuildOdeHeightfield`/relies on `terrain.DrawGL()` at all -
  `Terrain` is now purely a height *function* (`HeightAt`), read by
  `GroundMeshBuilder`/`GroundChunkGenerator` for the visible mesh and by
  a new per-chunk `ground_colliders` heightfield lifecycle in
  `PlayScreen::Tick` (mirrors `veg_colliders`/`building_colliders`'s own
  incremental create-on-load/destroy-on-unload pattern, one 9x9-sample
  `PhysCreateHeightfield` geom per chunk instead of one fixed geom for
  the whole session) for physics. `GLPaint` draws `DrawGroundGL(city)`
  in place of `terrain.DrawGL()`; `DrawCityGL`'s own asphalt draw is
  unchanged. This is the direct fix for the user's own mid-implementation
  report: "Heightmap ei ole tarkoitettu näkyviin enää... Emme halua
  asfaltia ja heightmappia yhtäaikaan" - confirmed via `--groundpic`
  (new diagnostic, same shape as `--buildingpic`) that the old rolling
  terrain no longer renders at all.
  - **A real coverage bug, caught by this task's own real-block
    self-test, not shipped as-is**: the first `GroundChunkGenerator`
    fallback-fill classified each 10x10m candidate cell by requiring
    *all 4 corners* clear of every road/block before filling it (one
    separate `VegMesh` per cell) - at this project's ~15m street
    spacing, a road merely grazing one corner voided the *whole* cell.
    `RunGroundChunkGeneratorSelfTest`'s own real-chunk coverage check
    caught it numerically (only 37.7% of a real 100x100m chunk got any
    ground mesh at all), and `--groundpic`'s own captured frame showed
    it directly as large sky-blue voids between grass patches - exactly
    the "tulee ruma jos ei yhtään mieti mitä asfalttien välillä on"
    failure mode the user's own clarifying answer warned about. A
    smaller 5m/center-point-only version only got to 54.7% - still
    visibly gappy. Fixed properly by giving `GroundMeshBuilder::
    BuildFlatGroundGrid` an optional per-cell `Function<bool(Vec2)>`
    clearance predicate, called once per cell at its own center *inside
    one shared mesh* for the whole chunk (no more "reject the whole
    cell's separate VegMesh" cost to shrinking further) - the real
    fallback now classifies at 1m resolution, so any one misclassified
    cell wastes at most 1m², and one merged mesh replaces what used to
    be dozens/hundreds of separate `VegMesh` draw calls. Real-chunk
    coverage went to 59.6% (the remaining ~40% is legitimate: real
    asphalt plus a documented residual - see below), confirmed visually
    gap-free except one small sliver.
  - **A remaining known limitation, not chased further this task**: a
    narrow median strip inside a divided/dual-carriageway junction can
    still show a small gap in `--groundpic`'s own captured frame - the
    kind of sliver-shaped "block" `GroundMeshBuilder::InsetBlockPolygon`
    already documented as expected to fail (collapses to near-zero
    area) *and* one whose entire width sits within `kRoadWidth*0.5` of
    a bounding road centerline on every side, so the coarse fallback's
    own simple distance-based clearance test also excludes it. This is
    the same class of gap Task 3's own scope note already flagged
    ("block inset, not a full road-network boundary trace/union - a
    deliberate, documented first-cut scope call"), not a new regression
    - a real fix would need actual polygon union/boolean of the road
      ribbon footprint against the block set, out of scope here.
  - Physics: chose a per-chunk `PhysCreateHeightfield` (not a real
    trimesh - `PhysBackend.h` has no trimesh collider, and none is
    needed since no per-material friction is modeled) over trying to
    reuse the visible mesh's own triangulation, so the collider stays
    simple/cheap and height-continuous with `Terrain::HeightAt`
    regardless of whatever the visible mesh's own asphalt/grass
    boundary looks like.
  - Verified: full regression suite (`--selftest` including the two new
    tests above plus a far-from-origin `GroundColliderTest` spawning at
    world (150,150) - well outside `Terrain`'s own old fixed 100m
    half-width - and settling on the new per-chunk collider without
    falling through; `--geomtest`; `--gltest`; `--menutest`;
    `--playtest`, which now logs a real non-zero `ground_meshes` count
    alongside `--playtest`'s existing chunk/road/block/building counts)
    all green; `--groundpic` framebuffer capture confirmed visually:
    `Terrain::DrawGL`'s old rolling terrain is completely gone, grass
    fills block interiors with real height variation and meets the
    asphalt edge with no double-rendering, trees no longer render
    inside buildings (Task 2's own fix, re-confirmed visually here).

- **Island Terrain, Task 1 (noise math foundation)**: new `TerrainNoise.h`/
  `.cpp` - `SimplexNoise2D` (standard 2D simplex noise, Gustavson's
  algorithm, seeded via a real Fisher-Yates shuffle over a
  `RandomNumberGenerator` instance - deliberately *not* Core's own
  `RandomNumberGenerator::Permutation`, which just fills each slot with
  an independent `Random(num)` draw and can repeat/omit values, wrong
  for a simplex permutation table which needs a true bijection) plus
  `OctavedSimplex`/`OctavedRidge` fractal sums on top. Algorithm (not
  code) ported from `TLabAltoh/realistic_terrain`'s own `noise.py` -
  that repo is a Blender add-on + DirectX11 compute shader, neither
  portable as-is; see Manager/2-plan/.../010-island-terrain/
  001-planning.md for the full research writeup, including the
  reference's own (non-standard, swapped) frequency/lacunarity naming
  this port deliberately does not replicate.
  - `Core/MathNumeric/Random.h` (`RandomNumberGenerator`) isn't pulled
    in by `SweetSandy.h`'s own include chain even though the package is
    already a transitive build dependency (confirmed via the link
    line) - needed an explicit `#include <Core/MathNumeric/MathNumeric.h>`
    in `TerrainNoise.cpp` itself.
  - Verified: determinism (same seed -> identical output, different
    seed -> different); empirical range sanity over a real sample grid
    (measured [-0.997, 0.998], not just "should be near [-1,1]");
    continuity (no discontinuity at epsilon-separated points - the kind
    of indexing bug a single-point test would miss); more octaves
    measurably increase output variance; `OctavedRidge` is both
    visibly sharper (higher second-difference variance along a
    scanline) and has the expected upward-biased mean (ridge's own
    `1-|n|` definition should pull values toward 1, not center near 0
    like plain noise - confirmed: plain mean ≈ -0.03, ridge mean ≈
    0.47). Full regression suite green.

- **Island Terrain, Task 2 (island height function)**: new
  `IslandTerrain.h`/`.cpp` - `IslandTerrain::HeightAt` composes Task 1's
  `OctavedSimplex`/`OctavedRidge` (blended, matching the reference's own
  `(simplex+ridge)/2` default) with a Chebyshev-distance falloff map and
  a `[water_level, max_height]` clamp into one **stateless** function -
  no baked grid, no storage, safely evaluable at any `(x,z)` - this is
  what actually fixes "flattens outside the old square" for open-world/
  island mode (there's no "outside" for a pure function).
  - **A real deviation from the reference, deliberate not accidental**:
    the reference normalizes its own output via `(h-h.min())/(h.max()-
    h.min())` against a *finite generated grid's own* extrema - doesn't
    exist for a function that must answer a single point query with no
    pre-scan. Uses a fixed, tunable `height_scale` (150m default)
    instead - documented tradeoff, not silently different behavior.
  - The reference's own general falloff formula
    (`1 - t^a/(t^a+(b-b*t)^a)`) reduces algebraically to a plain `1-t`
    at its own default parameters (steepness=1, offset=1) - implemented
    directly as that simpler form.
  - **A real, honest false start while writing the self-test**: first
    tried "local height variance near the island center should be
    comparable to variance near the island's own far edge" - failed
    (104 vs. 0.2), but not because of a bug: `h = noise * falloff`
    means falloff shrinks *relief itself*, not just the mean, so
    terrain legitimately taper toward the coast (a real, correct
    island-shaping property, confirmed by hand-deriving the expected
    falloff*noise scaling). A second attempt (raw pre-falloff variance
    near origin vs. 1000km away) was also too statistically noisy at
    121 sample points to be reliable (12x difference, neither zero nor
    obviously broken). Replaced both with the actually-robust check:
    the same epsilon-step continuity property Task 1's own test already
    uses, re-run at 1000km from the origin - a real precision breakdown
    would show identical-or-discontinuous values, not just "different
    but still locally smooth" (confirmed: max delta ≈ 4.5e-8 at 1000km,
    same order as near the origin).
  - Verified: falloff hand-computable at center (1.0), island radius
    (0.0, including the Chebyshev corner case), and beyond (-1.0 at 2x
    radius); water-level clamp exact far from the island; island center
    measurably higher than far beyond its radius; continuity holds at
    1000km from the origin; determinism across separate same-seed
    instances. Full regression suite green.

- **Island Terrain, Task 3 (water plane)**: new `DrawWaterGL`
  (`GLPrimitives.h`/`.cpp`) - an unlit, translucent (`GL_BLEND`) quad at
  a fixed world `y=water_level`, re-centered under the camera every call
  (not a fixed-size world object) so it always reaches the horizon.
  Depth-tested but doesn't write depth (standard translucency practice).
  New `GLDebugWater`/`--watertest` diagnostic (same isolated-scene shape
  `GLDebugQuad`/`--gltest` already established) proves it's a *real*
  blend, not just "some pixels drew": draws a known opaque "seafloor"
  color first, the water plane over it, then samples one pixel
  (`GLReadPixel`, new small helper in `GLDebug.h`/`.cpp`) at the known
  overlap point and asserts every RGB channel sits strictly *between*
  the two source colors - equal to either pure color would mean no
  blend happened at all. `--watertest [path.png]` also saves a
  framebuffer capture; made the seafloor deliberately smaller than the
  water plane's own footprint (12m vs. 60m radius) after the first
  capture came back as one solid blended color filling the whole
  frame - technically correct but not visually informative; shrinking
  it now shows a clear boundary between "water over background" and
  "water over seafloor".
  - Verified: `--watertest` object-pixel presence, real numeric
    channel-wise blend at the sampled overlap point (measured overlap
    `(79,112,156)` strictly between seafloor `(150,120,70)` and water
    `(60,110,180)` on every channel), framebuffer capture confirmed
    visually. Full regression suite green (this task only adds a new
    mode, touches nothing the existing suite exercises).

- **Island Terrain, Task 4 (WorldBuilder/PlayScreen integration, phase
  complete)**: `Terrain` extended, not replaced/paralleled - gains one
  member (`One<IslandTerrain> island`, set by `Terrain::Load` for a new
  `TERRAIN_ISLAND` `TrackDef` kind instead of building a baked grid).
  `HeightAt` branches on it; `NormalAt` needed **zero changes** (already
  just calls `HeightAt` four times for a finite-difference normal, so
  it's automatically correct for both modes). `FlattenUnderRibbon`/
  `BuildOdeHeightfield` become no-ops in island mode. Net effect: every
  existing generator (`RoadCreatorMeshGenerator`, `VegChunkGenerator`,
  `BuildingChunkGenerator`, `GroundChunkGenerator`) works on the new
  `"island"` track (100km world_size, 150m height_scale) completely
  unchanged - the whole reason to extend `Terrain` instead of adding a
  parallel type. New `"island"` catalog entry shows up in New Game
  automatically (`NewGameScreen` already iterates the catalog).
  `PlayScreen::GLPaint` draws `DrawWaterGL` (Task 3) gated on the new
  `Terrain::IsIsland()`, after every opaque draw.
  - **A real, hard-to-find bug, root-caused via targeted temporary debug
    logging (added, used, then removed - not shipped)**: the vehicle
    spawned at the world origin appeared frozen - `key_throttle`/
    `in.throttle` confirmed true every tick via one debug log, yet x/z
    barely moved while y slowly, accelerating-ly dropped (classic
    partial-freefall, not a normal suspension settle). A second debug
    log dumping each nearby ground_collider's own chunk bounds/height
    range revealed the real cause: world origin (0,0) sits *exactly* at
    the shared corner of **four** separately-built per-chunk
    `ground_colliders` (LOD0 `cell_size=30` always puts a chunk corner
    at the origin) - a car parked exactly there gets inconsistent
    per-wheel collision across those four independently-sampled
    heightfields (each wheel can land on a different tile). The small
    tracks' own gentle relief (height_scale=12) made the same corner
    seam small enough to never surface this; island's much steeper
    terrain (height_scale=150) made it obvious. Fixed narrowly:
    `PlayScreen::StartRun` now spawns at `(5,·,5)` instead of exactly
    `(0,·,0)` - a transient boundary crossing *while already moving* is
    a different, harmless case (well under a car-length's travel time),
    so this is not a general per-chunk-collider redesign, just "don't
    park exactly on a chunk-grid corner."
  - **A second, real (if expected) finding from the visual capture, not
    a bug**: `--islandpic`'s own framebuffer captures show the vehicle
    progressively rolling over while driving on the catalog's real,
    dramatic terrain (150m height_scale, un-eroded ridge noise) - the
    raw, un-eroded terrain this phase's own noise+falloff composition
    produces can be genuinely too steep for the stock vehicle tuning at
    speed. This is exactly why a later "terrain erosion" phase exists
    (per the user's own answer) rather than something to chase down
    here - confirmed already anticipated by `RunIslandDrivingSelfTest`'s
    own design (see below), which deliberately drives on a much gentler
    height_scale to isolate "does the physics/height-function
    integration work" from "can the vehicle handle alpine terrain".
  - New `RunIslandDrivingSelfTest` (headless, CPU-speed step loop, not
    real-time/GUI): a real `PhysicsWorld`+`Vehicle`+`Terrain` (island
    mode, but a local `TrackDef` copy with `height_scale=25` for the
    reason above) drives 500m+ on a real heightfield collider built
    from `IslandTerrain::HeightAt` directly, confirming it doesn't fall
    through and settles with genuine relief nearby at the end -
    initially tripped on two more of its own bugs before that:
    (a) the drive corridor was built long-in-x/narrow-in-z, but the
    vehicle's own forward axis is +z (`Vehicle.cpp`'s own hub_offset
    convention) - the car immediately drove off the corridor's own
    narrow *z* band into empty space, an easy "chassis fell through"
    misdiagnosis until logging `cp.x` showed it walking far outside the
    corridor instead of the expected forward travel; (b) even widened
    to the correct axis, real uneven terrain yaws/drifts the car
    sideways over a long drive with zero steering input (confirmed: -24m
    by 5s, -103m by 10s in a 40m-wide corridor) - widened to 400m.
  - New `--islandpic` diagnostic (same shape as `--lodpic`), started on
    the `"island"` track specifically.
  - Verified: `RunIslandDrivingSelfTest` green (finite positions
    throughout; loose 300m mid-drive / 15m final-settle bounds - not a
    tight per-tick bound, since real airtime over real hills/ridges is
    expected and not itself a bug, same philosophy
    `RunObstacleCourseSelfTest` already uses for its own jump; real
    relief confirmed near the final position); `--islandpic` framebuffer
    captures confirmed visually: real terrain relief following real
    slopes (buildings stand upright against a visibly tilted ground -
    correct), roads/city rendered continuously, no flat plateau
    anywhere; `--lodpic`/`--groundpic` (small tracks) re-confirmed
    unaffected by the spawn-position change. Full regression suite
    (`--selftest` with the new test, `--geomtest`, `--gltest`,
    `--watertest`, `--menutest`, `--playtest`) green.
  - **Known, explicitly out-of-scope limitation** (documented in the
    task doc before implementation, not discovered after): city/road
    generation has zero terrain awareness and will keep generating
    streets across what the island height function clamps to open
    ocean - the "terrain-aware city/road placement" initiative
    `001-planning.md` already named as a separate, later, multi-phase
    undertaking, not attempted here.

- **Terrain Erosion, Task 1 (heightmap tile baking + disk cache)**: new
  `ErosionTile.h`/`.cpp` (`ErosionTile` - a finite `257x257` baked grid,
  `2048m` square, `ErosionTileCoord`/`ErosionTileCoordFor`/
  `ErosionTileOrigin` for the tile-index<->world-space mapping,
  `BakeErosionTile` materializing `IslandTerrain::HeightAt` into it) and
  `ErosionTileCache.h`/`.cpp` (same `DiskBlobCache`-backed, seed-keyed
  shape `WorldChunkCache` already established). Algorithm (droplet-
  based hydraulic erosion) ported from `SebLague/Hydraulic-Erosion`
  (MIT) - unlike the Island Terrain phase's own reference, this one's
  actual simulation core (`Erosion.cs`) is plain, dependency-free C#,
  confirmed by reading the full file - no compute shader needed for a
  first CPU port. No erosion applied yet - that's Task 2; this task
  only proves the bake/cache/reload shape.
  - Verified: tile coordinate math hand-computed at known points
    (including negative coordinates, since the island spans both signs
    around its own center); every baked grid sample matches
    `IslandTerrain::HeightAt` exactly at that point; `ErosionTile::
    HeightAt`'s own bilinear sampling has no discontinuity at epsilon-
    separated points; a real disk cache round-trip (bake, save, load
    into a fresh instance, every sample matches exactly - not
    approximately, since this is a lossless float array) plus a
    never-saved coordinate correctly misses. Full regression suite
    green.

- **Terrain Erosion, Task 2 (droplet erosion algorithm port)**: new
  `HydraulicErosion.h`/`.cpp` - `ErodeHeightmap`/`SimulateOneDroplet`
  ported directly (algorithm and structure, not just referenced) from
  `SebLague/Hydraulic-Erosion`'s own `Erosion.cs` (confirmed plain,
  dependency-free C# - no compute shader needed for a first CPU port).
  One deliberate simplification: the reference's own brush precompute
  only recomputes near a map edge and *reuses* the last-computed shape
  for interior cells (correct only because of a subtle row-major-
  iteration-order invariant) - this port always computes every cell's
  own brush directly instead, same output, no reliance on that
  invariant, not worth the fragility for a one-time-per-tile bake.
  - **A real, confirmed scale mismatch between the reference and this
    project's own data**: the reference's default `gravity=4` (used in
    `speed = sqrt(speed² + deltaHeight*gravity)`) implicitly assumes
    small per-step height deltas (a heightmap normalized to a small
    range) - this project's tiles carry real absolute meters
    (`IslandTerrain`'s own `height_scale`, up to 150). A first
    self-test using a deliberately steep hand-built slope (1.0 height
    drop per grid step) immediately drove `speed*speed +
    deltaHeight*gravity` negative on the very first step, which (with
    this port's own added `max(0.0, ...)` guard - the reference's own
    formula has no such guard and would produce NaN there) collapsed
    `speed` to 0, permanently pinning `sedimentCapacity` at its own
    tiny minimum for the rest of that droplet's life - erosion/
    deposition still ran, but at a negligible, wrong-looking magnitude.
    Fixed the *test* (not the algorithm) to use a gentler slope
    matching the reference's own implied scale, confirming the ported
    algorithm itself is correct; confirmed separately (with real
    `IslandTerrain`-baked data, real height_scale=150) that the added
    NaN-guard does its job - no non-finite values, but likely still
    triggering the same "capacity collapses to the minimum" dampening
    the reference was never tuned for. Real-data *parameter* tuning
    (not a further algorithm change) is explicitly left to Task 3,
    where it can be judged against an actual visual result.
  - Verified: brush weights sum to 1 at an interior point; a hand-built
    gentle slope (steep-then-flat) both net-erodes near the top and
    net-deposits in the flat valley - the actual "erosion happens where
    expected" check; approximate mass conservation over a real
    `ErodeHeightmap` run (~0.2% loss, consistent with a few droplets
    exiting the map edge still carrying sediment); 20000 droplets over
    a real `257x257` island tile in 601ms with zero non-finite values;
    determinism confirmed on that same real tile. Full regression suite
    green.

- **Terrain Erosion, Task 3 (pipeline wiring)**: `Terrain` gains an
  in-memory `VectorMap<ErosionTileCoord, ErosionTile>` (never evicted -
  ~264KB/tile, a non-issue at this project's scale) plus a disk-backed
  `ErosionTileCache`; `HeightAt` (island mode) prefers a loaded eroded
  tile over `island`'s own raw function wherever one covers the query
  point. New `ITerrainErosionGenerator`/`ErosionChunkGenerator` -
  `WorldBuilder::Update`'s own **first** stage now, before any stage
  that reads `HeightAt` - `EnsureErosionCoverage` checks all 4 corners
  of a chunk's own AABB (not just its center), since a `WorldChunk` can
  straddle an erosion-tile edge. Retuned `gravity` (0.2, down from the
  reference's own `4.0`) for this project's real meter-scale terrain,
  verified via a roughness-reduction measurement (below), not derived
  on paper alone.
  - **A real design bug, caught before it shipped**: the erosion stage
    was first placed *inside* `WorldBuilder::Update`'s own cache-miss
    branch (alongside `city_gen`/`road_gen`/etc.) - meaning a chunk
    whose *content* was already cached on disk from an earlier session
    would skip erosion-tile loading entirely, silently serving raw
    (un-eroded) heights to every live `HeightAt` caller (e.g.
    `PlayScreen::Tick`'s own per-chunk `ground_colliders`, sampled
    every tick, not just at generation time) even though the cached
    mesh content itself reflected the eroded ground from when it was
    first generated - a real content/physics mismatch. Fixed by moving
    the erosion stage *outside* the cache-hit/miss branch, running
    unconditionally for every newly-loaded chunk.
  - **A real, measured performance finding, not a bug**: baking+eroding
    one tile takes ~1s (50000 droplets); the world origin sits exactly
    on a 4-way erosion-tile-grid corner (same root cause as Island
    Terrain Task 4's own physics-collider corner bug, a different
    subsystem hitting the identical coordinate), so a cold cache's very
    first tick can synchronously bake up to 4 tiles (~4.4s measured) -
    and because this app is single-threaded, that stalls the entire GUI
    event loop, including `--islandpic`'s own capture callbacks, which
    then fire in a bunched burst right after baking finishes rather
    than at their nominal times. Not fixed here - matches this
    project's own long-standing, explicitly-deferred "background
    generation is future work" stance (already the case for city/road/
    vegetation/building content); documented in `--islandpic`'s own
    comment, with "run the same command twice - the second, warm-cache
    run shows real driving" as the practical verification method.
  - Verified: roughness (mean adjacent-cell height delta) over a real
    tile measurably dropped after erosion (`0.4786 -> 0.4378`, ~8.5%);
    a chunk straddling a 4-way tile-grid corner loads exactly 4 tiles,
    not 1; 64/64 sampled points differ between eroded and raw
    `HeightAt` (confirms erosion is actually wired into the live path,
    not just present somewhere unreachable); `--islandpic`/`--lodpic`
    re-confirmed working (the latter untouched by any of this). Full
    regression suite green.

- **Terrain Erosion, Task 4 (integration + verification, phase
  complete)**: the real payoff check - a new `RunIslandErosionDrivingSelfTest`
  drives a real vehicle on the catalog's own real `height_scale=150`
  terrain (not Task 2/3's own deliberately-gentler test scale), A/B
  comparing raw vs. eroded at the *exact* spawn point Island Terrain
  Task 4's own `--islandpic` capture found rolling over.
  - **A real, serious bug found by this exact test - not anticipated by
    Task 3's own implementation or self-tests**: each erosion tile is
    baked+eroded fully independently - a droplet's own path never
    crosses into a neighboring tile's own separate grid. Before
    erosion, adjacent tiles agree exactly at their shared edge (both
    sample the same deterministic `IslandTerrain::HeightAt` there);
    erosion then moves material around *within* each tile with zero
    knowledge of what the neighboring tile's own droplets do at that
    same boundary - a real, physical cliff at every tile edge. Task 3's
    own roughness/coverage/determinism checks all sampled well inside a
    single tile's own interior, never near a shared edge between two
    independently-eroded tiles, so nothing there could have caught it.
    Measured directly: a vehicle spawned at `(5,5)` (which sits right
    next to the same tile-grid corner the world origin itself occupies)
    rolled over in **0.14m** with erosion on, vs. clearing the full
    500m test corridor with erosion off - the exact opposite of what
    erosion is supposed to do. Fixed by
    blending each tile's own eroded result back toward the *raw*
    (pre-erosion) height within a margin of its own edge, smoothstepped
    to exactly 0 right at the boundary - since the raw function is the
    one thing guaranteed identical on both sides of any shared edge,
    this guarantees two adjacent tiles meet at exactly the same height.
    New `EROSION_GEN_VERSION` (`ErosionTileCache.h`, same convention as
    `CITY_GEN_VERSION`) invalidates tiles cached before this fix.
  - After the fix: both raw and eroded reach the full 500m test
    corridor at this spot without rolling over (this particular real
    seed/location turned out drivable either way once the cliff bug was
    gone) - re-confirmed directly against the *real* `WorldBuilder`/
    `PlayScreen` pipeline via `--islandpic`, run twice (cold cache, then
    warm - see Task 3's own note on why): the second run shows the
    vehicle driving upright on real sloped terrain, matching the self-
    test's own result.
  - Verified: `RunIslandErosionDrivingSelfTest` green (within a small
    tolerance of raw vs. eroded, both near the 500m cap - not a strict
    `>=`, since two otherwise-identical capped runs can differ by
    centimeters from ordinary physics-timing noise); `--islandpic`
    (cold and warm cache) and `--lodpic` re-confirmed visually/
    numerically. Full regression suite (`--selftest` with the new test,
    `--geomtest`, `--gltest`, `--watertest`, `--menutest`, `--playtest`)
green - the last time this phase needs to say so.

## External Reference Integration

Reference files and plans for external engine integration live in
`Manager/2-plan/ai-upp/game/SweetSandy/`:

- **Phase 013 (`013-tesseract-integration/`)**: Forking Tesseract-Sauerbraten
  engine/game code from `~/Dev/Tesseract-Sauerbraten/src/`. Key files to port:
  `octa.h/.cpp` (octree), `worldio.cpp` (map .ogz format), `animmodel.h`/`
  skelmodel.h` (skeletal animation for pedestrian models), `rendergl.cpp`/
  `rendermodel.cpp` (rendering pipeline). **None usable directly** - all C++
  stdlib code must convert to U++ types (`Upp::String`, `Upp::Vector`, etc.).
  Asset sources: `packages/models/` (ogre/monster models for pedestrians),
  `packages/map/*.ogz` (octree maps as sub-octrees embedded in natural terrain).
  GUI code (`menus.cpp`, `ui.cpp`), scripting (`command.cpp`), multiplayer
  (`client.cpp`, `server.cpp`) are **not forked** - we use U++ CtrlLib GUI,
  Python-syntax Eon scripting, and our own ENet-based networking.

- **Phase 014 (`014-input-system/`)**: New `GameInput` class in
  `uppsrc/CtrlCore/` for gamepads/joysticks and game keyboard input. No
  SDL2 - uses native XInput (Windows), evdev/js (Linux), IOKit (macOS).
  Replaces GUI-focused `Ctrl::Key()` model with polling-based game input.

- **Phase 015 (`015-networking/`)**: Integrates Tesseract networking protocol
  with existing ENet fork (`uppsrc/plugin/enet/`). Maps message types to U++
  serialization. TCP for `.ogz` map file transfer, ENet/UDP for real-time
  game state sync.

- **Phase 016 (`016-in-game-gui/`)**: **Deferred** - rendering U++ CtrlCore
  widgets inside 3D world space. Difficult problem of `Ctrl::Paint(Draw&)`
  bridging to OpenGL texture, with 3D world GUI using Access/Visit/Visitor/Vis
  mechanism from `docs/gates/gui-user-gate.md`. Two-phase: 2D HUD first,
  then 3D world GUI.

### Integration Architecture Summary

```
+-------------------+     +-----------------+     +-------------------+
|  Natural Terrain  |     |  Sub-Octrees    |     |  Vehicles/Peds    |
|  (IslandTerrain)  |     |  (.ogz embedded)|     |  (PhysBackend)    |
+---------+---------+     +--------+--------+     +--------+----------+
          |                        |                       |
          |                        v                       v
          |               +--------+--------+     +--------+----------+
          |               | WorldBuilder    |     | GameInput         |
          |               | (streaming)     |     | (gamepad/keyboard)|
          |               +-----------------+     +-------------------+
          |
          v
+-------------------+
|  GroundMeshBuilder|
|  (flattens terrain|
|   under roads)    |
+-------------------+
          |
          v
+-------------------+     +-----------------+
|  RoadNetwork      |     |  Vegetation     |
|  (ribbon mesh)    |     |  (trees)        |
+---------+---------+     +--------+--------+
          |                        |
          v                        v
+-------------------+     +-------------------+
| Buildings (Phase) |     | AnimModels (Phase)|
| 013 Rt3D          |     | 013 Rt3D          |
+-------------------+     +-------------------+
          |                        |
          v                        v
+-------------------+     +-------------------+
| GLCtrl (PlayScreen)|    | InGameGui (Phase) |
| + CtrlCore GUI    |<----| 016 (deferred)    |
+-------------------+     +-------------------+
          |                        |
          v                        v
+-------------------+     +-------------------+
| ENet (plugin/enet)|     | Game Protocol     |
| TCP/UDP (Phase 15)|     | Map transfer      |
+-------------------+     +-------------------+
```
