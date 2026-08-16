// Three ways of getting ECS state into frp observables, over the same load:
//   "sq"         - script entity system writing into a Watched (raw, per change)
//   "sq_batched" - same, but per-eid Watched storage + membership set flushed
//                  once per frame (what std/frp.nut mkTriggerableLatestWatched
//                  SetAndStorage does, and what ec_to_watched.nut re-exports)
//   "pull"       - ecs.computed: the ES only marks dirty, the value is built
//                  during the frp update
//   "none"       - no mirroring
//
// shape "map" is an {eid: row} table over bench_bot, shape "single" is the
// component set of the one bench_hero entity.
//
// -derived:N adds the second half of a HUD: N observables built on top of the
// mirrored row. In "pull" they are Computeds over the mirror, recalculated by
// the graph and only while something consumes them; in "sq" the entity system
// derives them inline and pushes each into its own Watched, which is what a
// hand-written mirroring es does.

import "ecs" as ecs
import "bench" as bench
from "frp" import Watched, Computed
from "ecs.computed" import mkEcsComputed, mkEcsComputedEidMap

let mode = bench.mode
let shape = bench.shape
let subscribed = bench.subscribed
// with -derived:N this is how many of the derived values have a consumer, so a
// partly consumed fan-out can be measured: a HUD whose block is hidden keeps a
// subscriber on the value that decides visibility and none on the rest
let subscribedCount = bench.subscribedCount
// pull mode only, see readme.txt: "churn" writes the extra component but does
// not filter, "pass" filters with an expression that accepts every row,
// "native" filters in the query, "script" mirrors everything and selects in
// quirrel instead. All four write the same components, so they are comparable.
let filterMode = bench.filterMode
// How many observables the consumer derives from the row. The jetpack HUD that
// motivated ecs.computed has six; 0 leaves the mirror itself as the only value.
let derivedCount = bench.derivedCount
// the per-frame writes rewrite the current values, see readme.txt
let sameWrites = bench?.sameWrites ?? false

let counters = { triggers = 0 }
local state = null       // the observable under test
local storage = null     // sq_batched: per-eid observables
local onFrame = @() null // sq_batched: flush point, called every frame
local loadout = null     // pull/single: object component, checks deep-copy conversion
local rawMirror = null   // filter=script: the unfiltered mirror behind state
let derived = []         // -derived:N: what a HUD builds on top of the row

let mapCompsRo = [["bench__value", ecs.TYPE_INT], ["bench__hp", ecs.TYPE_FLOAT]]
let mapTrack = "bench__value,bench__hp"
let heroCompsRo = [
  ["hero__fuel", ecs.TYPE_FLOAT],
  ["hero__maxFuel", ecs.TYPE_FLOAT],
  ["hero__enabled", ecs.TYPE_BOOL],
  ["hero__alert", ecs.TYPE_BOOL],
  ["hero__flightMode", ecs.TYPE_BOOL],
  ["hero__lock", ecs.TYPE_BOOL],
  ["hero__altitude", ecs.TYPE_FLOAT],
]
let heroTrack = "hero__fuel,hero__alert,hero__flightMode,hero__altitude"

// The jetpack HUD derivations, in its order, cycled when more are asked for.
// Both mirroring modes run this same list over the same row shape, so the only
// difference measured is who builds the row and when the derivations run.
let derivations = [
  @(row) row?.hero__enabled ?? false,
  @(row) row?.hero__lock ?? false,
  @(row) (row?.hero__alert ?? false) && !(row?.hero__lock ?? false),
  @(row) (row?.hero__enabled ?? false) && (row?.hero__flightMode ?? false),
  @(row) row == null ? null : (row.hero__maxFuel > 0.0 ? (row.hero__fuel / row.hero__maxFuel) * 100.0 : -1.0),
  @(row) row == null ? 0.0 : (row.hero__enabled ? row.hero__altitude : -1.0),
]
let derivationOf = @(i) derivations[i % derivations.len()]

let mkMapRow = @(comp) { bench__value = comp["bench__value"], bench__hp = comp["bench__hp"] }
let mkHeroRow = @(comp) {
  hero__fuel = comp["hero__fuel"]
  hero__maxFuel = comp["hero__maxFuel"]
  hero__enabled = comp["hero__enabled"]
  hero__alert = comp["hero__alert"]
  hero__flightMode = comp["hero__flightMode"]
  hero__lock = comp["hero__lock"]
  hero__altitude = comp["hero__altitude"]
}

function registerEs(name, upsert, remove, compsRo, rqTag, track) {
  ecs.register_entity_system(name, {
      EventEntityCreated = upsert
      EventComponentsAppear = upsert
      EventComponentChanged = upsert
      EventEntityDestroyed = remove
      EventComponentsDisappear = remove
    },
    { comps_ro = compsRo, comps_rq = [rqTag] },
    { track = track })
}

if (mode == "sq" || (mode == "sq_batched" && shape == "single")) {
  // per-eid storage buys nothing for a singleton, so the batched mode reuses
  // the raw one there and only differs on the map shape
  if (shape == "map") {
    let bots = Watched({})
    state = bots
    registerEs("bench_sq_map_es",
      @(_evt, eid, comp) bots.mutate(@(t) t[eid] <- mkMapRow(comp)),
      function(_evt, eid, _comp) {
        if (eid in bots.get())
          bots.mutate(@(t) t.$rawdelete(eid))
      },
      mapCompsRo, "bench_bot", mapTrack)
  }
  else {
    let hero = Watched(null)
    state = hero
    for (local i = 0; i < derivedCount; i++)
      derived.append(Watched(derivationOf(i)(null)))
    // one closure builds the row and every value derived from it, then pushes
    // each into its own Watched - the shape a mirroring es is written in
    let publish = function(row) {
      hero.set(row)
      foreach (i, w in derived)
        w.set(derivationOf(i)(row))
    }
    registerEs("bench_sq_single_es",
      @(_evt, _eid, comp) publish(mkHeroRow(comp)),
      @(_evt, _eid, _comp) publish(null),
      heroCompsRo, "bench_hero", heroTrack)
  }
}
else if (mode == "sq_batched") { // map shape only, see above
  let eidsSet = Watched({})
  let rows = {}
  let pending = {}
  state = eidsSet
  storage = rows
  let flush = function() {
    if (pending.len() == 0)
      return
    eidsSet.mutate(function(v) {
      foreach (eid, _ in pending)
        v[eid] <- eid
    })
    pending.clear()
  }
  onFrame = flush
  eidsSet.whiteListMutatorClosure(flush)
  registerEs("bench_sq_batched_map_es",
    function(_evt, eid, comp) {
      let row = mkMapRow(comp)
      if (eid in rows)
        rows[eid].set(row)
      else {
        rows[eid] <- Watched(row)
        pending[eid] <- true
      }
    },
    function(_evt, eid, _comp) {
      if (eid in rows) {
        rows.$rawdelete(eid)
        pending[eid] <- true
      }
    },
    mapCompsRo, "bench_bot", mapTrack)
}
else if (mode == "pull") {
  let mapParams = { comps = mapCompsRo, comps_rq = ["bench_bot"] }
  let heroParams = { comps = heroCompsRo, comps_rq = ["bench_hero"] }
  if (filterMode == "pass") {
    // accepts every row, so this measures the expression itself against "churn"
    mapParams.filter <- "ge(bench__value, 0)"
    heroParams.filter <- "ge(hero__fuel, 0.0)"
    // tracked but not part of the verdict: its flips flag rows whose mirrored
    // values did not change, which is what -samewrites measures
    if (sameWrites)
      mapParams.comps_filter <- [["bench__alive", ecs.TYPE_BOOL]]
  }
  else if (filterMode == "native") {
    mapParams.comps_filter <- [["bench__alive", ecs.TYPE_BOOL]]
    mapParams.filter <- "bench__alive"
    heroParams.filter <- "hero__alert" // already in comps, so no comps_filter entry
  }
  else if (filterMode == "script") {
    // selecting in quirrel means mirroring the component the choice reads, and
    // building a second table from the first one
    let scriptComps = clone mapCompsRo
    scriptComps.append(["bench__alive", ecs.TYPE_BOOL])
    mapParams.comps = scriptComps
  }

  if (filterMode != "script")
    state = shape == "map" ? mkEcsComputedEidMap(mapParams) : mkEcsComputed(heroParams)
  else if (shape == "map") {
    rawMirror = mkEcsComputedEidMap(mapParams)
    let raw = rawMirror
    state = Computed(function() {
      let res = {}
      foreach (eid, row in raw.get())
        if (row.bench__alive)
          res[eid] <- row
      return res
    })
  }
  else {
    rawMirror = mkEcsComputed(heroParams)
    let raw = rawMirror
    state = Computed(@() (raw.get()?.hero__alert ?? false) ? raw.get() : null)
  }

  if (shape != "map")
    loadout = mkEcsComputed({ comps = [["hero__loadout", ecs.TYPE_OBJECT]], comps_rq = ["bench_hero"] })

  // holds the mirror itself, not the module variable: dropMirrors clears both
  let src = state
  for (local i = 0; i < derivedCount; i++) {
    let fn = derivationOf(i)
    derived.append(Computed(@() fn(src.get())))
  }
}

// a HUD watches the values it derived, not the row they were built from
if (derived.len() > 0) {
  foreach (i, obs in derived)
    if (i < subscribedCount)
      obs.subscribe(@(_v) counters.triggers++)
}
else if (subscribed && state != null)
  state.subscribe(@(_v) counters.triggers++)

// Read only when the harness asks: for an unsubscribed pull mirror every read
// activates a recalc, so the harness reads once at the very end.
function readCount() {
  if (state == null)
    return -1
  if (shape != "map")
    return state.get() == null ? 0 : 1
  return storage != null ? storage.len() : state.get().len()
}

function readChecksum() {
  if (state == null)
    return -1
  if (shape != "map")
    return (state.get()?.hero__fuel ?? -1.0).tointeger()
  local sum = 0
  if (storage != null) {
    foreach (_eid, w in storage)
      sum += w.get().bench__value
  }
  else {
    foreach (_eid, row in state.get())
      sum += row.bench__value
  }
  return sum
}

// an object component must arrive as a deep-copied plain table, not a wrapper
// borrowing ECS memory
function readLoadoutAmmo() {
  if (loadout == null)
    return -1
  let v = loadout.get()
  if (typeof v != "table")
    return -2
  return v?.ammo ?? -3
}

// Which filter modes actually select rows, as opposed to only writing the extra
// component. Matches what the harness expects, see readme.txt.
let filterSelects = filterMode == "native" || filterMode == "script"

// The aggregate checksum can alias a wrong field, a wrong type or a stale row.
// These compare every mirrored field against an independent read of the same
// component instead.
function verifyRow(eid, row, comps, errors, what) {
  if (typeof row != "table") {
    errors.append($"{what} eid {eid}: row is {typeof row}, want a plain table")
    return
  }
  foreach (c in comps) {
    let name = c[0]
    let want = ecs.obsolete_dbg_get_comp_val(eid, name)
    if (!(name in row))
      errors.append($"{what} eid {eid}: {name} is missing")
    else if (row[name] != want)
      errors.append($"{what} eid {eid}: {name} is {row[name]}, want {want}")
  }
}

function verifySingle(eid, errors) {
  let row = state.get()
  if (filterSelects && ecs.obsolete_dbg_get_comp_val(eid, "hero__alert") != true) {
    if (row != null)
      errors.append("single: filtered out, but the mirror is not null")
    return
  }
  if (row == null) {
    errors.append("single: the entity matches, but the mirror is null")
    return
  }
  verifyRow(eid, row, heroCompsRo, errors, "single")
}

// Same check one level down: every derived value must equal its derivation
// applied to an independent read of the components it comes from.
function verifyDerived(eid, errors) {
  local want = null
  if (!filterSelects || ecs.obsolete_dbg_get_comp_val(eid, "hero__alert") == true) {
    want = {}
    foreach (c in heroCompsRo)
      want[c[0]] <- ecs.obsolete_dbg_get_comp_val(eid, c[0])
  }
  foreach (i, obs in derived) {
    let expected = derivationOf(i)(want)
    if (obs.get() != expected)
      errors.append($"derived #{i}: {obs.get()}, want {expected}")
  }
}

function verifyMap(eids, errors) {
  let expected = {}
  foreach (eid in eids)
    if (!filterSelects || ecs.obsolete_dbg_get_comp_val(eid, "bench__alive") == true)
      expected[eid] <- true

  // sq_batched holds the rows in storage; its published eid set only ever grows
  let rows = {}
  if (storage != null) {
    foreach (eid, w in storage)
      rows[eid] <- w.get()
  }
  else {
    foreach (eid, row in state.get())
      rows[eid] <- row
  }

  if (rows.len() != expected.len())
    errors.append($"map: {rows.len()} rows, want {expected.len()}")
  foreach (eid, row in rows) {
    if (!(eid in expected))
      errors.append($"map: eid {eid} is in the mirror but should not be")
    else
      verifyRow(eid, row, mapCompsRo, errors, "map")
  }
  foreach (eid, _ in expected)
    if (!(eid in rows))
      errors.append($"map: eid {eid} is missing from the mirror")
}

// eids is [hero] on the single shape and every bot on the map shape.
// Returns null when everything matches, otherwise a joined error list.
function verifyFields(eids) {
  if (state == null)
    return null
  let errors = []
  if (shape != "map") {
    verifySingle(eids[0], errors)
    verifyDerived(eids[0], errors)
  }
  else
    verifyMap(eids, errors)
  return errors.len() > 0 ? "; ".join(errors) : null
}

// A mirror registers an entity system, so it may only be created during es
// loading - otherwise every call would cost a whole-game ES reorder. Called
// after end_es_loading, so it must fail.
function readCreateOutsideLoadingRefused() {
  try {
    mkEcsComputed({ comps = [["bench__value", ecs.TYPE_INT]], comps_rq = ["bench_bot"] })
    return 0
  }
  catch (e) {
    return 1
  }
}

// releases the mirror handles so the harness can check that the sweep then
// unregisters their entity systems. The derived observables have to go too: a
// Computed over the mirror holds it alive, and a held mirror is never swept.
function dropMirrors() {
  state = null
  storage = null
  loadout = null
  rawMirror = null
  derived.clear()
}

return {
  onFrame
  readCount
  readChecksum
  readLoadoutAmmo
  verifyFields
  readCreateOutsideLoadingRefused
  readTriggers = @() counters.triggers
  dropMirrors
}

