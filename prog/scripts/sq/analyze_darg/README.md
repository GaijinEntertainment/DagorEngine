# analyze_darg: build a daRg UI tree without the game

`darg_walk.nut` builds the full component tree of a daRg UI in a console
Quirrel host (csq). It does not need the game, daRg or a renderer. Use it to
compare Quirrel VM variants (for example table layouts) for time and memory
on real UI scripts.

## darg_walk.nut

`walkTree(root, opts = null)` resolves a component as daRg does:

- A function (or a callable instance) is called with no arguments. The result
  is resolved again.
- A table (or a class) is one element. Its `children` slot (an array, one
  component or null) is resolved recursively.
- An array is resolved item by item. A null is skipped.
- An instance whose class has no `_call` metamethod (a stateful component
  descriptor) is a leaf.
- After the walk, `onAttach` of each element is called once, parent first, as
  daRg does after a build. A handler with one parameter gets `ElemRefMock`.

Each builder call and each `onAttach` call is in try/catch. A cycle stops its
branch. A depth over `maxDepth` (256) or more than `maxElements` (2000000)
elements stops the walk. Each of these counts as an error.

The result keeps every builder, every element table and every children array
(`kept`), so the memory of the built UI stays alive while you keep the result.
It also gives `elements`, `builderCalls`, `attachCalls`, `maxDepth`, `errors`
and `errorList` (the first errors with a component path),
`propReads` and `propHits` (the setup reads and the keys they found), and
`sizeHist` (the element tables by key count: 0-4, 5-8 .. 33-64, more).

Each element table gets the reads that daRg does when it sets the element up:
the component build, `Element::setup`, the layout and the parameters of its
render object. The parameter reads are those of the render objects that
`ROBJ_KEYS` lists: text, textarea, inscription, image, vector canvas, box,
frame and movie. Any other render object gets only the color keys, so its
counts are not the reads of daRg. Most of these keys are absent, so most reads
miss, as in daRg. A key counts as found (a hit) when its value is not null.
`readSetupKeys(desc, counts)` does these reads for one element table and adds
the reads and the hits to `counts[0]` and `counts[1]`.

Options: `maxDepth`, `maxElements`, `maxErrorsKept`, `callOnAttach`,
`catchErrors` (false lets the first error stop the run with a full call
stack), `elemRef`, `setupReads` (false skips the setup reads).

## active_matter bench

The entry script is `active_matter/prog/utils/analyze_darg/am_ui_bench.nut`.
Its `.sqconfig` is a copy of `active_matter/prog/ui/.sqconfig` with the paths
changed. It adds the `stubs/` folder first in `base_path` and the
`%analyze_darg` mount point. Keep the copy equal to the UI config when the UI
config changes. `active_matter/prog/validateScripts.py` runs the bench with
`repeat=1`, so CI checks that it still loads and builds; a state that fails to
build does not fail CI, it shows in `UIBENCH_ERROR` lines.

Run it from its folder:

```
cd active_matter/prog/utils/analyze_darg
../../../../tools/dagor_cdk/windows-x86_64/csq-dev.exe am_ui_bench.nut -- repeat=10
```

Parameters after `--` (all optional):

| Name | Default | Meaning |
| --- | --- | --- |
| `repeat` | 10 | rebuilds of each state after the first build |
| `states` | all | comma list of `login`, `loading`, `menu`, `hud` |
| `only` | none | measure only the state with this full name |
| `menus` | 1 | 0 skips the menus and screens of `menu` and `hud` |
| `dump` | 0 | 1 prints `UIBENCH_FILE` lines: builder calls for each source file |
| `errors` | 8 | error lines to print for each state |
| `nocatch` | 0 | 1 stops at the first error with the csq call stack |
| `props` | 1 | 0 skips the setup reads |
| `data` | none | a dump folder of `active_matter/prog/utils/tests/offline_data` to load after the login |
| `scenario` | all states | `menus` runs the menu scenario instead of the states |
| `record` | none | a folder: the UI records the data that it gets into it, as in the game |
| `sizes` | 0 | 1 prints the size of each store that holds the data, after the load |

`data` loads the dump through the entry points of the UI (the same handlers
as the game, see the README of the dump folder): the profile with the items,
the premium balance, the queues, the schedule, the contacts and the
presences. `UIBENCH_DATA_ERROR <kind> <message>` shows a kind that failed. A
path is relative to the current folder, for example
`data=../tests/offline_data/synthetic` from `utils/analyze_darg`. `record`
with `data` gives a recorder test with no game: the recorded files of the five
required kinds must be equal to those of the dump. The recorder stops before
the presence events, and its `manifest.json` has the time and user of the run.

The script loads `%ui/root.nut` (the UI does not need to be the main script),
logs in a fake user, adds a fake player and hero entity, and builds the tree
of these states:

- `login`, `loading`: before login, and logged in with no level.
- `menu`: out of battle (the player base). Then each menu of
  `playerBaseHudMenus` (`menu.<id>`), the tabs of the Options window in
  `SETTINGS_TABS` (Graphics, Sound, Game, VoiceChat, Chat, Streaming,
  KbdMouse and Controller; the Interface tab is not built), a message box,
  three navigation scenes and three modal windows. The credits window is not
  built: with no license texts (they are in a vromfs) it sends a logerr.
- `hud`: in battle. Then each menu of `battleHudMenus` (`hud.<id>`).

The debriefing menus get the sample data of the UI
(`baseDebriefingSample.nut`).

For each state the script makes one first build and `repeat` rebuilds. Before
each build it calls `update_deferred()` 8 times, so deferred FRP updates and
the effects of `onAttach` arrive as they do in the next frame of the game.

### Menu scenario

`scenario=menus` acts as a player on the player base (a fake entity has the
`player_base` tag): after the login only one menu is open at a time, and the
script keeps no build. The steps are:

1. `base`: the player base with no menu.
2. Each menu of `playerBaseHudMenus` in key order: open, build, close.
3. One level deeper, one step for each tab or sub-page of a menu with tabs
   (`MENU_TABS`): the contacts window modes, the market categories of the
   loaded offers, the alter selection, the raid preparation, the AM
   processing filters, the journal tabs, the harmonization tabs and the
   monolith levels. The step name is `<menu id>/<tab>`.

To close a step the script closes the menu and all modal windows, and does
the resets of `onDetach` that the harness does not call (the contacts popup,
the raid preparation window). The debriefing menus get the sample data only
when there is no `data`. `states`, `only`, `menus` and `dump` have no effect
here.

## Output lines

- `UIBENCH <phase> <ms>`: `load` (require of root.nut), `first.<state>`,
  `best.<state>` (best of the rebuilds), `all` (all states, setup included),
  `sum_first` and `sum_best` (sums over the states), `props` (the setup reads
  alone over the first tree of each state, best of `repeat` runs).
  Use `sum_best` and `props` to compare VM variants, in the layout variants of
  `prog/1stPartyLibs/quirrel/quirrel/bench/vmperf/README.md`: code layout alone
  moves these numbers by several percent. With `repeat=0` the script does not
  print `best.<state>`, `sum_best` and `props`.
- `UIBENCH_MEM <phase> <kb>`: memory from `dagor.memtrace` after `load`, after
  the login (`ready`) and after all builds (`built`). All builds stay alive.
- `UIBENCH_COUNT <state>.<name> <n>`: counts of the first build. When the
  last rebuild has other `elements`, `builderCalls`, `errors` or `maxDepth`
  counts than the first build, also `rebuildElements` and
  `rebuildBuilderCalls` of the last rebuild. `rebuildErrors` and
  `unsettledRebuilds` (rebuilds with other values of these four counts than
  the rebuild before) show only when they are not zero.
- `UIBENCH_COUNT total.<name> <n>`: sums over all states. `uncaught` counts
  errors outside the walk (for example in FRP subscribers). `tableSize.<0to4 ..
  gt64>` counts the element tables by key count.
- `UIBENCH_ERROR <state> <message> AT <path>`, `UIBENCH_UNCAUGHT <message> AT
  <file:line>`: the first errors.
- `CHECK ...`: the counts that must be equal on each run with the same
  parameters, and a signature of the counts of all states. Timing and memory
  are not in it. With `data` it also has `dataErrors`.

The menu scenario prints for each step `UIBENCH menus.<step> <ms>` (the first
build), `UIBENCH best.menus.<step> <ms>` (best of `repeat` rebuilds),
`UIBENCH_MEM menus.<step>.open <kb>` (the menu is open and its build is
alive) and `UIBENCH_MEM menus.<step> <kb>` (after the close). Before each
memory read it calls `collectgarbage()`. At the end it prints `sum_first`,
`sum_best`, `all` and
`CHECK scenario=menus steps elements builders errors dataErrors uncaught sig`,
with the counts of the first build of each step.

The CHECK line does not change with the table iteration seed
(`--iter-seed:<n>`) or from one process to the next on the same day.
`dagor.random` has a fixed seed and the `datetime` stub returns zeros, but the
wall clock can change a screen on another day (see Known gaps).

### Memory report

With `data` the script loads the kinds in three groups: `profile` (the
profile with the items and the premium balance), `queues` (the queues and
the schedule) and `contacts` (the contacts and the presences). For each group
it prints `UIBENCH_MEM data.<group>.before <kb>`, `UIBENCH_MEM
data.<group>.after <kb>` and `UIBENCH_DATAMEM <group> <kb>` (the difference,
which also has the state that the handlers make), then
`UIBENCH_DATAMEM total <kb>` and, for each kind,
`UIBENCH_DATASIZE <kind> <bytes>`: the exact size of the loaded answer
(`get_quirrel_object_size` of a copy). With `sizes=1` it then reads each
exported value with a `get` member (a Watched or a Computed) of
`profileState.nut`, `allItems.nut`, `matchingQueues.nut`, `contactPresence.nut`
and `contactsWatchLists.nut`. It prints `UIBENCH_STORE <module>.<name> <bytes>`
for each one that holds more than 2 KB and `UIBENCH_STORE total` of all of
them, the small ones too; two stores that share a table both count it.

The menu scenario then prints `UIBENCH_MEM menus.done <kb>` (all menus closed)
and `UIBENCH_DATAMEM derived <kb>`: the growth from `menus.ready` to
`menus.done`, that is the state that the menus compute and keep. To get the
part that comes from the data, subtract `derived` of a run with no `data`.
That run loads the sample debriefings and has fewer steps, so the result also
holds the state of the steps that only the data opens.

Each memory value is `get_memory_allocated_kb()` after `collectgarbage()`.
It counts all memory of the process and has steps: a small allocation can use
memory that the allocator already has, so a small kind can show 0. Run the
report without `--static-analysis`, which adds its own memory.

Example on the synthetic dump, from `active_matter/prog`:

```
../../tools/dagor_cdk/windows-x86_64/csq-dev.exe utils/analyze_darg/am_ui_bench.nut -- data=utils/tests/offline_data/synthetic scenario=menus repeat=1
```

## Mocks

`stubs/ecs.computed` replaces the shared stub: the mirrors read a list of fake
entities (`setFakeEntities`), with the comps_rq, comps_no and default rules of
the native module. Mirrors with a filter stay at the default value. Each other
module comes from the shared stubs, as for the static analysis of the UI.

The mock also runs the callbacks of `ecs.SqQuery.perform`, through
`set_stub_query_handler` of the shared `ecs` stub (with no handler the stub
does not call them). Only a fake entity with `compPrefix` takes part: a query
finds it when the entity has no component of `comps_no` and each component of
`comps_rw`, `comps_ro` and `comps_rq` starts with the prefix. The filter string
of a query has no effect. A missing component gets the default of the query or
of its type, and a write updates the mirrors that read it. With `data` the
script adds the profile entity (`compPrefix = "player_profile__"`), so the
profile handlers write the items and the other profile components, and
`mkEcsComputed` gives them to the UI.

The template DB of the shared `ecs` stub has only the templates that
`set_stub_templates` gives it. The bench gives it the fake reward templates
of the base upgrades (`base_upgrade_*` of
`gameBase/gamedata/templates/am_items.blk`): the monolith gate sends a logerr
for a base upgrade offer with no template. A template gives only its own
parameters, not those of its `_use` parents.

## Known gaps

The harness does not do these things that daRg does:

- Element setup: the harness reads the keys that daRg reads, but it does not
  use the values, and it does not read the keys of behaviors.
- Layout and rendering: sizes are not calculated (`calc_comp_size` gives
  zero), no render objects, no pictures and no fonts.
- Behaviors, hotkeys, event handlers, `onClick`, `onElemState`, `onDetach`,
  animations, transitions and timers (the timer stubs do not call back).
- FRP-driven partial rebuilds: daRg calls only the builders of invalidated
  elements and reuses the unchanged children. The harness rebuilds the full
  tree each time.
- The `cursor` slot, panels and stateful components are not walked.
- daRg accepts a builder result only when it is a table, a class or null. The
  harness also resolves a returned function or array.
- A `logerr` is not counted, but csq then exits with code 1 and CI fails.

What the active_matter bench does not reach:

- ECS data: only one fake hero exists and `register_es` handlers never run.
  The in-battle HUD shows no weapons, items, markers or teammates. With
  `data` the profile entity holds the items, but no item entity is made, so
  the stash and inventory grids stay empty.
- Profile and server data: with no `data`, inventories, market offers,
  contracts, raids and contacts are empty, so most lists have no rows.
- The template DB stub gives no item templates (only the base upgrade
  ones), so item types fall back to their defaults (all market offers are in
  one category).
- The tab state of the sleeve customization, the audio module, the two
  debriefings and the complex contracts is not exported, so the menu
  scenario builds only their first tab.
- The replay HUD, the editor, FMV playback, most modal windows (they need
  arguments), tooltips, context menus and the other navigation scenes.
- The UI reads the wall clock (`dagor.time` is native). The counts did not
  change between runs, but a screen gated by date can change on another day.
