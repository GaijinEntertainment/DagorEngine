# daRg / Quirrel Context Primer (for commit analysis)

This primer gives you the minimum daRg/Quirrel knowledge needed to read UI
commit diffs correctly and classify what was wrong and what was fixed.

## What daRg is

daRg (Dagor Reactive GUI) is a reactive UI framework in the Dagor engine,
inspired by React/Flutter. UI is scripted in Quirrel script language.
A UI scene is a tree of elements built from "components": plain Quirrel
tables describing an element, or functions (closures) returning such tables.

```quirrel
// static component: a table
let label = { rendObj = ROBJ_TEXT, text = "Hi" }

// reactive component: a builder function returning a table
let counter = Watched(0)
let view = @() {
  watch = counter              // rebuild when counter changes
  rendObj = ROBJ_TEXT
  text = $"Value = {counter.get()}"
}
```

## Component builder semantics (critical)

- A builder function (closure component) is called EVERY time any observable
  listed in its `watch` property changes, and on initial build. The element
  is set up again from the new description. Its children are matched to the
  old children: same `key`, or the same description table, or (both keyless)
  the same behavior list. A child closure that is the same object as before
  is reused without a call. Unmatched children are recreated.
- Therefore a builder must be a PURE function of state: it must only read
  state and return a description. Side effects in the builder body (setting
  Watched values, sending events/requests, playing sounds, subscribing,
  logging business actions) run on every rebuild, at unpredictable times,
  and are a classic bug source. Setting a Watched inside a builder is not
  detected by frp: the set is deferred, invalidates the watching element
  next frame, and the builder runs again, so the loop repeats every frame.
- One-shot effects belong in lifecycle hooks: `onAttach` (element inserted
  into the tree), `onDetach` (removed). These are script-level properties of
  the component table.
- A `function ... { return { ... } }` or `@() {...}` component WITHOUT a
  `watch` field is still re-invoked when a parent rebuild passes a new
  closure object (the usual case for a lambda created in the parent
  builder); effects there are equally wrong.
- `watch` must list ALL observables whose `.get()` the builder reads.
  A missing entry means the UI silently does not update ("stale UI");
  an extra/too-broad entry means over-invalidation (perf).

## FRP primitives (module "frp")

- `Watched(v)` - mutable observable.
  - `.get()` read. `.value` read still works but logs a deprecation error
    in debug builds; `.value` write and calling the observable (`state(v)`)
    throw. Commits migrating `.value` -> `.get()/.set()` are API migrations.
  - `.set(v)` assign; the stored value changes at once, but propagation to
    subscribers/computeds is deferred and batched (`update_deferred`), so
    multiple sets in one frame coalesce.
  - `.modify(fn)` set to `fn(currentValue)`.
  - `.mutate(fn)` mutate contained table/array in place, then trigger.
    Mutating `w.get().field = x` WITHOUT `mutate` does NOT notify anyone:
    a very common bug (UI not updating).
  - `.trigger()` manually notify subscribers without changing value.
  - `.subscribe(fn)` / `.unsubscribe(fn)` change callbacks. A subscribe
    without matching unsubscribe (e.g. subscribing in a builder, or in
    module scope of a repeatedly executed file) leaks and fires callbacks
    for dead UI. `subscribe_with_nasty_disregard_of_frp_update` runs in the
    same deferred pass as a plain subscriber; its only difference is that it
    may `.set` other observables without the subscriber-validation error
    (the set lands in the next update). It is regularly replaced by plain
    subscribe.
- `Computed(fn)` - derived read-only observable. `fn` must be a pure,
  cheap function of other observables; the sources are the observables
  among the free variables and literals of `fn` (nested lambdas included),
  fixed at construction. Bugs seen in practice:
  - doing heavy work or allocation in a Computed that runs often;
  - reading an observable only inside a named helper function, so it is
    not a source: "Computed must have at least one source observable", or
    a Computed that never recomputes on that observable;
  - setting other Watched values inside a Computed (side effect: frp logs
    "triggered during recalc of" and defers the set to the next update);
  - Computed results are compared one level deep by value, so a fresh table
    with the same top-level entries is not a change; a fresh table that
    holds fresh nested tables/arrays is, and re-runs every dependent.
- Value identity: `Watched.set` skips propagation when the new value equals
  the old by reference/primitive equality; a new equal-content table counts
  as a change. `mutate` always propagates.
- Computed values are always frozen. `make_all_observables_immutable(true)`
  freezes Watched values too, but nothing in this repo enables it; treat
  stored values as immutable anyway and use `mutate`.

## Element description properties you will see in diffs

- `rendObj` - render object type: ROBJ_TEXT, ROBJ_TEXTAREA (multiline,
  needs `behavior = Behaviors.TextArea`), ROBJ_IMAGE, ROBJ_SOLID, ROBJ_BOX,
  ROBJ_FRAME, ROBJ_9RECT, ROBJ_PROGRESS_LINEAR, ROBJ_VECTOR_CANVAS, etc.
- `size` - `[w, h]`, `flex(weight)`, `SIZE_TO_CONTENT`, `sh(%)/sw(%)` screen
  percent, `hdpx(px)` dpi-scaled px, `pw/ph(%)` parent percent, `fontH(%)`.
  `size = [x, y]` arrays and other constant tables/arrays should be marked
  `const` (Quirrel compile-time constant) or hoisted to `let` outside the
  builder, so a new array is not allocated on each rebuild - commits adding
  `const` or hoisting literals out of builders are perf fixes.
- `children` - array or single child; null children allowed.
- `watch` - observable or array of observables (see above).
- `behavior` - one or more of Behaviors.Button, .TextInput, .TextArea,
  .Pannable, .WheelScroll, .Marquee, .DragAndDrop, .RtPropUpdate, etc.
  Event props per behavior: `onClick`, `onChange`, `onReturn`, `onHover`,
  `onElemState`, `onDoubleClick`, `onAttach`, `onDetach`, hotkeys, etc.
- layout: `flow` (FLOW_VERTICAL/HORIZONTAL), `halign/valign` (children
  alignment), `hplace/vplace` (self placement), `gap`, `margin`, `padding`,
  `pos`, `minWidth/maxWidth/minHeight/maxHeight`, `clipChildren`,
  `sortOrder/sortChildren`, `zOrder`.
- `key` - stable identity across rebuilds; equal keys match old and new
  children first. Wrong/missing/non-unique keys cause: element state (scroll
  pos, input focus, animations) jumping to wrong items, fade-out playing on
  the wrong item, needless recreation. Duplicate keys are not reported; the
  first free match wins.
- `transform` - `{ pivot, rotate, scale, translate }`, render-only. Must be
  present (even `transform = {}` / `true`) for transform animations to work.
- `animations` / `transitions` - declarative anims; `play=true` on appear,
  `playFadeOut=true` on removal (needs stable `key`), `trigger` names.
- `onAttach` / `onDetach` - script lifecycle hooks; correct home for side
  effects, timers (`gui_scene.setTimeout/setInterval` with matching
  `clearTimer` / `gui_scene.clearTimer` in onDetach), event subscriptions.
- `eventHandlers` / hotkeys / `onElemState(stateFlags)` with S_HOVER,
  S_ACTIVE, S_KB_FOCUS bit flags.

## Rebuild model

Watched change -> element(s) watching it are invalidated -> next frame the
builder closures re-run -> new description diffed against old -> changed
subtrees rebuilt, unchanged children reused (matched by key, the same
description table, or the same behavior list). Consequences:

- Descriptions that are re-created identically every time (new closures,
  new arrays/tables inline) defeat reuse and cost CPU. Hoisting static
  parts to module scope (`let`) or `const`, and memoizing component
  factories, are real perf fixes.
- Persistent per-element state must live in Watched/observables or keyed
  storage, not in local variables of a builder (they reset on rebuild).
- Big `watch` at a high-level container = large subtree rebuilt per change.
  Pushing `watch` down to small leaf components is a perf improvement.

## Quirrel language notes (for reading diffs)

- Tables `{a = 1}`, no commas needed between slots on
  separate lines; `<-` new slot assignment; `@(x) expr` lambda;
  `$"text {expr}"` string interpolation; `?.` null-propagation; `??`
  null-coalesce.
- `let` = immutable binding (preferred), `local` = mutable.
- `const` = compile-time constant; `const [1,2]` / `const {a = 1}` literals
  avoid per-call allocation.
- Modules: `require("path.nut")` returns module exports (cached);
  `from "module" import name1, name2`; `import "module" as m`. Migration
  from require to import statements is stylistic unless it fixes a real
  double-execution/cycle problem.
- `freeze(obj)` returns frozen reference to the object.
  Sharing one mutable table/array between components (default parameter
  tables, module-level mutable tables passed as `children`, `__merge`
  results aliasing) causes cross-component contamination; fixes clone
  (`.map`, `clone`, spread) or freeze.
- Truthiness: 0, null are false-ish, empty string/array/table are TRUTHY -
  `if (arr.len())` vs `if (arr)` bugs.
- Integer division: `1/2 == 0`; float needed for fractions.
- A function parameter whose default value is a mutable table/array literal
  (`function f(opts = {}) {...}`) shares one object across all calls that omit
  the argument; mutating it leaks state between calls.
- Common stdlib in UI code: `array.map/filter/reduce/each/findindex/
  findvalue`, `tbl.__merge(other)` (returns new table), `tbl.__update`
  (mutates in place - aliasing hazard when applied to shared tables).

## Category definitions (use these consistently)

- FRP-misuse: wrong Watched/Computed usage - missing watch entry, mutation
  without mutate(), set inside computed/builder, wrong dependency, misuse
  of trigger/subscribe semantics, equality/identity mistakes.
- builder-side-effect: any side effect performed in a component builder
  function body instead of onAttach/onDetach/event handler.
- subscription-leak: subscribe/timer/event-handler registered without
  matching cleanup, or cleanup in wrong place.
- lifecycle: onAttach/onDetach misuse, missing key, element reuse/identity
  problems, fade-out/animation lifecycle issues.
- layout/behavior-misuse: wrong size mode, SIZE_TO_CONTENT with
  unmeasurable children, flex misuse, wrong behavior properties, clipping,
  hit-area/input issues.
- performance: allocations per rebuild, missing const, over-broad watch,
  heavy computeds, needless rebuild, RtPropUpdate misuse, big lists without
  keys, excessive getInt/deep table walks per frame.
- quirrel-language: language-level bugs - truthiness, integer division,
  shared mutable tables, scoping, null-propagation, require/import issues.
- state-management: app-level state modeling errors - duplicated or
  desynced state, stale closure captures, persist misuse.
- event-handling: wrong event handler, missing consume/propagation flag,
  hotkey mistakes, focus handling.
- other-bug: real fix that fits none of the above.

Judge each commit by its DIFF, not only its message. Feature commits or
content tweaks are not evidence of mistakes; skip them.
