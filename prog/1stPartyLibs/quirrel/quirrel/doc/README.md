# Quirrel reference site

A dense, cross-linked reference in the style of cppreference: one page per symbol,
a declaration block taken from the VM, and an example whose output is checked by
CI.

    python build.py                 # builds sq with cmake into build/bin, refreshes the dump, writes _site/
    python build.py --sq <path>     # with an interpreter you already built
    python serve.py                 # then open http://localhost:8000/

The build needs python3. It makes the interpreter, then runs it to refresh
`gen/api_dump.jsonl`; a machine with no toolchain, which is what Read the Docs
gives the site, warns and renders from the committed dump. `QUIRREL_SQ` in the
environment is the same as `--sq`, and below `sq` stands for whichever interpreter
is in use. It replaced a Sphinx site, so `quirrel-cheatsheet.md` next to it is the
one page that is not part of the site: it is the rule sheet reviewers and agents
read, and AGENTS.md points at it.

Use `serve.py`, not `python -m http.server`: on Windows the stock server takes MIME
types from the registry, where `.js` is often `text/plain`, and a browser refuses to
start a worker served as that. The site then reports that it has no VM.

## Where each part of a page comes from

| Part | Source |
| --- | --- |
| signature, attributes, arity, types | `gen/api_dump.jsonl`, dumped from a running VM |
| one-line summary | the `docstring` field of the C++ binding |
| prose sections | `content/<module>/<name>.md` |
| example and its output | `examples/<module>/<name>.nut` and `.out` |
| the numbers on the Performance page | `content/_bench.json`, written by `../bench/benchmarks.py` |

Nothing on a page restates what the VM already knows, so a signature cannot drift
from the implementation.

## Refreshing the dump

`build.py` refreshes the dump before it writes the site. To refresh it without
writing the site, run this from the tree root, the directory above this one:

    sq doc/gen/dump_api.nut > doc/gen/api_dump.jsonl

`gen/check.py` re-runs this and compares the symbols, so a signature cannot drift
from the implementation unnoticed. It ignores the version in the dump's header: that
records the interpreter that produced the file, and CI builds the interpreter from
source, so a release bump alone would otherwise fail a commit that changed no API.

Use the interpreter built from this tree, so the dump can never describe a
different version than the source. It reaches every built-in type delegate:
`SqModules`'s constructor calls `registerTypesLib()` and `registerModulesLib()`
unconditionally, whatever the host does.

The same script under `csq-dev` dumps the Dagor module set as well (`dagor.*`,
`frp`, `DataBlock`, ...), which is how a Dagor script API site can reuse this
tooling. It is the wrong source for this site: it also carries host-only additions
such as `async.delay`, which core Quirrel does not have.

Output is JSON Lines because the host print function breaks a line longer than 64K.

## Writing a page

`content/math/clamp.md` is the model. Front matter holds `see_also`; the body holds
`## Parameters`, `## Return value`, `## Errors`, `## Notes` and `## Example`.

- Parameter names in the `## Parameters` list are checked against the real
  signature. A name the function does not take fails the build.
- An inline `` `code` `` span naming a known symbol becomes a link on its own. The
  function's own name and its parameters never auto-link, so `min` on the
  `math.clamp` page stays the lower bound rather than becoming `math.min`.
- `[text](sym:math.min)` links explicitly; an unknown id fails the build.
- `{{example:math.clamp}}` pulls in `examples/math/clamp.nut` and its `.out`.
- A second example named `<name>-basic.nut` is the short one shown first, above the
  prose. The suffix is spelled in `gen/model.py`, `gen/render.py` and
  `gen/check.py`, so renaming it is a three-site change.

## Containers and the classes inside them

`content/_meta.json` lists the containers the dump may have. A class nested in a
module is part of the module's page when the two share a `group`: `io.file` becomes
a section of `io/index.html`, so the group index lists modules only. A class the
file puts in a group of its own keeps its own page, which is how the type delegates
(`types.String` and the rest) stay separate.

A container page is ordered description, values, functions, then one section per
class it carries. Links go through `Container.link_url`, never `url`: an inlined
class has no page, only an anchor.

## Writing a narrative page

A page under `content/pages/` carries `title`, `group`, `order` and `summary` in
its front matter. `content/_meta.json` lists the groups in sidebar order, and
`order` only sorts a page inside its own group, so a new group never renumbers
another one. A group holds pages or containers, never both.

Every page group is a chapter with an overview: the page marked `group_index: true`
sorts first in the group, the sidebar heading links to it instead of listing it, and
`{{subtopics}}` on it prints one row per page of the group out of the `summary` of
each. `{{subtopics:Language}}` does the same from outside the group. So a new page
appears in its chapter overview on its own, and only its `summary` has to be written.

`content/_index.md` is the landing page, and it is the overview of
`render.INTRO_GROUP`: the sidebar heading of that group links to it. It is not a
`Page`, so it has no front matter, and `page:index` is the way to link to it.

A container group (Modules, Types) has no page of its own. Its sidebar heading goes
to the section of the landing page that lists the group, so a landing-page section
must not carry a group's name.

In a table cell, `\|` is a literal pipe. Without it a cell holding `a || b` splits
into three.

A row may carry a link target after its last cell: `| a | b | sym:types.Array.append |`
under a two column head makes the whole of `b` the link, so the reader hits the
section from anywhere in the cell. A cell that already links something of its own
keeps those links instead.

## Checking the output

`build.py` reads the pages back and follows every internal link, anchor included,
so a target that moved fails the build. `gen/check_links.py <dir>` runs the same
pass on its own.

## Examples

Each example is a real script with a committed `.out`, collected by
`../testRunner.py` as an exec test, so the `quirrel_tests` CI job verifies every
output printed on the site. Keep them deterministic: no `rand`, no clock, no
dependence on table iteration order.

Generate the expected output by running the example, never by hand:

    sq --check-stack doc/examples/math/clamp.nut > doc/examples/math/clamp.out 2>&1

From the tree root again, and with the `doc/` prefix on the script path: an
example that prints a call stack embeds the path it was invoked with, and
`gen/check.py` accepts only the one CI uses.

Keep the `2>&1`. `testRunner.py` hands the same file to `Popen` as both `stdout` and
`stderr`, so CI compares the two streams merged; an example that writes to stderr, such as
one for `error`, only matches when the expected file was made the same way. `sq.cpp` turns
off stdout buffering, so the interleaving is the source order.

## Running the samples in the browser

Every sample is editable and the Run button executes it in a wasm build of this
same VM, so a reader gets an answer from Quirrel rather than a description of it.
Ctrl+Enter runs the sample the caret is in, and an edited sample grows a `reset`
link that puts the documented one back.

Typing recolours the sample as you go. `theme/highlight.js` does that, and it has
to agree with the `highlight()` in `gen/markdown.py` that coloured the page in the
first place, or a sample would change colour the moment anyone touched it. The
keyword list and the token pattern are shared: the build writes them to
`lexer.js`. Prove the rest agrees with `node gen/check_highlight.js`, which
compares the markup for every example against the page the build produced.

Rewriting the markup on each keystroke empties the browser's undo stack, so the
editor keeps its own; Ctrl+Z and Ctrl+Y work inside a sample.

`wasm/quirrel.js` and `wasm/quirrel.wasm` are committed, so the ordinary doc build
needs no emscripten at all. Refresh them when you change VM behaviour a sample
exercises:

    wasm/build_wasm.sh            # Linux, WSL, macOS
    wasm/build_wasm.bat           # Windows; needs python3 and cmake on PATH

Either one does nothing when `wasm/quirrel.version` already matches the header:
no SDK is fetched and nothing is compiled, so running it on a current tree costs
milliseconds. Pass `--force` after changing VM behaviour without bumping the
version, which the stamp cannot see. Otherwise they install the SDK when the pinned
version is not there, so a first build needs no separate step.
`wasm/install_emsdk.sh` and `wasm/install_emsdk.bat` do only that part, for
provisioning a machine ahead of time; both are wrappers around
`wasm/install_emsdk.py`, which is the one copy of the logic. It also decides where
the SDK lives (`$EMSDK`, else `$GDEVTOOL/emsdk`, else `~/.emsdk`) and prints that
for `--dir`.

Both build scripts write the same three files: the two artifacts and
`wasm/quirrel.version`, the version they were built from. The cmake tree goes to
`build/emscripten` at the tree root, so nothing intermediate lands next to the
pages. Nothing has to be sourced first - each activates the SDK itself. Windows
needs its own script because the SDK installs `emcmake` as a `.bat`, which a
POSIX shell does not resolve.

Refresh from Linux or WSL when you have the choice. The two hosts produce different
bytes from the same sources - same emscripten, different host toolchain - so
alternating platforms rewrites a megabyte of history for no change in behaviour.
Rebuilding on the same host is reproducible: unchanged sources give a byte-identical
artifact, so a needless refresh costs nothing.

`wasm/emsdk_version.sh` pins the SDK, and the installer provisions exactly that
version. It is a no-op that touches no network when the pin is already active, which
is why the build scripts call it every time. `build_wasm.sh` drives `wasm/CMakeLists.txt`, which pulls the four
libraries in with `add_subdirectory` so the set of files can never drift from the
library build. The ninja it needs comes from the same SDK; cmake does not.

`build_rtd.sh`, which Read the Docs runs, rebuilds the VM only when
`wasm/quirrel.version` no longer matches `include/squirrel.h`; otherwise it
publishes the committed pair and the whole build is python3, in seconds. When it
does rebuild, `wasm/check_examples.js` runs every sample through the new VM and
warns if any output disagrees with its committed `.out`.

The stamp sees a version bump and nothing else. The pair is built from
`wasm/runner.cpp`, `wasm/CMakeLists.txt` and the squirrel sources those pull in, so
a behaviour change without a bump leaves it matching and the server keeps
publishing the old VM. A refresh is owed on any change to those sources, not only
on a bump, and locally that is what `--force` is for; the server has no equivalent
and cannot detect the case.

Either way the VM is optional: when a rebuild is needed and fails, the warning goes
in the log and every page still publishes, with the Run button reporting that it
has no VM.

`wasm/runner.cpp` sets up the VM exactly as `sq/sq.cpp` does, so a sample that runs
on the site runs under the test runner too. Three things differ, all of them forced
by the browser:

- the only readable file is the sample, under the name `sample.nut`. Built-in
  modules are found in the module table before any file lookup, so `require("math")`
  works and `require("./other.nut")` fails with a message that says why.
- the watchdog is armed before the sample starts, and the hook ignores the timeout
  in the shared state so that a script cannot extend its own deadline.
- there is no command line, so `__argv` holds the script name alone.

Prove that the browser agrees with the rest of the site:

    node wasm/check_examples.js

It runs every example through the wasm VM and compares with the committed `.out`.
Only the three samples in its `EXPECTED_TO_DIFFER` list may differ, each with the
reason; anything else is a defect. Run it after any change to the VM or the runner.
