Headless benchmark of ECS -> quirrel(frp) state mirroring at HUD-shaped
parameters.

Modes:
  -mode:none        no mirroring (baseline)
  -mode:sq          script entity system writing into a Watched on every change
  -mode:sq_batched  per-eid Watched storage + membership set flushed once per
                    frame - what std/frp.nut mkTriggerableLatestWatchedSetAnd
                    Storage does, i.e. the real production baseline
  -mode:pull        ecs.computed: the ES copies the changed rows into a raw
                    cache at event time; the script value is built from that
                    cache during the frp update, and only while consumed

Shapes:
  -shape:single     one bench_hero, 7 components (jetpack-hud sized)
  -shape:map        {eid: row} over N bench_bot, 2 components per row

Parameters:
  -entities:N -changes:M -subscribed:0|1 -frames:N
  -recreate:N   map shape only: destroy and respawn one entity every N frames,
                so row removal and re-adding are exercised
  -derived:N    single shape only. N observables derived from the mirrored row,
                the way a HUD builds its own values on top of one state row: in
                pull mode Computeds over the mirror, in sq mode derived inline by
                the entity system and pushed into N Watched. Both run the same
                derivation list (the jetpack HUD's six, cycled), so the only
                difference measured is who builds the row and when the
                derivations run. With -derived:N, -subscribed: is how many of the
                N have a consumer, not a flag: a HUD whose block is hidden still
                watches the value that decides visibility and nothing else
  -filter:M     pull mode only, M is off|churn|pass|native|script. Compares a
                query filter against the same selection written in quirrel; the
                four non-off values write the same components, so they are
                comparable to each other. See the filter block at the end.
                The map shape filters on bench__alive, which -filter:native does
                not mirror, so rows enter and leave membership without any
                mirrored value changing; the single shape filters on a component
                it does mirror, and reads as defVal while rejected
  -samewrites:1 map shape with -filter:pass only. The per-frame writes rewrite
                the current values; daECS drops same-value writes when it
                compares its shadow copies, so only the bench__alive flips reach
                the mirror (pass tracks alive without filtering on it in this
                mode). Rows get copied and flagged while nothing mirrored
                changed - a filter component churning is the production shape of
                this - which is what the compare-first path in the pull is for:
                re-check the flagged row against the raw cache and build nothing.
                -filter:native is rejected here: it filters on alive itself, so
                every flip is a membership change and no flagged row keeps its
                place, which is a different workload measuring rebuilds

Per-window output is average us per frame, split into churn (the ECS writes),
ecs (tick + act, where script mirroring and the raw copies run) and frp
(updateDeferred, where pull builds), plus events/recalc/copied-rows counters
for the pull mode.

Correctness is checked once at the very end, against the ECS state itself, so
an unsubscribed pull mirror builds no script value for the whole measured run
(its raw cache follows the changes regardless). Derived
values are checked the same way, against their derivation applied to an
independent read of the components.

Built and run in place, from this directory. Measure with the release config,
the dev config skews timing:
  jam -sConfig=rel
  ecsFrpBench.exe -mode:pull -shape:map -entities:8 -changes:4

Measured 2026-08-14, windows x86_64 rel build (vc17), event-carried-copies
design, avg total us per frame, best of 3 runs x 5 windows.

single entity, 1 tracked component changing per frame:
  none 0.25 | sq 1.19 | pull 0.90 | sq unsubscribed 1.01 | pull unsubscribed 0.36

map, 4 changed entities per frame, subscribed:
  n     none   sq     sq_batched  pull
  4     0.44   2.69      2.35     1.53
  8     0.44   2.68      2.35     1.56
  16    0.45   2.73      2.38     1.67
  32    0.47   2.75      2.47     1.61
  64    0.45   2.73      2.41     1.61

map, unsubscribed (baseline 0.44):
  n=8          2.79      2.33     0.69
  n=64         2.58      2.41     0.65

map, n=64 with all 64 entities changing per frame - every row is copied at
event time and rebuilt during the pull (baseline 3.96, the churn itself
dominates):
               37.52    32.68    18.73

Unit costs implied by the above: one script ES change costs ~530 ns, one
copied row ~60 ns for two scalar components (~110 ns for the single shape's
seven), one rebuilt row ~220 ns for two scalar components.

Pull is flat in the size of the matching set because only the rows the events
carry are copied and re-read. Past MAX_DIRTY_EIDS distinct changed rows
between two pulls it reconciles the whole table against the raw cache
instead: one cheap compare per row, and only the rows whose values differ are
rebuilt - unlike the old overflow fallback, which rebuilt every row and woke
every consumer. The all-64 line above sits exactly at the cap. Unsubscribed
it pays only the raw copies - about a tenth of the script versions' cost over
the baseline.

Derived fan-out, single shape, 4 tracked components changing per frame, same
conditions (baseline 0.32):

  derived  subscribed   sq     pull
  0        0            1.11   0.45
  0        1            1.22   1.06
  6        0            2.53   0.50
  6        1            2.36   1.17
  6        6            2.58   2.30

Both columns publish the mirrored row as well, so an es that only published the
derived values would sit somewhat lower than the sq column; the delta down each
column is the part that isolates the fan-out.

A derivation costs about the same on either side, ~0.24 us per value in the es
closure and ~0.23 us per consumed value as a Computed. What differs is that the
script mode pays for all six whatever anyone is looking at, so its column is
flat, while the pull column tracks consumption. Fully consumed it is still the
cheaper one; at one consumer of six - a HUD with its block hidden, which is the
state a jetpack HUD is in almost always - it is 2x cheaper, and with nothing
consumed it is back at the price of the raw copies.

Filters, map shape, 4 changed entities per frame, same conditions as above.
"off" is plain pull, so it lines up with the pull column of the previous table:

  -filter:    off    churn   pass   native  script
  n=8        1.56    1.73    1.76    1.72    2.57
  n=64       1.69    1.80    1.86    1.66    4.68

  off     no filter, and no bench__alive write either
  churn   writes bench__alive but does not filter
  pass    a filter that accepts every row
  native  the real filter, keeps 6 of 8 (48 of 64) rows
  script  mirror everything, select in quirrel from a derived Computed

Only "off" is a different workload: a filter needs the component it reads to be
written, and that write is the whole off -> churn step.

Checking a condition costs ~10-15 ns per row (churn -> pass). Building a row
costs ~220 ns. So "native" comes out at or below "churn": rejecting a row saves
much more than testing it costs.

Flagged-but-unchanged rows (-filter:pass -samewrites:1, ~2 alive flips per
frame so ~2 rows re-read per pull): frp is 0.37 us per frame. The compare runs
against the raw cache, so a flagged row that did not change costs no table and
no ECS access, and the consumer is never woken (the run asserts triggers == 1,
so a compare that stops recognizing unchanged rows fails the mode instead of
only reading slower). When every flagged row did change (the plain map table
above), the failed compares add a few percent to the pull.

Doing the same in quirrel is worse, and worse the bigger the set gets, because
that pass walks every mirrored row every frame while the filter only sees the
rows the events carry. It also has to mirror the component being tested.
