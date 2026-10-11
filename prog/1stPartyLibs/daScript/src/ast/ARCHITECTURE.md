# src/ast - the front end's AST

Contract: `ARCHITECTURE_COMMON.md` (repo root). This document carries the mechanisms of this
folder that a rule cites; `ARCHITECTURE_INFER.md` carries the inference mechanisms code cites,
and `ARCHITECTURE_BUILT.md` the programs a host builds from AST nodes.
The rest of inference, with parsing and simulation, is in `skills/internal/cpp_codebase_notes.md` (repo-only).

## 1. The module-cache read in `trySerializeProgramModule` (`ast_parse.cpp`) {#module-cache-read}

The front end parses modules in require order. When a module cache is installed
(`ModuleFileCache`), each module first tries `trySerializeProgramModule`. The reader stands at
the next record and checks that the record names this file, that the size and content hash of
the bytes the compile's `FileAccess` serves for that name match (never a stat, so a file that is
not on disk under that name is covered, and a rewrite that leaves the bytes alone serves), and
that every compile-time input the record's macros pinned
(`add_module_cache_dependency` - a file's bytes, an environment variable's value under
`env:NAME`, a command-line flag's occurrences under `arg:--flag`) still has the same content. A
match deserializes the module and counts it as served; the module object is a `ModuleDas`, the
class the parser builds, because the AOT emitter asks each module whether it can be AOT'd and a
plain `Module` answers no. A served module's file record carries the file's name and length,
not its text; the reader resolves the name through the compile's `FileAccess` - the file system,
`.das_project` hook or in-memory access a parse would have gone through - and every line
reference in the module points at the object the access returns, source included, so a lint
reading a `nolint:` marker or an error printing a source line sees the bytes the access serves.
The module takes its own file from the access the way a parse leaves it (`letGoOfFileInfo`);
a name the access cannot produce keeps the record's source-less object. The record also carries
the `CodeOfPolicies` of the compile that wrote it, and a record whose policies differ from the
reading compile's is never served - a lint compile and a run compile of one graph never share a
record. The mismatch fails the record the way damage does, reparsed in place rather than cutting
the stream (damage landing on the policy bytes must not cut it either), and the writeback that
repairs it carries the new policies. The record header also carries the text collector's
answer for the file when the record was written (`collectRequireNames` over `getAllRequire`,
guards and groups already applied, names sorted), and the reader runs the same collector over the
file it is about to serve and compares: a member a group gained or a guard a build flipped changes
a module's dependencies without touching its bytes, and a record served then would carry the old
dependency set. Both sides are one function, so a require only the walk knows - a host's
`parseCustomRequire` - compares equal to itself instead of cutting the stream on every warm read.
That mismatch is a cutoff (`require set changed`), since the modules after it may have moved too.
The first mismatch is the cutoff: the reader marks the stream failed, the
module and everything after it parse from source, and the writer rewrites the whole file - the
served records re-serialized from the modules the reader restored, then the freshly parsed
ones. A record whose header matched but whose payload fails to deserialize reparses in place,
without cutting the stream, whenever the record carries a usable payload length.

A served module replays what its own init registered - the macro classes a `[call_macro]` or
`[reader_macro]` annotation adds through the module's init function come back when the record
re-instantiates its macro program - but not a registration made while the module compiled,
into itself or into another module: an annotation's `apply`, running in another module's macro
context, that calls `add_call_macro` (or any other `add_*_macro` / `add_*_annotation`) on
`compiling_module()`; the `[extern]` binder's `apply`, which adds the proxy every call site is
retargeted to into the `dasbind` builtin. The registering site marks the compiling module
(`Module::registersAtCompile` - the adapters set it when the target has no macro context yet,
which is what a module being compiled looks like; the binder sets it on the program's own
module) and the record header carries the mark. A marked record is never served: it reparses in
place, the records after it still serve, and the cache is not rewritten for it; a marked record
whose length word cannot skip it cuts the stream like a changed file. Serving it would leave any
module parsed from source against it - after a cutoff in the same read, or in a later compile of
the same process once a `shared` module is promoted - without the registrations, and its call
sites would fail to resolve (or, for a binder, find no proxy).

A builtin module's embedded source (`compileBuiltinModule`, the `unit_test.das` of `UnitTest`)
compiles with the stream hidden: its program is never read from or written to the cache. That
compile runs inside the module's constructor, and every record pins each builtin module it
references by the module's cumulative hash, the chain over the mangled names of the builtin
functions added to it. With the stream visible the embedded program was a record like any other,
written by a cold run and served by a warm one, and the module's hash then differed between the
two kinds of run - so every record naming the module failed its read on the other kind, the
entry program's included, and the hash-drift resume reparsed each of them on every start.

Every diagnostic the read prints - a record for another file, a changed file, a changed
dependency, changed policies, a truncated or version-mismatched stream, a failed payload, a
reparse in place - is
gated on the serializer's `quietCache`. The host sets that flag for the default cache, which is
on unasked for an ordinary run; ungated, each of those lines would be output every user sees on
an ordinary edit. An explicit `-module-cache` leaves the flag off, so those runs get the lines
together with the host's verdict.

A served record's time splits three ways: the decode, the finalize (`finalizeModule` -
annotations, macro-module re-simulation, `gc_collect`) and the setup `Program::serialize` runs
after the stream (symbol marking, stack allocation). The serializer accumulates the last two over
every program it read, das reading them back through `ast_serializer_finalize_usec` and
`ast_serializer_setup_usec`, and the decode is the read minus both; `-log-compile-time`'s cache
line prints decode against finalize, with the macro simulate named inside finalize.

## 2. The module scan and the descriptor manifest (`dyn_modules.cpp`) {#module-scan-manifest}

`require_dynamic_modules` walks `<dasroot>/modules/`, then the project root's, then each
`-load_module` folder, and for every `.das_module` it finds calls `init_dyn_modules`. A
descriptor is a daslang program whose `initialize(project_path)` registers require paths
(`register_native_path`) and C++ modules (`register_dynamic_module`); those two builtins in
`module_builtin_fio.cpp` are the whole surface a descriptor's effect reaches. A descriptor compiles
under `ignore_shared_modules`: the shared daslib modules it requires neither come from nor land in
the environment's promoted set, so the program compiled after the scan has the same module set -
and the same module-cache records, `daslib/builtin` first - whether the scan compiled, replayed
or was skipped with `-no-dynamic-modules`. Compiling and running the descriptors is the scan's
cost, and it grows with the number of descriptors in the tree, so the scan keeps a manifest beside each descriptor, holding the rows the
registry received from it.

The manifest is a property of the module tree, not of the script or the cwd: the rows depend on
the descriptor's bytes and on four process-wide inputs a descriptor can read - its folder
(`project_path`), the das root (`get_das_root()`), the binary kind (`das_is_dll_build()`) and the
cross-compile target (`get_cross_platform_name()`, which `dasOpenGL`'s descriptor consults to
register its module for the web target only) - so the file sits next to the descriptor, and a
read-only tree simply compiles on every start. The binary kind and the compile target pick the
file's name - `.das_module.manifest`, `.das_module.static.manifest` for `daslang_static`, the
platform name before `.manifest` for a `--jit-target` run (`.das_module.emscripten.manifest`) -
so the kinds and targets sharing one tree (both suites at once, a sweep mixing native and wasm
runs) never rewrite each other's rows, and each replays its own from its second start on. Its key
is the descriptor's size and content hash (`hash_block64`, no stat) plus those four inputs, one
line each (`root`, `dll`, `dasroot`, `target`), plus one `dep` line per file the descriptor's
compile read - every module in its program with a file name, the daslib ones included - carrying
that file's size and hash as the scan's `FileAccess` serves it (hashed once per file per
access, whatever the descriptor count); a mismatch on any of them recompiles that descriptor
and rewrites its manifest, so a descriptor whose rows come from a module it requires recompiles
when that module changes. The file is line-oriented, tab-separated, with a
format version (`MANIFEST_HEADER`) on its first line and an `end` line carrying the row count
(`dep` lines are key, not rows); a missing `end`, a count mismatch, an unknown row kind, a wrong
field count or a `dm` row whose `on_error` is not one of `RegisterOnError`'s three values is
damage, and the reader answers damage with a recompile and a rewrite, not a partial replay. The
writer refuses a descriptor whose recorded string holds a tab or newline (`field_ok`), rather
than an escaped form the reader would have to decode, and goes through a `.tmp` and a rename.

Recording is armed around one descriptor run: each builtin appends the arguments it actually
received, in order, and `register_dynamic_module` records its call whatever the outcome and adds
the das-visible module name once the load succeeded. Replay registers every `np` row as
recorded. A `dm` row carrying that name is not loaded by the scan: the row waits under the name
(`defer_dynamic_module`), and the load happens at the first require that names it. The
prerequisite walk (`getPrerequisits`) finds no module under the name and asks the loader the
scan installed (`setDeferredModuleLoader`); the loader dlopens and registers the module, runs the
`initDependencies` fixed point that `Module::Initialize` runs (`Module::InitializeDependencies`) over
the grown list, and when a module reports it cannot initialize - what it needs is deferred too -
brings every deferred module in and runs the fixed point again, which is the set an eager start
has. A row whose own dlopen fails takes the same road - every deferred module comes in, the
pending retry runs - and the require then finds the module or fails as a cold start would.
The deserializer (`Program::serialize` reading, and the module-cache record header) fetches a
builtin module by name the same way: a stream written where a module had loaded lazily is read
where nothing required it yet. `Module::Initialize` takes it too: on a half-warm tree - one descriptor compiled cold, so its
module loaded on start, beside replayed ones whose rows wait - the eager fixed point fails on
the first pass, brings every deferred module in and runs once more. A tree is half-warm
whenever two processes warm it at once, which parallel AOT batches do. The
rows and the loader are the scan's: `require_dynamic_modules` clears the rows before its
walk and installs the loader, and `Module::Shutdown` clears both, so an environment that
follows sees neither the last one's rows nor its loader. A parse that no prerequisite walk
precedes - the `compile` of a string - meets a deferred module at the parser's own require
(`ast_requireModule`) and loads it there; the loader puts the parse's program back as the
bound one, since a module's builtin das part parses under a program of its own. The load runs under one gc root of its own with the thread root's nodes parked meanwhile,
because a constructor's nodes go to the active root while a builtin das module it compiles
dumps its leftovers on the thread root, and a collect stops at a node owned by another root;
after the load every module, not only the new ones, collects from that root, since a
constructor registers into modules that exist already, and the rest is swept. A `dm` row
with no name - the recording start's load failed - replays as recorded, so the Quiet deferral
and the post-scan retry of a sibling `DT_NEEDED` dlopen behave as on a compiled start. A row
still pending after that retry whose artifact is on disk failed on something other than a
missing file - on a platform that resolves a library's imports at load (Windows, Linux without
a RUNPATH), a sibling a deferred row holds, which the retry alone can never bring in - so the
scan brings every deferred module in and runs the retry once more, the set an eager start has;
a pending row whose artifact is absent stays pending and silent, as Quiet means, so a build
without a module's artifact pays nothing for it. A replayed nameless row whose module is
registered by the end of the scan - loaded at the scan, or by that fallback - gets the module's
name written back into its manifest (the same key and dependency stamps, the row named), so the
next start defers it as a start that recorded a clean load would; a row that never loads stays
nameless. A
require guard (`require ?mod`) and `builtin_module_exists` ask whether the build has the
module (`guardModuleAvailable`): linked in, or waiting in a manifest row, which the guard
loads then - so `require ?das_metal metal/das_metal_boost` still means "on a build with
Metal", a cold start and a warm start answer alike, and `llvm`, a witness module no das file
requires unguarded, comes in through the guards dasLLAMA places on it. A guard whose name
holds a `/` is a path instead: the guard is taken when the guard's own file resolves through the
compile's `FileAccess` - the rail for a pure-das package, which has nothing C++ to guard on, and
for a witness of a cross-package dependency. A guard the build does not have skips the require
silently and without resolving the target, so a skipped require probes no file paths; a guard the
build has over a target that does not resolve is the ordinary missing-module error. Neither form
falls back to the target's own resolvability: a module's source directory sits in every checkout
whatever the build configured. A load adds
nothing to `$`: a module-cache record carries each builtin module's cumulative hash of
mangled names, and a process that loaded a different set of C++ modules would otherwise fail
every record on `$`, so a `vector<T>` of a handled element registers into the element's
module (`vectorHomeModule`, `ast_handle.h`) whichever module builds it - a module that exists
already, when the element is another module's - and only a vector of a builtin element lands
in `$`, which every library lists first because
`ModuleLibrary::addModule` puts a module's dependencies before it. The described name of such
a vector carries that module - ``ast::dasvector`ptr`Expression``, not ``$::...`` - so code
that names one compares the part after `::` (`daslib/ast_boost`'s printer). `-ignore-manifest`
reads and writes no manifest: every descriptor compiles and every C++ module loads on start,
the form a tool that enumerates modules - the MCP server - runs under.
`no_manifest()` inside `initialize` marks the descriptor as one that runs on every start: its
manifest carries the stamp and the flag and no rows, and is not rewritten. A module group is a
name that `require [group]` expands to a list: `register_module_group(group, member, guard)` in
a descriptor - recorded as a `grp` row and replayed - or `registerModuleGroupMember` from a C++
module adds a member, once (`ast_module.cpp`, one process-wide registry under a mutex, kept for
the life of the process like the native paths: a descriptor registers once per process, so a
cleared registry could only lose members); the list comes back sorted by member path, since the
scan registers in `readdir` order, which no platform promises. A member's guard is the guard a
hand-written `require ?guard member` would carry - a module name, or a path when it holds a
`/` - and is evaluated wherever the list is read; a descriptor whose das files sit in every
checkout guards its row on the C++ module only a build configured with it has, since a member
requiring a module the build lacks fails every requirer of the group, and a manifest cannot
record what a build has - its key does not see the build's artifacts. The text collector
(`getAllRequireReq`) and the parser (`ast_requireModuleGroup`) expand the same list into one
require per member, the group's guard and `public` on each, a member's own guard on that member;
a member resolves and fails as a require spelled by hand would, and a group nothing registered
adds nothing. The collector reads `require[group]` with no space as the parser does. The
requirer calls the members through `daslib/module_group`, whose `call_module_group` reads the
same list at macro time (`module_group_for_each_member`, the `rtti` module, a path guard
resolved through the compiling program's own access) and emits one qualified call per member, so
the expansion and the calls agree on the set. Membership is tree-level, so the answer does not
depend on the walk order - a module joins from its own descriptor, and the requirer names only
the group; the module-cache record stamps the expansion (sec.1), so a member joining later
re-parses the modules that require the group. With
`DAS_TRACE_MODULE_LOAD=1` the scan prints one line per descriptor - `replayed N row(s) in <sec>
(shared module load <sec>, deferred K)`, `compiled (<why>), manifest written (N row(s))`,
`compiled (no_manifest)`, `compiled (manifests ignored)`, or why a manifest was not written -
and a deferred load prints `[module] require <name>: loading the deferred <class>`, the
fallback `[module] loading every deferred module (K)`, and the scan's own fallback announces
itself first with `[module] a pending module's artifact exists - loading every deferred module`.
A replayed descriptor's time is its manifest read plus its rows, and the second number is the
share the `.shared_module` dlopen and module constructor took.

## 3. A require after the walk (`requireModuleNow`, `ast_parse.cpp`) {#require-after-walk}

`requireModuleNow(requireName, access, logs, policies)` is a `require` issued by code that runs
after the prerequisite walk - a macro, a simulate macro, an `[init]` - for a module the walk
never saw. It is a compile's: the das entry (`daslib/cross_context`, the `ast` module's
`require_module_now`) refuses a call with no program compiling or simulating, since a running
script has no compile whose stream, policies and access the walk could join, and a null access
is the compiling program's own (`Program::access`, set by `parseDaScriptEx` for a parse and by
`deserialize_program`'s access argument for a program a reader restores - a stream carries no
access, so a reader that gives none leaves a late require nothing to walk with), so a
`-project` mapping or a host's custom access reaches the late walk as it reaches the host's; a
caller restricts by passing an access of its own. It answers the module already in the process when
there is one (a promoted shared module, a linked C++ module, or a deferred manifest row, which
it loads), and otherwise walks the target's own prerequisites, compiles the missing ones the way
`compileDaScript` does - each parsed as a dependency, promoted when its program asks to be
shared, the debugger switching serialization off as it does for the host - and then the target
itself, which must be `shared`: a module that is not promoted lives only in the walk's
`ModuleGroup` and dies with it, so a non-shared target is refused by name rather than returned
dangling. The target is named by its file: the already-there answer looks the module up by the
file's stem before any parse, so a file declaring another name would compile and promote a new
module on every call, and the walk refuses the mismatch by name instead. The answer is the
module or null, and every error the walk, the dependency parses and the target's parse produced
is written to the caller's `logs`, so a caller reports the text it got rather than the compiler's
own log. The module is not a dependency of any program: its symbols are not visible to the
caller and its macros do not apply to the caller's program; the caller reaches it through its
macro context (`Module::macroContext`, `daslib/cross_context`'s `macro_context_of`). Every macro
context carries a context mutex from its creation (`makeMacroModule`), because
`invoke_in_context` locks its target and a mutex created at the first call would be created
twice by two callers. The call saves and restores the environment's bound program, compiler log
and compiling names around the walk in a scope object, since it may run mid-parse of another
module and an unwind must put them back; the symbol state its passes compute lives on its own
`Program` (sec.4), so the caller's - which may be mid-simulate with a JIT reading it - is
untouched. One recursive mutex serializes every late require in the process. The late walk has
no module cache of its own: a stream holds many compiles in order, shared modules written once
(a persistent serializer, dastest's `--ser`), read at a parse's start and written at its end.
A late require issued after the compile - a simulate macro, an `[init]` of the program - finds
the reader between records, so its modules are the next records of that compile, read from the
environment's bound serializer and written back through the same writer from its cursor
(`writtenModules`), and the host's writeback at its end repeats nothing; the hosts keep their
cache armed through `simulate` (`utils/daslang`, `utils/daslang-live`), where that case lands -
the JIT's emitter is it. A late require nested in a compile - a macro during a parse (the bound
program's `isCompiling`), a macro module's `[init]` during its record's read (the reader's
`readingRecord`) - would write its records ahead of the requirer's, which the reader meets at
the requirer's position, or read mid-record; so a nested walk hides the stream for its duration
(`CompileEnvScope`), parses its modules from source, once per process, and pushes no
record. A module whose parse failed pushes no record either, whatever the walk.

## 4. Program-scoped symbol state (`ast.h`, `ast_export.cpp`, `ast_allocate_stack.cpp`) {#program-scoped-symbol-state}

Whether a program uses a function or a global, and the slot each holds in that program's
context, are decided per program - the symbol-use pass (`markSymbolUse` and its variants,
`ast_export.cpp`) walks the whole library from the program's roots, and `allocateStack` numbers
what it marked; `markFunctionDependencies`, which `collect_dependencies` runs for a macro, walks
one function's closure only, refreshing the use-edges it reaches and leaving the rest of the
program's symbol state as it found it - while the `Function` and `Variable` objects of a shared module are one instance
for every program in the process. The state therefore lives on `Program`: `usedFunctions` and
`usedVariables` are sets, `functionIndices` and `variableIndices` maps, read through `isUsed`
and `indexOf` and written through `setUsed` and `setIndex`; `clearSymbolUse` empties the sets.
An index is `-1` for an object the allocation never saw, `-2` for one it saw and found unused,
the context slot otherwise; the constant folder tells the first from the rest. Simulate reads
the tables through `context.thisProgram`, and das code through the `ast` module's `is_used`,
`function_index` and `variable_index`, each taking the program - a null program there is a
holder nobody filled and throws, never an answer. `Program::simulate` binds itself to the
context it fills, and clears the binding at the end when the `rtti` option is off; a macro
context's binding is cleared after its simulate whatever the option, since the context lives in
the module for the process while its program is a temporary. A context that simulates an
expression outside that call takes the compiling program from the bound environment instead:
`eval_single_expression` sets it on the context it makes, and on a caller's context only when
that context has none, putting back what was there. A context that still has no program answers
-1 for every function and variable (`programIndexOf`, `ast_simulate.cpp`) - with no tables to
read, nothing is used and nothing holds a slot. The index tables are per allocation:
`allocateStack` clears both before numbering, so -1 means this pass never saw the object, and a
folding round's slots do not survive into the final pass. A compile nested inside another - a
macro calling `compile`, a late `require` (sec.3), the folding program - fills its own tables
and leaves the outer program's answers standing. The stream a module-cache record carries has
neither the flag nor the slot: both are recomputed by the reading program. Three per-compile
values still live on the objects: `Variable::stackTop` and `Function::totalStackSize` /
`totalGenLabel`, which `allocateStack` writes and the same program's simulate reads before any
nested compile can run. The JIT's selection - which functions of a program a DLL holds - is a
third table, `jitSelected`, read through `is_jit_selected` and written through
`set_jit_selected` / `clear_jit_selection`; `Function::requestJit` is the `[jit]` annotation
only, an author's request the selection reads and never writes.
