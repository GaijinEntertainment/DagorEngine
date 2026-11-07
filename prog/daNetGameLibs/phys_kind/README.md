# phys_kind: network physics kinds in daScript or C++

The `phys_kind` module registers one DNG network physics type, `PhysKind`
(gamePhys, on `PhysType::CUSTOM8`). A new physics kind needs no PhysType slot,
no snapshot message and no registration code: it is an entity template, a setup
system, the handler of `CmdPhysKindStep` (and of `CmdPhysKindApplyState` when
the kind caches values from its state), and an input system on the owner. The
slot keeps the DNG netPhys behaviour: controls, AAS, snapshots, LOD, sleep and
hidden sync, teleport, rewind and replay.

A game that uses it lists `"phys_kind"` in `danetgamelibs` of its
`setup_libs.py`, calls `NEED_MODULES_PHYS_KIND()` in its `pull_game_das()`
(`dasGameModulesList.cpp`, with `#include <phys_kind/pullDas.h>` or
`<daNetGameLibsDasModules.h>`) so that `require DngPhysKind` resolves, and
keeps `CUSTOM8` free for it. Each target that compiles `phys_kind/physKind.h`
lists `$(Root)/prog/gameLibs` in `AddIncludes` for the gameLibs physKind API:
a C++ kind module such as `phys_kind_jolt_vehicle`, and a game or DNG module
whose das AOT code requires `DngPhysKind`.

Worked example: the netPhysTestbed sample, with its scripts in
`samples/netPhysTestbed/prog/scripts/net_phys_testbed`. Its skateboard
(`es/skateboard.das`, `es/skateboard_common.das`) is a daScript kind, its human
slide and zipline are daScript kinds that a HumanPhys human switches to, and
`phys_kind_jolt_vehicle` is a C++ kind whose state lives in a Jolt body.

## Add a daScript kind

A kind script requires `DngPhysKind` (`PhysKindActor`, `phys_kind_*` functions)
and `danetlibs.phys_kind.es.phys_kind_events` (the two events, `phys_kind_ext`),
and `Dacoll` when it traces the world.

1. Template: use `phys_kind_base` and add a tag that names the kind.

       my_kind{
         _use:t="phys_kind_base"
         "my_kind:tag"{}
       }

   `phys_kind__shadowSim:b=yes` on an entity makes its remote copies simulate
   from the last snapshot toward the present, at most 1 s past it, as WT does.
   Without it a remote copy shows the interpolated snapshots and runs no step.

   `phys_kind__snapshotHz:p4=30,20,10,5` sends the snapshots of each LOD
   (`Closest`, `Normal`, `Faraway`, `Min`) at the server frame rate halved up to
   three times, the lowest that keeps that rate, but never above the rate of the
   zone: at 60 frames a second, one in two frames for `Closest` and `Normal`,
   one in four for `Faraway`, one in eight for `Min`. Below the zone rate the
   entities spread over the frames; a `phys__lowFreqTickrate` record keeps the
   zone rule. A shadowSim kind can take low rates; a kind that interpolates
   needs a snapshot in each interpolation delay (2 to 4 ticks).

   A snapshot carries the position in steps of 2 mm (`Closest`, `Normal`), 8 mm
   (`Faraway`) and 3 cm (`Min`), but a remote copy keeps it as a float: 4 mm steps
   past 32 km, 8 mm past 65 km. It carries any speed in steps of 8, 16, 31 and
   63 mm/s; the AAS has steps of 0.25 mm and 1 mm/s. A HIDDEN snapshot has no pose.

2. Kind data: one struct for the state and one for the controls, plain data
   only (no pointers, strings or arrays of variable size), alignment 8 or less.

   - The state struct is at most `PHYS_KIND_STATE_EXT_MAX` (128) bytes. The AAS
     carries all of it. A snapshot carries only a prefix of it, of the size that
     `phys_kind_set_snap_ext_sizes` gives per LOD (`Closest`, `Normal`,
     `Faraway`, `Min`), at most `PHYS_KIND_SNAP_EXT_MAX` (16) bytes. A HIDDEN
     snapshot carries the `Min` prefix. Put first the data that remote copies
     need, for example the controls they must replay. A kind with a state wire
     sends the wire fields of each LOD in place of the prefix.
   - The controls struct is at most `PHYS_KIND_CTRL_EXT_MAX` (32) bytes.
   - Add new fields only at the end: a reader zero-fills bytes that an older
     writer did not send. A wire needs the same fields on both sides.
   - Read and write the data with `phys_kind_ext(state_or_controls, type<T>)`
     from `danetlibs.phys_kind.es.phys_kind_events`. It returns null and logs an
     error when `T` is over the limit.
   - Field annotations let the wire code the data in fewer bits, so the kind
     keeps plain floats and ints (needs `phys_kind_set_wire`, step 3):

         struct MyState
           @net_bits = 10 @net_max = 1.0 steer : float     // -1..1 in 10 bits
           @net_bits = 16 @net_min = 0.0 @net_max = 1.0 t : float
           @net_step = 0.01 @net_lod = "Normal" speed : float
           @net_bits = 3 mode : uint8
           @net_lod = "none" integrator : float            // AAS only

     `@net_bits` (1 to its width) alone on an int of 1, 2 or 4 bytes keeps that
     many bits, clamped to their range, signed for a signed type. A float
     takes `@net_bits` (1 to 24) with `@net_max` (`@net_min` is `-@net_max` if
     left out) in steps from 0, or `@net_step` within +-(2^31 - 1) steps.
     A bool takes 1 bit, a field with no annotation its bytes.
     `@net_lod` gives the farthest LOD whose snapshots carry the field
     (`Closest`, `Normal`, `Faraway`, `Min` if left out, `none`); on a struct
     field it limits its fields. Vectors and fixed arrays code each element.
     Each snapshot LOD fits 128 bits, a `@net_step` field at its most (37 bits).
     `phys_kind_set_wire` logs an error for bits out of these limits.
   - The AAS carries the state data as it is, so the owner replays from the
     exact server values.

3. Setup, `[es(on_appear, REQUIRE=my_kind)]` on `PhysKindActor&
   phys_kind_net_phys`: set `phys.mass` and call `phys_kind_set_snap_ext_sizes`,
   or `phys_kind_set_wire(phys, type<MyState>, type<MyControls>)` from
   `danetlibs.phys_kind.es.phys_kind_wire` for annotated data.
   A kind that other bodies must hit adds a dynamic collision with
   `phys_kind_add_collision_box(phys, tm, size)` (box tm and full size in the body
   frame). The testbed slide traces the world and the zipline follows its cable;
   neither adds one.

4. Step, `[es(REQUIRE=my_kind)]` on `CmdPhysKindStep`: move
   `phys.currentState` (`location`, `velocity`, `omega`, kind data) forward by
   `evt.dt` for tick `evt.tick`.

   - Read the controls from `phys.appliedCT` only when `evt.useControls` is
     true: the server and the owner have controls, a remote copy has none. A
     remote copy that simulates (shadowSim) steps from the kind data that the
     snapshot gave, so keep there the part of the controls it needs.
   - The owner replays ticks after each server correction with `evt.isForReal`
     false. A step must give the same result for the same state and controls,
     and must not send events, play sounds or create entities when
     `isForReal` is false.
   - Collision and traces can use the dacoll world (`require Dacoll`:
     `tracedown_normalized`, `rayhit_normalized`, and others).
   - A kind at rest can call `phys_kind_put_to_sleep(phys)`: it zeroes the
     velocities, and the slot skips the step and sends the snapshots as asleep.
     The step does not run while the kind sleeps, so another system (on the
     game's `CmdPrePhysUpdate`, for example) checks `phys_kind_is_asleep(phys)`
     and calls `phys_kind_wake_up(phys)`. A sleeping owner sends no controls, so
     wake on `producedCT` too, not only on `appliedCT`.

5. Controls, in an input system on the owner client (`tag=input`): write the
   controls struct into `phys.producedCT` each frame. The slot sends them to the
   server and applies them on both sides. A kind with a controls wire: the owner
   snaps them to it before it sends them, so it applies the values that the
   server reads.

6. `CmdPhysKindApplyState` tells a kind that the slot wrote its state: on a
   teleport, a reset and a takeover for every kind. A daScript kind whose state is
   all in `currentState` can ignore it; a kind that caches values from the state
   updates them here.

## Switch an entity between physics types

An entity can change its physics at runtime by a recreate with another net phys
sub-template, for example a HumanPhys human that becomes a slide kind and back
(`es/human_common.das` `switch_physics` in the testbed). Copy `net__physId` from
the new entity in the recreate callback. The slot drops the stale AAS and controls
of the replaced actor; a partial AAS of the next actor can still reach the old one
until the client recreates it.

## C++ and external kinds

A C++ kind handles the same events in an ES. `phys_kind__external:b=yes` marks a
kind whose state lives in an engine body (a Jolt vehicle): it has no rewind and
replay, the owner shifts the body by the server error instead, and
`CmdPhysKindApplyState` carries `keepBodyMotion` for owner corrections. See
`phys_kind_jolt_vehicle`.

A C++ kind describes its data for the wire with the `PHYS_KIND_WIRE_*` macros
of `physKindState_api.h`, the same codecs as the das annotations, and gives
them to `phys.setWire(state_fields, controls_fields)` in its setup:

    static const PhysKindWireField STATE_WIRE[] = {
      PHYS_KIND_WIRE_RANGE(MyState, steer, -1.f, 1.f, 10, PhysKindLod::Min),
      PHYS_KIND_WIRE_INT(MyState, mode, 3, PhysKindLod::Normal)};
    phys.setWire(make_span_const(STATE_WIRE), {});

## Check a kind

Run it in the netPhysTestbed sample, from `samples/netPhysTestbed`:
`run_testbed.bat --lag 100 --jitter 20 --loss 5` builds the client and the
dedicated server, runs `run_testbed.py setup` once (shaders and fonts), starts
them on a bad network, and stops them on a key press; leave out the options for
a clean network. `phys_kind_desync_stats(phys)` and
`phys_kind_owner_snapshot_stats(phys)` give the owner's corrections (processed,
untestable, small, large); the physKindTest unit test covers the wire format.
