# Bumping the vendored daScript

This directory is a **curated subset** of upstream daScript, not a mirror, and it carries
dagor-local patches. A bump that copies upstream over it silently drops those patches and
adds trees dagor must not build. Work through the steps below in order.

## 1. Choose the upstream revision

Use a revision that is reachable from upstream `origin/master`. A local rebase or a branch
tip that was never pushed gives a sha nobody else can resolve, and it mixes unrelated work
into the bump. Check before you start:

    git -C <upstream> branch -r --contains <rev>

Name that sha in the commit message. Keep the JIT/LLVM work in its own commit on top.

## 2. Copy upstream content

Rules that keep the subset curated:

- Update files already tracked here; delete files upstream removed.
- Add a new upstream file only when its **parent directory already exists** here. Upstream
  keeps whole module trees dagor does not build. **This rule is silent when it is wrong**: a
  tool that IS carried can name a directory it skipped, and nothing fails until someone runs
  that tool. After the copy, check that every `.das` a carried tool spawns out of
  `get_das_root()` is actually here - that is how `utils/gen1-to-gen2` (the converter
  `utils/mcp/tools/convert_to_gen2.das` runs) went missing while the MCP server around it
  was carried:

      V=prog/1stPartyLibs/daScript
      grep -rhoE '\{get_das_root\(\)\}/[A-Za-z0-9_./-]+\.das' $V --include=*.das \
        | sed 's|{get_das_root()}/||' | sort -u | while read -r t; do
          case "$(basename "$t")" in _*) continue ;; esac   # tests' own temp names
          [ -e "$V/$t" ] || echo "MISSING: $t"
        done

  Two hits are expected, both deliberate: `tests/gc/_sub/nonexistent.das` and
  `tests/lint/foo.das` are names their tests expect to be absent.
  `tests-cpp/big/standalone_ctx/` is carried for its three `.das` fixtures alone
  (`standalone_init_fixture.das` and its `standalone_init_dep.das`, `standalone_modules_fixture.das`),
  which `tests/jit_tests/jit_lib.das` spawns; the C++ and CMake beside them are not carried.
- Never carry: all of `src/hal` except `crash_handler.cpp` (see below), `include/vecmath`,
  `include/dag_noise`, `include/fast_float` (dagor has its own), `utils/internal`, and
  tests for modules dagor
  does not build (dasHV, dasPEG, dasPUGIXML, dasSQLITE, strudel, spoof, dasClangBind,
  dasGlfw, dasOPENAI, dasOpenGL, dasStbImage).
- **Preserve, do not overwrite**: `modules/dasImgui` and `modules/dasQuirrel` (generated
  here, against dagor's own imgui), `das_config.h` (dagor override: EASTL +
  `ska::flat_hash_map`), every `jamfile`/`*.jam`, `dagorAdapter/`, `das-fmt/`,
  `_colorer4/`, `far3colorer/`, `include/daScript/das_common.h`,
  `daslib/quirrel_boost.das`, and `utils/daScript/` except `main.cpp`. dagor's CLI
  `utils/daScript/main.cpp` is upstream's `utils/daslang/main.cpp`: copy it from there and
  re-apply its row in step 3.

Of `src/hal`, dagor carries **`crash_handler.cpp` and nothing else**, and the `jamfile`
names `src/hal` in its autoscan list with `project_specific_file_info.cpp` and
`performance_time.cpp` in the exclusion regex. Do not "simplify" this into dropping the
directory. `crash_handler.cpp` is what defines `das::print_current_stack_trace`, which
`src/misc/gc_node.cpp` calls unconditionally from the gc break, so dropping it makes the
symbol undefined in **every** das host and tempts a per-host definition - 13 of them, each
a flat `debug_dump_stack` in place of upstream's das-aware walk. The other six files are the
ones that clash: `project_specific_file_info.cpp` defines `get_file_access` / `get_context` /
`get_clone_context`, which the game supplies (`das_es.cpp` + a `defaultFileAccess.cpp`) or
`dagorAdapter/da.cpp` for tools; `performance_time.cpp` defines the `ref_time_ticks` family,
which dagor's engine kernel already provides; `debug_break.cpp` defines `os_debug_break`,
which every host defines itself. A das host still supplies `os_debug_break` and
`install_das_crash_handler` on its own.

## 3. Re-apply the dagor-local patches

This is the complete list of what dagor carries on top of upstream. None of it is optional,
and a bump that copies upstream over this directory drops all of it. After the copy, check
every row. When a row's patch turns out to be identical to upstream, upstream adopted it -
delete the row rather than leaving a future bump hunting for it.

### Behaviour patches

| File | Patch |
| --- | --- |
| `include/daScript/ast/ast.h` | `BuiltInFunction::noFolding` + `noFold()`; `Module::compileBuiltinModule` member spellings; the `das_pull_<Module>` thunk in `REGISTER_MODULE` and `REGISTER_MODULE_IN_NAMESPACE` |
| `include/daScript/daScriptModule.h` | `NEED_MODULE` pulls through `das_pull_##ClassName` and declares it itself, so the ~970 game module entries need no `DECLARE_MODULE` |
| `include/daScript/daScriptBind.h` | `das_bind_no_fold`, wrapped around `DAS_ADD_FUN_BIND` and `DAS_ADD_FUN_BIND_VALUE_RET` |
| `src/ast/ast_const_folding.cpp` | `foldableBuiltin()` on the four folding sites, and the `noFolding` check in `isConstExprFunc` |
| `src/ast/ast_parse.cpp` | `get_file_access` stays a declaration; the executable supplies it |
| `src/misc/alloc_tracker.cpp` | `std::_Exit(1)` in the leak dump stays commented |
| `src/misc/daScriptC.cpp` | `initialize_modules_impl` at global scope, not the anonymous namespace upstream puts it in (see the `NEED_MODULE` note in step 4) |
| `include/daScript/simulate/code_of_policies.h` | `version_2_syntax = false` |
| `utils/aot/main.das`, `utils/aot/main_llvm_aot.das` | `cop.no_aliasing = true`, mirroring `das_scripts.cpp`. **Losing this costs the game its AOT silently** - step 6 |
| `daslib/lint_everything.das` | `paranoid(prog, true, build_lint_macro_disabled(prog))`, not the 2-arg overload, whose empty disabled set arms default-off rules (LINT029, STYLE005) and ignores the `.lint_config` cascade. Belongs upstream |
| `daslib/command_line.das` | `get_das_exe()` falls back to argv[0] itself - normalized for `cmd.exe`, with the `.exe` suffix appended - because dagor's interpreter is `das-dev`, not `daslang`. Upstream returns raw `argv[0]`, which `cmd.exe` cannot launch, so every subprocess test fails on Windows only |
| `daslib/decs.das` | `[never_inline]` on `create_entities_from_cmp`. Auto-inline splices the block literal into the caller, and `ensure_capacity` - a generic from a privately required builtin - then fails privacy outside `decs` |
| `daslib/debugger.das` | `@ignore_heap_usage` on `g_installed_agents`. Without it dagor's "global variable requires heap memory" check logs an error per script that requires the module, which is **fatal on WT** (`visualErr` asserts `!total_logerr` at shutdown) |
| `daslib/debugger.das`, `daslib/quote.das`, `daslib/rtti.das`, `daslib/lint.das` | `options no_global_variables = false`. A mutable global in a daslib module otherwise fails every game script with `error[30930]` |
| `modules/dasLLVM/daslib/llvm_aot.das`, `llvm_jit_run.das`, `llvm_jit_common.das` | an LLVM-AOT object defines the `aot_<obj>_pull` anchor jam's pull list links it in by, moves every `llvm.global_ctors` entry into `.init_array` on ELF (the C API leaves UseInitArray off, so the list lands in legacy `.ctors`, which lld and mold never run), and is written with the suffix jam names it by (`.obj` on windows). **Losing this breaks every LLVM-AOT game link** |
| `modules/dasLLVM/daslib/llvm_macro.das`, `modules/dasLLVM/daslib/llvm_tune.das` | `options no_global_variables = false`, and `no_aliasing = false` on the tuner: its sidecar reader returns a table built from json values, which the aliasing pass sees as reaching json's null sentinel, so a host compiling with `no_aliasing` cannot load it |
| `src/simulate/debug_info.cpp`, `include/daScript/simulate/debug_info.h`, `src/ast/ast_module.cpp` | `cppBindingLineInfo`'s cache is releasable and `Module::shutdownInternal` frees it once the last environment is gone. Upstream leaves ~2000 `FileInfo`s immortal behind an `AllocTrackerInternalGuard`, which hides them from das's own tracker but not from dagor's, and a dagor dev build **fails the run** on `!!! MEMORY LEAKS !!!`. Belongs upstream |
| `utils/daScript/main.cpp` (copy of upstream `utils/daslang/main.cpp`) | no `modules/external_declare.inc` / `external_pull.inc` (dagor has no daspkg-generated modules), and the leaked-smart-pointer `exit(1)` sits under `#ifndef DAS_AOT_COMPILER` - game contexts live to exit by design |
| `modules/dasStdDlg/src/dasStdDlgWin.cpp` | `std::` qualification and `.c_str()` - dagor `string` is eastl |
| `include/daScript/daScriptC.h`, `include/daScript/misc/platform.h`, `include/daScript/misc/wyhash.h`, `include/daScript/simulate/aot.h`, `src/misc/job_que.cpp`, `src/builtin/module_builtin_runtime.cpp` | ARM64EC (`_M_ARM64EC`, which also defines `_M_X64`) takes the ARM64 branches: NEON, popcount, 128-bit multiply, spin hint, CPU-feature detection; `get_architecture_name` answers `"arm64ec"`. Belongs upstream |
| `src/runtime/context.cpp` | `makeWorkerFor` copies `maxFastCallDepth`. Upstream leaves the worker at 0, so a capped program's checked fastcall nodes panic on the first call in a parallel ES. Belongs upstream |

### Test adaptations

| File | Patch |
| --- | --- |
| `tests/fio/test_process.das` | `resolved_das_root()`; the suite runs with a relative `-dasroot` and one case gives the child its own cwd |
| `tests/mcp/test_popen_argv_pipe.das` | `resolved_das_exe()` fallback; `get_das_exe()` assumes a binary named `daslang` |
| `tests/fio/popen_argv.das` | `test_help_after_separator_reaches_the_script` passes `-dasroot` to its child, like the file's other fixture spawn |
| `tests/daslib/test_rst_distinct_ctor.das` | `test_rst_comment_reads_distinct_typedef` passes `-dasroot` to its driver |

A child with no `-dasroot` derives the root from its own path, which only works for a binary
at `<root>/bin/`; das-dev is `tools/util/das-dev`, so such a child finds no `daslib`.
| `tests/language/reflection.das` | `test_compile_sample` sets `version_2_syntax` explicitly instead of relying on the default |
| `tests/language/cpu_supports.das` | `"arm64ec"` takes the `arm64` arm: an ARM64EC process runs on arm64 silicon and answers arm feature names |
| `tests/language/fast_call_depth.das` | `test_host_policy_and_host_catch` writes its fixture with `create_temp_file_result` and runs without a `run` lambda: the vendored dastest has no `test_temp_dir()`, and an `inscope` local inside a lambda fails to compile (50625) |

Tests **not carried**, because they cannot pass here and are not language bugs:
`tests/mcp/test_mcp_jsonrpc.das` (needs dashv), `tests/dastest/test_preflight_config.das`
(needs `utils/internal/preflight`), `tests/dastest/test_dasfmt_exclude_mask.das` and
`modules/dasLLVM/tests/llvm_cache_selfheal.das` (both spawn the interpreter with no `-dasroot`,
or expect upstream exit codes; das-dev is not at `<root>/bin/`, so it derives no dasroot).
`tests/module_cache/` is not carried either: 26 of its 54 cases fail under das-dev, even
with an absolute `-dasroot`.

### Mechanical normalization, redone every bump

Not logic, but the CI checks in step 7 fail without it.

| File | What |
| --- | --- |
| `src/parser/ds_lexer.cpp`, `ds2_lexer.cpp`, `lex.yy.h`, `lex2.yy.h` | tabs expanded on 4-column stops, trailing whitespace stripped |
| `src/parser/ds_parser.ypp`, `ds_parser.hpp`, `ds2_parser.ypp`, `ds2_parser.hpp` | the redundant optimize-off pragma line removed |
| `include/daScript/simulate/simulate_gc.h` | trailing whitespace stripped |
| `modules/dasLLVM/CMakeLists.txt`, `modules/dasUnitTest/CMakeLists.txt`, `modules/dasStdDlg/CMakeLists.txt` | tabs expanded on the lines the bump adds only |
| `daslib/aot_cpp.das` | the two emitted-pragma lines kept byte-identical to the previous revision, so they are not added lines |

### Preserved wholesale

These are not line patches - never overwrite them from upstream: `modules/dasImgui` (~55
generated files, built against dagor's own imgui) and `modules/dasQuirrel`.

Roughly 185 tracked files here have no upstream counterpart at all: `das_config.h` (the
EASTL / `ska::flat_hash_map` override), every `jamfile` and `*.jam`, `dagorAdapter/`,
`utils/daScript/` other than `main.cpp`, `das-fmt/`, `_colorer4/`, `far3colorer/`,
`include/daScript/das_common.h`, `daslib/quirrel_boost.das`, and this file.

**Comparing which files differ is not enough.** A dagor patch that upstream also touched shows
up as ordinary drift, and three of them were lost that way in one bump: `cop.no_aliasing`,
`@ignore_heap_usage`, and the `get_das_exe` fallback. Grep master's copy of every file for
dagor's markers and check each hit survived:

    V=prog/1stPartyLibs/daScript
    git ls-tree -r --name-only origin/master -- $V | sed "s|^$V/||" | while read f; do
      [ -f "$V/$f" ] || continue
      comm -23 <(git show "origin/master:$V/$f" | grep -E "dagor|@ignore_|no_global_variables|no_aliasing|das_pull_|DAGOR_" | sort -u) \
               <(grep -hE "dagor|@ignore_|no_global_variables|no_aliasing|das_pull_|DAGOR_" "$V/$f" | sort -u)
    done

**Verify the whole tree, do not trust the copy.** Compare every tracked file's blob against
the pinned revision; the only files that may differ are the ones listed above. Anything else
is a lost patch or a stale copy:

    U=<upstream clone>; V=prog/1stPartyLibs/daScript
    git ls-files $V | sed "s|^$V/||" | while read f; do
      git -C $U cat-file -e "<rev>:$f" 2>/dev/null || continue
      git -C $U show "<rev>:$f" | cmp -s - "$V/$f" || echo "differs: $f"
    done

## 4. Adapt the dagor side to upstream API changes

Upstream renames and removals surface as build errors far from this directory. Recent ones,
as a guide to the shape of the work:

- A removed header whose content moved (for example `ast_typefactory_bind.h`, whose
  `typeFactory<ResT(*)(Args...)>` went to `ast_typedecl.h` and `cast_arg<char*>` to
  `interop.h`) needs every `#include` of it dropped across the repo.
- A constructor that gained a required argument (for example `TypeDecl(Type)` now needs a
  `LineInfo`) needs `das::cppBindingLineInfo()` at every C++ binding site.
- A removed `Function` flag needs its setters removed from the binders here
  (`modules/dasImgui`, `modules/dasQuirrel`).
- A macro upstream's CMake defines for every `libDaScript` target has no jam twin, so the
  vendored `jamfile` has to define it: `DAS_ENABLE_DYN_INCLUDES=1` is one, and without it
  `src/builtin/jit_runtime.cpp` calls `describe_pending_dynamic_modules` with the header its
  own `#ifdef` skipped.
- A generated helper that moves to another module takes its visibility with it: `typeFactory<vector<TT>>`
  now registers `dasvector`TT`, `length` and the rest in the ELEMENT type's home module rather
  than `library.front()`, so a script that reaches such a vector through a wrapper module needs
  that module re-exported - `addBuiltinDependency(lib, require("<home>"), true)`.
- A module dagor builds that gains an upstream source needs its `jamfile` updated: the
  jamfiles are dagor-only and name every source, so a new file is silently not compiled and
  the module's own registration then fails to link. `modules/dasStdDlg` gained the tray API
  this way; dagor wires only the shared layer and `dasStdDlgTrayNotImplemented.cpp`, since no
  dagor host shows a tray icon and the linux backend wants libdbus-1 headers.

When you delete the body of a loop or an `if` during such an adaptation, delete the
statement too. A loop whose body became empty leaves its variable unused, which linux does
not report and the Windows CI rejects with `-Werror,-Wunused-variable`.

**`NEED_MODULE` only works at global scope.** The dagor macro declares
`das_pull_<Module>` itself, and a declaration inside a namespace declares a
namespace-local symbol, so a pull upstream moved into a namespace fails to link with
`has internal linkage but is not defined` for every module in the list. Move the pull to
global scope; do not make the macro `extern "C"`, because several dagor stubs
(`moduleStub.cpp`, `stubDasModule.cpp`, `dasGameModulesList.cpp`) define `das_pull_*` by
hand with C++ linkage, some of them `__cdecl`.

## 5. Build and test, in this order

Build with clang and keep the load down (`-j10`, or `-j15` under `nice`).

1. `cd utils/daScript && jam -j10 -sPlatformSpec=clang` - builds `tools/util/das-dev`.
   Missing symbols here usually mean upstream split a file into new ones that step 2 did
   not copy, or added a `src/` subdirectory that the `jamfile` autoscan list does not name.
2. The vendored suite, which is what the `dascompile` CI job runs:

       cd tests && ../../../../tools/util/das-dev -dasroot .. \
         _dasroot_/dastest/dastest.das -- --color --test .

3. Each game's AOT tool: `cd <game>/prog/aot && jam -j8 -sPlatformSpec=clang -sConfig=dev
   -sUseDlss=no`. Build the tool before using it; a pre-bump binary reads the new `daslib`
   and fails in ways that look like script bugs.
4. Each game's `validateDascripts` arg set (see `validateDascripts.py`/`.bat` per game),
   with `--ast-verify-batch` added. Judge by `[E]` lines, not the exit code: a non-zero
   exit with no `[E]` is the dagor leak-dump artifact. `ast_verify`'s `[simulate_macro]` arm
   reads `compiling_module()`, which throws `compileModule only available during compilation`
   in the game AOT hosts and aborts the dry run; the `[post_compile_macro]` arm still runs, so
   read the findings from a run without the flag when that fires.

**Every `get_file_access` implementation must read `if ( pak && *pak )`, not `if ( pak )`.**
An empty project path otherwise builds a `ModuleFileAccess` for a project named `""`. There
are two faces of the same bug: the vendored test suite exits 1 with no output at all,
because the `_Exit` discards buffered stdout, and a game's AOT step dies with
`FATAL ERROR: failed to compile: error[20605]: missing prerequisite ''`. The
implementations live in the game and tool hosts, not here. Find them without an extension
filter, because eden's is a `.cpp.inl` and `--include=*.cpp` walks straight past it:

    grep -rnE "get_file_access *\( *(char|const char) *\*" prog/ eden/ skyquake/ enlisted/ \
        active_matter/ outerSpace/ | grep -v 1stPartyLibs/daScript

Every one needs the guard, including the ones spelled `pak ?` rather than `if ( pak )`.

## 6. Run a game and prove the AOT is LINKED, not just initialized

Nothing in step 5 catches a bump that costs the game its AOT: the vendored suite, the AOT
tools and the `--ast-verify-batch` sweeps all pass while every entity system silently falls
back to the interpreter. Build one game with C++ AOT and run it.

    cd <game>/prog && jam -j8 -sPlatformSpec=clang -sConfig=dev -sUseDlss=no \
        -sNeedDasAotCompile=yes
    # repeat until it links: generated AOT sources are only picked up on a following pass
    cd <game>/prog && ../../tools/dagor_cdk/linux-x86_64/vromfsPacker-dev prog.vromfs.blk
    cd ../game && ./linux64/<game>-dev gamedata/scenes/test_all.blk \
        -config:circuit:t=am-dev -config:game_das_enable_serialization:b=no \
        -config:sound/enabled:b=no -nosound -quiet

The vromfs repack is mandatory: `game.vromfs.bin` packs `daslib/*.das`, so a stale pack
feeds pre-bump daslib to the new compiler. `-nosound` skips a `dngsound::init()` fatal on
content that has gone stale against git HEAD, which is unrelated to das. Then start the game
twice more without `game_das_enable_serialization:b=no`: the first start writes the AST cache
and the second loads every script from it, the only path that reads the game's own strings
back through the serializer.

**Then read the right channel.** `daScript: init AOT mode` and a non-zero `aot lib size`
only say the artifact loaded. A function whose hash does not match is reported by
`das_log_aot_link_errors`, which writes the blocks to **`das_aot_errors.log` in the log
directory** and summarises through **`logwarn`** - so `logerr`, `[E]` greps and even
`error[50101]` greps of `debug` all come back clean while hundreds of systems interpret:

    D=<game>/game/.game_logs/<latest>
    ls -la $D/das_aot_errors.log          # must not exist
    grep -c "failed to link cpp aot" $D/logwarn $D/debug   # must be 0

`.game_logs/` keeps the previous runs, so an older directory from before the bump is a free
baseline - compare its counts and its `aot lib size` against the new run.

If the counts are non-zero, they are hash mismatches, and the cause is one of:

- **A policy that changes emitted code differs between the AOT driver and the runtime.** Any
  such policy must match `prog/gameLibs/ecs/scripts/das/das_scripts.cpp`, or the artifact
  records hashes the runtime never computes. `cop.no_aliasing` is the one that has bitten:
  the runtime sets it, it gates `Program::patchInline`'s plain-call auto-inline tier, and
  without it in the driver the tool inlines what the game does not.
- **A stale artifact.** `-sForceDasAotRebuild=yes` (`jBuild.jam`: `ALWAYS` on every AOT
  target) regenerates every AOT source with the current compiler.

## 7. Satisfy the CI checks that only exist in CI

Run `cd prog && GDEVTOOL=~/devtools python3 clang_format.py` after committing and amend if it
changes anything. Adding one line to a macro can widen the block and force every continuation
backslash to move, which is a whole-file reformat the `clang_format` lane rejects.

Two traps when checking formatting by hand. The version has to be **18.1.8** (`pip download
clang-format==18.1.8` gives a linux binary; 18.1.3 disagrees on some wraps), and the style must
be **discovered, not forced**: `prog/daNetGame/`, `prog/daNetGame-das-aot/` and
`prog/daNetGameLibs/` each carry their own `.clang-format` that inherits the root and overrides
it (the das-aot one sets `BinPackParameters: false`), so `clang-format -style=file <path>` is
right and `--style=file:.clang-format` silently checks the wrong rules. Only reformat files the
bump actually made non-compliant - compare against master's copy, since several dagor files are
already non-compliant and reformatting them is unrelated churn.

A new upstream lint rule fires on existing game code: LINT022 (unused private symbol) flagged
a dead `empty` overload in a WT script. Fix the code the rule points at rather than silencing
the rule; the AM lane lints locally, the WT lane only reports in CI.

These are not in `codecheck.py`; they run on the build server and fail the whole job.

- **Tab characters** and **trailing whitespace** in added lines. The generated flex output
  (`src/parser/ds_lexer.cpp`, `ds2_lexer.cpp`, `lex.yy.h`, `lex2.yy.h`) is tab-indented
  upstream and de-tabbed here: expand tabs on 4-column stops and strip trailing whitespace
  after copying. Elsewhere, de-tab only the lines the bump adds. Check first that no tab
  sits inside a string literal.
- **The optimize-off pragma.** Upstream adds an MSVC optimize pragma that turns optimization
  off, directly above the existing one that turns size optimization on, in the four parser
  files. The second immediately overrides the first, so drop the off line. In `daslib/aot_cpp.das` these pragmas are *emitted* into
  generated AOT C++ and must stay; keep those lines byte-identical to the previous revision
  so they do not appear as added lines. Note this file avoids spelling the pragma in full,
  because the check greps the diff and would flag the documentation too.
- **Windows `-Werror`**. Unused variables and unused functions pass on linux and fail
  there. Read every adaptation for dead code before pushing.

Scan the finished change for all three before uploading:

    git diff origin/master..HEAD --unified=0 | grep -nE '^\+.*optimize\( *"", *off'
    git diff origin/master..HEAD --unified=0 | grep -nE '^\+.*[ \t]+$'
    git diff origin/master..HEAD --unified=0 | grep -nP '^\+.*\t'

## 8. Commit and upload

- Stage explicitly. `git add -A` over the game trees picks up **stale untracked copies**
  that live in the worktree (`prog/scripts/game/es/` has several) and commits them as new
  files. Before staging a path outside this directory, confirm `git ls-files` knows it.
- One commit for the bump, one for anything built on top. Keep each `Change-Id`.
- Never rewrite the branch while a build is running against the worktree: the build then
  compiles a tree that never existed, and reports errors that are not real.
- A new patchset restarts the whole CI matrix, so batch fixes instead of uploading each one.
