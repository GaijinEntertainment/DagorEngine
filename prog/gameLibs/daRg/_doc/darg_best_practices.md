# daRg / Quirrel UI Best Practices

This document distills recurring bugs and anti-patterns found by analyzing
~1000 bug-fix and optimization commits across several daRg-based game UIs.
It is written as review context. A review of daRg/Quirrel UI code loads it
whole; an edit loads section 10 and the theme it touches, as AGENTS.md says.
The lint `prog/utils/darg_analyzer/darg_lint` finds many of these mistakes,
and `prog/utils/darg_analyzer/README.md` is the guide to a sweep with it.

daRg is Dagor's reactive GUI framework. UI is described by components:
plain Quirrel tables, or builder functions returning tables. Reactivity
comes from FRP observables (`Watched`, `Computed`) listed in a component's
`watch`; when one changes, the builder re-runs and the framework diffs and
rebuilds the changed subtree. All examples below use generic names.

Rules are grouped by theme. Each theme lists what to look for when
reviewing. Frequency notes ("seen repeatedly") reflect how often the
mistake appeared in real commit history.

The ten themes, so an edit can pick its section without reading the file:

1. Keep component builders pure
2. `watch` must exactly match what the builder reads
3. Reading and writing observables correctly
4. Computed discipline
5. Subscription, timer, and lifecycle hygiene
6. Performance
7. Quirrel language traps
8. Layout and behavior misuse
9. State management patterns
10. Reviewer checklist

## 1. Keep component builders pure

A builder function re-runs on every change of any observable in its
`watch`, and whenever a parent rebuilds it. Anything but "read state,
return a description" is a bug waiting to fire at unpredictable times.

### Do not create timers, subscriptions, or requests in a builder body

Seen repeatedly, with severe symptoms: leaked subscribers (each rebuild
adds one more, and all of them run on each change), per-frame rebuild
loops, timers re-armed on every rebuild, duplicate network requests.

```quirrel
// WRONG: re-armed and re-subscribed on every rebuild
function mkPager(pageState) {
  pageState.subscribe(startTimer)
  gui_scene.resetTimeout(delay, advance, timerId)
  return @() { watch = pageState, children = mkDots(pageState.get()) }
}

// RIGHT: effects live in lifecycle hooks, one pair per element instance
function mkPager(pageState) {
  return @() {
    watch = pageState
    onAttach = function() { pageState.subscribe(startTimer); startTimer() }
    onDetach = function() { pageState.unsubscribe(startTimer); gui_scene.clearTimer(advance) }
    children = mkDots(pageState.get())
  }
}
```

### Do not set observables from a builder

Setting a `Watched` that a parent (or the builder itself) watches creates
a rebuild loop that can run every frame. Move the set into a subscription
or an event handler.

```quirrel
// WRONG: parent watches previewState -> infinite rebuild loop
let panel = function() {
  previewState.set(buildPreview())     // side effect in builder
  return { watch = previewState, children = ... }
}

// RIGHT: derive it
let previewState = Computed(@() buildPreview(source.get()))
let panel = @() { watch = previewState, children = ... }
```

### Do not create Watched/Computed inside a builder

Every rebuild allocates a fresh observable and discards accumulated state
(progress, selection, animation phase). Create observables once, at module
scope or in the factory that owns the data, and pass them in.

```quirrel
// WRONG: progress resets on every rebuild
function renderItems(itemsList) {
  return itemsList.map(function(item) {
    let stage = Watched(Stage.Queue)          // recreated each rebuild
    return @() { watch = stage, ... }
  })
}

// RIGHT: created once, keyed by data
let stages = itemsList.map(@(item) Watched(Stage.Queue))
function renderItems(itemsList) {
  return itemsList.map(@(item, i) @() { watch = stages[i], ... })
}
```

The same applies to whole widgets: hoist static components and their
Computeds to module scope instead of calling `mkWidget()` per render;
factories re-invoked every render accumulate state and have crashed
long sessions.

### Do not mutate shared data from a builder

`sort`, `reverse`, `extend`, `append` mutate in place. Reordering a
module-level or observable-held array in a builder corrupts it for every
other consumer, on every render.

```quirrel
// WRONG: flips the shared array each render
foreach (i, r in rewardsList.reverse()) { ... }

// RIGHT: clone before reordering
foreach (i, r in (clone rewardsList).reverse()) { ... }
```

### Do not memoize builders that read observables

A memoized builder either never hits (fresh closure arguments defeat the
key) or, worse, hits and returns a description frozen at old observable
values. Memoize only pure functions of plain values, and key the cache on
every input that affects the result.

Call `memoize()` one time, at module scope: each call makes a new cache
that the process keeps, so a `memoize()` in a function body makes one more
cache on each call. A cache is dropped whole when it grows past its cap
(and by `clearMemoizeCaches`), so never memoize a record that holds state:
a `Watched`, a subscription, or onAttach/onDetach bookkeeping. Keys that
grow with play make the cache grow too. A memo of a function with no
parameters keeps its one value in a local, which nothing ever drops.

Review checklist for this theme: any call in a builder body that is not a
read or a pure computation; `subscribe`, `setTimeout`, `setInterval`,
`set`/`mutate`/`modify`, network/event calls; `Watched(`/`Computed(` inside
a function that returns a component description; in-place array mutators
on shared data; `memoize` around anything reading `.get()` or making
state, and a `memoize()` call that runs more than one time.

## 2. `watch` must exactly match what the builder reads

### List every observable the builder reads, transitively

A missing `watch` entry is a silent stale-UI bug: data changes, the
component does not. Seen repeatedly, often introduced by copy-paste (the
watch of the original component kept while the data source was changed)
or by adding a new `.get()` without updating `watch`.

```quirrel
// WRONG: reads two observables, watches one
let status = @() {
  watch = itemsList
  text = $"{itemsList.get().len()} of {capacity.get()}"
}

// RIGHT
let status = @() {
  watch = [itemsList, capacity]
  text = $"{itemsList.get().len()} of {capacity.get()}"
}
```

Helpers called from the builder count: if `mkRow(item)` reads
`selectedId.get()`, the caller's `watch` must include `selectedId`.

A missing entry can hide behind a parent. A parent rebuild calls a child
builder again only when it gives a new function object; the same object is
reused with no call. The UI VMs compile with closure hoisting, so an inline
builder that uses no local of its parent is one object for all parent runs.
A builder that is fresh only because each parent rebuild makes a new closure
goes stale when it is hoisted to a factory or to module scope. So give
a builder a `watch` that names all it reads before you hoist it.

### Every branch must carry the watch

A conditional builder that returns `{ watch = state }` in one branch and a
plain table in another freezes in the branch without `watch`, including
early-bail branches that render nothing.

```quirrel
// WRONG: visible branch never re-evaluates, element never hides
let hints = function() {
  if (!inVehicle.get())
    return { watch = inVehicle }
  return { children = hintRows }        // watch lost
}

// RIGHT: hoist the watch
let hints = function() {
  let watch = inVehicle
  if (!inVehicle.get())
    return { watch }
  return { watch, children = hintRows }
}
```

### Do not over-watch: keep `watch` on the smallest leaf

Watching a selection or high-frequency observable at a list container
rebuilds every row on every change. Push the watch down to the per-item
component that actually renders the difference. For high-frequency
sources (positions, per-frame values), derive a coarse, identity-stable
Computed and watch that instead:

```quirrel
// WRONG: every marker rebuilt on every position update
let markers = @() { watch = [marksList, heroPos], children = ... }

// RIGHT: rebuild only when the derived answer changes
let closestMarks = Computed(function(prev) {
  let cur = selectClosest(marksList.get(), heroPos.get())
  return isEqual(cur, prev) ? prev : cur   // identity-stable
})
let markers = @() { watch = closestMarks, children = ... }
```

Conversely, an observable consumed only inside event handlers (not to
produce the description) does not belong in `watch` at all.

### `watch` entries must be observables

A plain value, enum, or function in a `watch` array is reported as a traced
script error and skipped, so the element never updates on it. Also make sure
a prop that may be "value or observable" is detected and unwrapped with
`.get()` plus added to `watch` when it is one.

Review checklist: for each `.get()` in a builder (including helpers it
calls), is the observable in `watch`? Do all return branches share the
same `watch`? Is anything watched that only handlers read? Is a whole-list
container watching per-item state?

## 3. Reading and writing observables correctly

### Use `.get()` / `.set()`; never treat the wrapper as the value

Operating on the `Watched`/`Computed` object itself instead of its value
is one of the most frequent bug classes. The wrapper is always truthy and
never equal to a plain value, so guards silently pass or fail.

```quirrel
// WRONG                          // RIGHT
if (isReady) ...                  if (isReady.get()) ...
if (id in selectedIds) ...        if (id in selectedIds.get()) ...
if (mode == null) ...             if (!mode.get()) ...
state("newValue")                 state.set("newValue")   // calling throws
obs?.get().field                  obs.get()?.field        // guard the value
```

`.value` reads are deprecated (error log in debug builds); `.value` writes
and calling the observable throw. New code uses `.get()`, `.set()`,
`.modify()`.

### Never mutate the contents of an observable without `.mutate`

Editing the table/array returned by `.get()` in place notifies nobody:
persistence, sync, and UI all silently miss the change. Sorting it in
place additionally corrupts the shared value for other consumers.

```quirrel
// WRONG: no notification, shared state silently changed
settings.get().history <- entry
let sorted = stashItems.get().sort(byPrice)

// RIGHT
settings.mutate(@(v) v.history <- entry)
let sorted = clone stashItems.get()
sorted.sort(byPrice)
```

### `mutate` vs `modify`

`mutate(fn)` edits the container in place and ignores `fn`'s return value.
`modify(fn)` replaces the value with what `fn` returns. Using `mutate`
with `filter`/`map` throws the result away:

```quirrel
// WRONG: filter result discarded, nothing removed
items.mutate(@(v) v.filter(pred))
// RIGHT
items.modify(@(v) v.filter(pred))
```

Also guard `.mutate` on observables whose value can still be null.

### Guard `.set()` with an equality check for repeated sources

Event handlers that receive fresh tables on every event re-trigger all
watchers even when nothing changed (tables compare by reference). Compare
first:

```quirrel
eventbus_subscribe("state.update", function(v) {
  if (!isEqual(state.get(), v))
    state.set(v)
})
```

A Computed result is compared one level deep by value, so a fresh array of
the same primitives is not a change. A fresh array of fresh tables is, and
re-runs every dependent; return the previous value when content is equal
(see the identity-stable Computed pattern above).

### `Watched`/`WatchedRo` store a value, not a producer

```quirrel
// WRONG: stores the closure itself, always truthy
let supported = WatchedRo(@() queryFeature())
// RIGHT
let supported = WatchedRo(queryFeature())
```

Review checklist: any comparison, membership test, indexing, or call
applied directly to an observable; in-place edits of `.get()` results;
`mutate` whose lambda returns a value; unconditional `.set` in hot event
handlers; lambdas passed where values are expected.

## 4. Computed discipline

### A Computed must be a pure, cheap derivation

No timers, no `.set` of other observables, no network calls in the recalc
body. Self-rescheduling belongs in the update function that feeds a
ticking observable, not in the Computed:

```quirrel
// WRONG: arming a timer while recomputing
let timeLeft = Computed(function() {
  let left = max(endTime.get() - now.get(), 0)
  if (left > 0)
    gui_scene.resetTimeout(1, refresh)      // side effect
  return left
})

// RIGHT: the updater re-arms itself, the Computed stays pure
function refresh() {
  now.set(curTime())
  if (endTime.get() > curTime())
    gui_scene.resetTimeout(1, callee())
}
let timeLeft = Computed(@() max(endTime.get() - now.get(), 0))
```

### Name every source observable in the Computed body

Sources are fixed when the Computed is constructed: the observables among
the free variables and literals of its function, including lambdas nested
in it. `.get()` calls are not traced at run time, so a source that is read
only inside a named helper function is invisible. Such a Computed has no
sources and is rejected with "Computed must have at least one source
observable"; with other sources present it silently never recomputes when
the hidden one changes. Read the observable in the Computed itself:

```quirrel
function isModified(name) {
  return modifiedComps.get()?[name] == true
}

// WRONG: modifiedComps is a free variable of isModified, not of the Computed
let modified = Computed(@() isModified(compName))

// RIGHT
let modified = Computed(@() modifiedComps.get() != null && isModified(compName))
```

A read behind `&&`/`||` or an early return still registers the source, so
short-circuits are safe here.

### Time-dependent Computeds need a time dependency

Reading the clock (`get_sync_time()` etc.) creates no subscription; the
Computed goes stale the moment it is computed. Read a ticking countdown
observable inside it so it re-evaluates as time passes. Seen repeatedly
for cooldowns, countdowns and expiry gating.

### keepref Computeds that exist only to drive subscriptions

A Computed referenced only by its own `.subscribe(...)` has no owner and is
freed with its last script reference; its subscriber silently stops firing.

```quirrel
// WRONG: collected at some point, callback stops firing
Computed(@() a.get() && b.get()).subscribe(onChange)
// RIGHT
let gate = keepref(Computed(@() a.get() && b.get()))
gate.subscribe(onChange)
```

### Prefer one Computed over N raw subscriptions

Driving one output from several observables with independent per-flag
subscribers races (last writer wins) and rescans source data on every
notification. Derive the answer once and subscribe to it:

```quirrel
// WRONG: each subscriber overwrites the shared output
foreach (w in [menuA, menuB, menuC])
  w.subscribe(@(v) setSuppressed(v))

// RIGHT
let anyShown = keepref(Computed(@() menuA.get() || menuB.get() || menuC.get()))
anyShown.subscribe(setSuppressed)
```

Also: null-guard inputs inside Computeds (state is often empty at startup;
guard divisors too), and hoist a Computed that depends only on module
observables out of per-item factories so it is created once.

Review checklist: side effects in Computed bodies; sources read only
inside helper functions; clock reads with no ticker; `Computed(...).subscribe(...)`
without keepref; multiple subscribers writing one output; Computeds
allocated per row.

## 5. Subscription, timer, and lifecycle hygiene

### Pair subscribe/unsubscribe in onAttach/onDetach with the same reference

An unsubscribe with a different function object than the subscribe is a
no-op leak. Never wrap the handler in a fresh lambda on one side:

```quirrel
// WRONG: refs differ, never unsubscribed
onAttach = @() state.subscribe(@(v) handler(v))
onDetach = @() state.unsubscribe(handler)

// RIGHT
onAttach = @() state.subscribe(handler)
onDetach = @() state.unsubscribe(handler)
```

`onDetach` must mirror `onAttach` (a copy-pasted onAttach in the detach
slot doubles the leak). If a subscription's callback captures nothing
element-specific, subscribe once at module scope instead of per element.

### On attach, apply the current value before subscribing

A subscription fires only on future changes; state set before the element
attached is missed (symptom: UI correct only after the next change).

```quirrel
onAttach = function() {
  applyState(showHints.get())   // catch up
  showHints.subscribe(applyState)
}
```

### onAttach runs one time for each element, later than its build

daRg keeps an element when its own `watch` rebuilds it, also when the new
description has another `key` or `onAttach`. A parent rebuild reuses it
when the new description has an `onAttach`/`onDetach` of the same source
and matches it (the same `key`; with no key, the same description table
or the same behavior list). The script `onAttach` runs one time
for each element, and the element keeps the `onDetach` of its last
build. So a setup in `onAttach` that uses a value of the build (a mode,
a kind) keeps the value of the first build. That is right for a setup
that follows an event (a join); a setup that must follow the state runs
again from a subscription to the observable that decides it.

Script hooks (`onAttach`, `onDetach`, a recalc handler) are queued and run
in a later update, not in the build (after the first render for an element
that an update builds), and a `.set()` there notifies one frame later still.
A value that `onAttach` publishes is absent from the first build,
so each reader must watch it.

A detach clears no state flag of the element. A component that keeps
its hover state in an observable clears it in `onDetach`, or the flag stays
set when the element goes away under the pointer (its click opens a scene
that replaces it, or a mode flag above it hides it).

### Timers: unique ids, atomic rescheduling, cleared on teardown

Duplicate-timer-id throws and leaked timers were seen repeatedly. Without
an explicit id the callback closure is the id: scheduling the same closure
object twice throws "Duplicate timer id", and a fresh closure of the same
lambda logs an error and adds a second timer. Rules: give timers explicit
unique ids; use `gui_scene.resetTimeout(t, cb)` instead of a clear+set pair
(it re-arms the existing timer in place); clear in the branch that decides
not to schedule; clear in `onDetach`/teardown so a late fire cannot touch
dead state (daRg does not clear timers on detach); `clearTimer` takes the
id, or the callback when no id was given.

### Keep subscriber callbacks cheap

Subscribers run synchronously inside the FRP update on the main thread.
Heavy work there stalls every update ("slow subscriber"). Defer it:

```quirrel
state.subscribe(@(v) defer(@() heavyReinit(v)))
```

For hot subscribers, collapse `map`+`filter`+`reduce` chains over the same
list into one pass, precompute lookup tables, and early-exit searches.

### Element identity: use stable keys

Without a stable `key`, keyless siblings match by position and behavior
list, so the wrong element is reused or recreated: animations restart or
never play in sequence, fade-outs (`playFadeOut`) play on the wrong item,
input focus and scroll state jump. Key animated components by their data
identity.
Conversely, distinct concurrent windows must not share one key, and
element-local state that must survive navigation (scroll position) has to
be lifted into an observable and saved/restored in onDetach/onAttach.

Review checklist: subscribe without a reachable unsubscribe; differing
subscribe/unsubscribe references; missing initial-value application on
attach; `setTimeout` without id where re-entry is possible; heavy work in
subscribers; animated components without `key`.

## 6. Performance

### Mark constant literals `const`; hoist static descriptions

Every non-const literal array/table in a builder is a fresh allocation per
rebuild, and a "new" value for the differ. Applies to `size`, `padding`,
`margin`, `borderWidth`, `animations`, `transform`, color tables.

```quirrel
// WRONG                              // RIGHT
size = [hdpx(200), hdpx(40)]          size = const [hdpx(200), hdpx(40)]  // if constant-foldable
padding = [0, 5, 0, 5]                padding = const [0, 5, 0, 5]
transform = {}                        transform = true   // cheapest way to enable transforms
```

`const` is folded at compile time (pure calls with constant arguments fold
too) and freezes a container result.
Hoist components that do not depend on builder inputs to module scope.

Three values must stay new on each build:
- a `hotkeys` array whose handler reads a value of the build (a builder
  local, an entry picked by state), or that has an item with no action (it
  takes the element's `onClick`): `Element::setup` reads `hotkeys` again
  only when the array is not the same immutable object, so a `const` array,
  or a frozen one made one time, keeps the handlers of the first build;
- a description with `RtPropUpdate`: it writes into its own description;
- a table as `key` (`key = {}`): a new key on purpose, so that the element
  is made again (an animation starts again) on each build.

Hoist to the narrowest scope that is made one time for each element:
the factory that makes the builder. Use module scope only for a value
with no per-element state. A component that keeps state (an `XmbNode`,
a hover flag) must not become one object that all instances share;
a hoisted value that the builder changes (`__update`) must be a copy.

### One value where an array is not needed

`watch`, `behavior` and `children` take one value as well as an array.
`size = X` is `[X, X]`; `padding`, `margin`, `borderWidth`, `borderRadius`,
`texOffs`, `screenOffs` and `safeAreaMargin` take 1 to 4 elements, and equal
elements are the scalar; `pivot`, `translate` and `scale` of a `transform`
take one number for both axes. A null is the exception: `size = null` is
`SIZE_TO_CONTENT`, `[null, null]` is 0 pixels, and an offset array asserts
on a null element. The array is one more allocation on each build.
Keep `children = [x]` where x can be null in a table with no `rendObj`
and no `size`: a non-null `children` slot counts in the gaps of its flow.
Keep the array also where another module extends the field
(`[...].extend(rootBase?.behavior ?? [])` throws on a single value).

### Do not hold `Picture` objects at module scope

A module-level `Picture(...)` pins its texture in memory for the process
lifetime, even when the screen is never shown. Build Pictures inside
factories/builders so textures load and release with the UI. Do not bake
option-dependent sizes or images at module scope either; derive them from
the option observable so changes apply live. Defer locale-dependent
lookups (`loc`, language id) from module root into builders.

### Cache expensive derivations; scan once

Repeated decode/parse/BLK-load reachable from reactive recomputation must
go through a cache (bounded, keyed on every input that affects the
result). Replace per-item full-collection scans with a keyed map built in
one pass. For "is there any X" flags, short-circuit on the first hit
instead of counting everything.

### Use the right reactivity tier

- Per-frame, high-cardinality updates (hundreds of items): toggle
  properties from an `RtPropUpdate` update() rather than allocating a
  Watched + subscription per item. The update function must be a
  statement-body function returning null or a property table, not an
  expression lambda (which returns the assigned scalar).
- Hover-only values: compute in `onElemState` on demand; a standing
  Computed for a tooltip string is waste.
- Values that change every frame but rarely matter: debounce with a
  threshold/hysteresis Computed before they reach `watch`.

Review checklist: non-const literal arrays in hot components; module-scope
Pictures; repeated `.get()` of the same observable in loops; chained
array passes in subscribers; standing Computeds for rarely-needed strings.

## 7. Quirrel language traps

These are pure language semantics that repeatedly caused UI bugs.

### Truthiness: `0` is falsy; empty string/array/table are truthy

`if (count)` drops a legitimate zero; use `!= null` for presence checks on
numbers. Inversely, `if (arr)` is always true; test `arr.len()`. Do not
use `""` as a sentinel for "no callback": `?.` treats it as present; use
`null`.

### `&&`/`||` return an operand, not null

`a && b` is `a` when `a` is falsy (`false`, `null`, `0`, `0.0`), else `b`;
`a || b` is `a` when truthy, else `b`. A guard leaks through as the result:

```quirrel
// WRONG: id is `false` when isValid is false, then id + 1 throws
let id = isValid && ids?[owner]
// RIGHT
let id = isValid ? ids?[owner] : null
```

### Parenthesize ternaries after `&&`/`||` and in comparators

`&&` binds tighter than `?:`, so trailing ternaries swallow the guards.
In sort comparators, `<=>` chained with `||` collapses keys to bools.

```quirrel
// WRONG: guards became part of the condition
return available && onBase && isWeapon ? weaponLink : plainLink
// RIGHT
return available && onBase && (isWeapon ? weaponLink : plainLink)

// WRONG: buyable key lost
return (b.buy?1:0) <=> (a.buy?1:0) || isAsc ? a.p<=>b.p : b.p<=>a.p
// RIGHT
return (b.buy?1:0) <=> (a.buy?1:0) || (isAsc ? a.p<=>b.p : b.p<=>a.p)
```

Prefer explicit `if (r != 0) return r` chains for multi-key comparators.

### `in` on arrays tests indices, not values

Use `findindex`/`findvalue` for arrays, or key a table. Conversely, to
test a table key use `in`; `findindex`/`findvalue` on a table scan values.

### Integer division truncates

`100 * cur / max` is 0 for small values; write `100.0 * cur / max`.
Similarly round, do not truncate, before formatting floats as integers.

### `__merge` returns a new table; `__update` mutates in place

Applying `__update` to a shared/base/default table contaminates every
other user of that table. Seen repeatedly, including the special case of
`__update(params)` clobbering an already-computed `watch` slot:

```quirrel
// WRONG: mutates the shared base style
let mkLabel = @(text, ovr = {}) baseStyle.__update(ovr)
// RIGHT
let mkLabel = @(text, ovr = {}) baseStyle.__merge(ovr, { text })

// WRONG: params.watch overwrites the computed watch list
return { watch = watchList, ... }.__update(params)
// RIGHT: strip keys you already computed
return { watch = watchList, ... }.__update(params.filter(@(_, k) k != "watch"))
```

Never `__update` a defaulted `{}` parameter or any caller-owned table;
callers of the same call site share state. Use `null` defaults and guard,
or merge into a fresh table. Clone tables received from native/immutable
sources before adding slots.

### Table iteration order is unspecified

When display order matters, iterate an explicitly sorted key array:

```quirrel
foreach (key in tbl.keys().sort())
  render(tbl[key])
```

Also do not key a collection by a non-unique field (entries overwrite);
use an array when duplicates are valid.

### Null-safety patterns

- Place `?.` on the value: `obs.get()?.field`, not `obs?.get().field`.
- `?[0]` for possibly-empty arrays; `?? default` after `parse_json`.
- Destructure optional config with defaults:
  `let { imgPath = null, color = defColor } = cfg.get() ?? {}`.
- `?.`/`?[` make the rest of the chain null-safe: `cfg?.block.field` is
  null when `cfg`, `block`, or `field` is missing. Links before the first
  `?.` are unguarded: `cfg.block?.field` throws when `block` is absent, and
  `(cfg ?? {}).block.field` throws on the missing `block`.
- `?.` suppresses the missing-slot error; it does not test for null, so a
  `""` or `{}` "no value" sentinel passes through it.
- Key types must match: integer and string keys never collide
  (`tbl[1]` vs `tbl["1"]`), a silent never-matches bug.

### `[pure]` is trusted, and the compiler acts on it

The mark is not checked. Outside `const`, a call of a `[pure]` function runs
on each call, as any call does. Under `const` (`const X = f(...)`, or an
inline `const f(...)`) the compiler runs the call one time, when it
compiles the module, and puts the result in the code; a container result
is immutable. So a `[pure]` function that reads state (an observable, or a
module table that other code changes, also through a callback) gives each
`const` that calls it the compile-time value for good. Mark only functions
that read their arguments and constants, or state whose change runs
the scripts again. `[nodiscard]` is checked on natives only. A container
that a module exports is immutable for the modules that import it.

### Misc

- `@(x) function() {...}` returns a closure instead of executing; use a
  direct function body for handlers that must run statements.
- `each`/`mutate` callbacks are for side effects; their return values are
  ignored -- use `map`/`modify` when you need the result.
- Import named exports (`from "mod" import fn`); binding a whole module
  table and calling it throws.
- Use `require_optional` for modules that may be absent in restricted VMs.
- Bitmask checks: `(value & mask) == mask`, never `value == mask`.
- Prefer `$rawdelete` over the deprecated `delete` operator.

## 8. Layout and behavior misuse

### Text needs measurable bounds

- `ROBJ_TEXTAREA` must have a bounded or flex width to wrap; without one
  it overflows or collapses. Multiline text needs an explicit size mode
  (e.g. `size = [flex(), SIZE_TO_CONTENT]`).
- `flex()` on the cross axis of a `SIZE_TO_CONTENT` parent collapses to
  zero: the child takes 100% of a parent that has no size yet, and gives
  no content size back. Size to content unless a fill is intended.
- Cap text inputs with `maxChars`.

### Size-mode pitfalls

- Do not mix a `SIZE_TO_CONTENT` `minWidth` with `flex()` when you want
  equal columns; the content minimum is a floor, and the other columns
  split what is left.
- To keep an element square on a non-square parent, drive both axes from
  the same reference (same percent axis or measured shorter side).
- Clamp dpi-scaled sizes to at least 1px: `hdpxi`/rounding can yield 0 at
  low resolutions and the element vanishes.
- A container of absolutely-placed children must itself be large enough
  to contain them, or hover/hit-testing fails outside its content bbox.
- Give a container an explicit `flow` when children must stack; without
  it they overlap at the same origin.
- Derive sizes from the observable they depend on (safe area, UI scale)
  and put that observable in `watch`; a computed-once size goes stale.

### Wrong keys and values fail silently

daRg reads only the keys it knows. A misspelled key (`fontsize`, `valing`,
`line_width`) is skipped with no message, and the element draws
with the default. `gap` takes a number (pixels) or a component put between
the children; an array or a `flex()`/`pw()`/`ph()` value gives no gap
and no message.

### Transforms and animations

- Transform animations require the element to declare `transform`
  (`transform = true` is the cheap form).
- Animated elements need a stable `key`; use `globalTimer = true` only
  when loop phases must stay synchronized across elements.
- Prefer animating `opacity`/`scale`/`translate` over layout-affecting
  properties.

### Input

- For pointer input that must work with mouse and gamepad cursor, use the
  device-agnostic `ProcessPointingInput`/`onPointerMove`, not
  `TrackMouse`/`onMouseMove`.
- Do not put a Button/onClick on a full-bleed container unless
  click-anywhere is intended; it swallows every click.
- Gate input-consuming helper elements (modifier monitors, shields) on
  the context that needs them, or they steal input from siblings.
- A modal must handle Esc/back itself and suppress the underlying
  screen's handler; shared back handlers should unwind one level.

## 9. State management patterns

### One source of truth, derived views

The largest category of bugs: two copies of the same state drifting apart.

- Do not mirror an observable into a second observable by subscription;
  derive with `Computed`.
- When a value has an override (preset, forced mode), expose one derived
  "effective value" observable and make every consumer read it, instead
  of re-implementing the branch at each call site.
- Do not share one `Watched` between two writers with different value
  shapes (whole-object `set` vs field `mutate`): last writer wins and
  data is lost. Separately-sourced state gets separate observables.
- Module-level `local` variables that other modules import do not
  propagate reassignment; shared mutable state that changes belongs in a
  `Watched` (stable reference).

### Reset state when its scope ends

Session/battle/screen-scoped state must be explicitly reset on the event
that ends its scope (logout, battle end, window close), or it leaks into
the next session. Initialize per-battle HUD state in `onAttach`, not at
module load. When one mode switches to another, reset all mode-specific
flags, not just the primary one. Centralize the reset in one function
called from every entry point.

### Snapshot vs live reads

- A value read once at build time is frozen; if the UI must follow it,
  read it in the builder and watch it (see theme 2), or pass the
  observable itself, not `obs.get()`, into component factories.
- Closures kept across rebuilds (animation callbacks, handlers with
  stable keys) must read current state from observables, not from stale
  captured locals.
- When comparing against a previous value, update the snapshot only
  after the comparison:

```quirrel
// WRONG: prev updated first, diff always empty
prev = cur
if (cur.len() != prev.len()) fire()
// RIGHT
if (cur.len() != prev.len()) fire()
prev = cur
```

### Guard async and repeated actions

- Wrap request-sending actions in an in-progress flag (set before, clear
  on completion in all paths) so double-clicks cannot double-send.
- Async callbacks may fire after logout/screen close: re-check the state
  they touch before acting.
- Do not rely on a false-to-true edge of a Watched for one-shot events;
  same-frame set/reset coalesces under deferred updates and the edge is
  lost. Drive one-shot work from an actual event or explicit flag.
- Reset one-shot "pending action" observables after consuming them.

### Sentinels and identity

- Use distinct, correctly-typed sentinels: reset an observable to a value
  of its declared type, use `null` (not `false`/`""`/`{}`) for absence,
  and match the sentinel the producer actually returns (a function
  returning 0-for-missing needs a falsy check, not `== null`).
- Address list items by stable id, not array position, whenever the list
  can be reordered or filtered.
- Identity predicates must compare every distinguishing field (variant,
  template, kind), not just a shared name.

## 10. Reviewer checklist

Builder purity
- [ ] No subscribe/timers/requests/`.set` in builder bodies; effects in
      onAttach/onDetach or subscriptions.
- [ ] No `Watched`/`Computed` created inside builders; no per-render
      factory invocations of static widgets.
- [ ] No in-place mutation (`sort`/`reverse`/`__update`) of shared or
      observable-held data in builders.
- [ ] `memoize()` runs one time (at module scope, or in a function called
      once), never around a record that holds state.

watch correctness
- [ ] Every `.get()` the builder (or its helpers) reads is in `watch`.
- [ ] All return branches carry the same `watch`.
- [ ] No container-level watch for per-item state; high-frequency sources
      wrapped in identity-stable Computeds.
- [ ] A builder to hoist watches all it reads: a parent rebuild does not
      call the same builder object again.

FRP API use
- [ ] No comparison/membership/indexing/calls on the observable wrapper;
      `.get()?.field`, not `?.get()`.
- [ ] `mutate` for in-place edits, `modify` for replacement; copies before
      sorting `.get()` results.
- [ ] Equality guard before `.set` of tables from repeated events.
- [ ] Computeds pure and cheap; every source read in the body, not only
      in a helper; time-dependent Computeds driven by a ticker; keepref
      on subscribe-only Computeds.

Lifecycle
- [ ] subscribe/unsubscribe with the same reference, paired in
      onAttach/onDetach; current value applied on attach.
- [ ] Timers: unique ids, `resetTimeout` for rescheduling, cleared on
      detach/teardown.
- [ ] Stable `key` on animated/stateful elements; per-scope state reset on
      scope end.
- [ ] An `onAttach` setup that uses a value of the build and must follow
      the state runs again when that state changes; a value published
      in `onAttach` is watched; hover flags kept in observables are
      cleared in `onDetach`.

Language
- [ ] No `if (x)` presence checks on numeric fields; no `&&`-as-value.
- [ ] Ternaries after `&&`/`||` and in comparators parenthesized.
- [ ] No `in` on arrays for membership; float math for ratios; `__merge`
      over `__update` on shared tables; sorted keys where order matters;
      `?.`/defaults on any data that can be absent.
- [ ] `[pure]` only on functions that read no state, or state whose change
      runs the scripts again.

Layout/perf
- [ ] `const` on constant literal arrays; static parts hoisted; no
      module-scope Pictures.
- [ ] No `const` or frozen `hotkeys` array whose handlers read the build,
      no frozen `RtPropUpdate` description, no hoisted `key = {}`.
- [ ] One value, not a one-element or equal-sides array (but keep
      `children = [x]` for an x that can be null, see theme 6).
- [ ] No misspelled description keys; `gap` is a number or a component.
- [ ] Text has measurable bounds; sizes derived from watched observables;
      dpi-scaled sizes clamped to >= 1.
- [ ] Pointer handling device-agnostic where gamepad is supported.

## Appendix: evidence

Rules above are generalized from analyzed fix commits in several
production daRg codebases (about 1070 fix/optimization commits reviewed).
Representative commits per theme (short hashes in this repository):

- Builder purity: 2ed45a93b97b, a1f61caa2acb, 3b2ecb34165b, cc8808ec0318,
  eb7a35f9c1bf, fa1694093fc1, 088a55fb51c4, 8262cc7f9b4e
- watch correctness: 78519951c22e, c20d4d3b0220, 1b3ff9a1cd16,
  b606454a17ac, 96856ace3979
- FRP API: a9294ceef26b, ad8df48d386a, 6552dd0b2263, 5ab0af44183b,
  cc4d4e24f58f, 3c76729ca9b6
- Computed discipline: 28b68a13d034, 7d8e990b7cc6, 966505b7d314,
  0effafe0888b, d5fd54ea0724
- Subscriptions/timers: ff8acbfad317, 95404c114c31, ef0e50ae39c0,
  ae24364cc504, f3dea5a9bbff
- Performance: 150e0ac5bb60, 0ced9f574b7c, 53f1dbfb2672, 1a29b33d761e,
  0ee627cf16bd, 3c76729ca9b6
- Quirrel language: ee9f98124f17, 0f4a6bad9baa, c54a9f230ae9,
  40a4e2ce90fb, 1362a0d91c69, c4bc0cbbac07, e27cca7cd494, 253d2e3ca284,
  93af78a3d7e0
- Layout/behavior: 535a701c4d47, 10b03112f1e3
- State management: 0b7819bd4001, 42fcde807020
