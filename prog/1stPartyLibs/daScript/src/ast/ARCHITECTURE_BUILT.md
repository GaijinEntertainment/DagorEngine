# src/ast architecture notes - built programs

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.

## 6. Built programs (`ast_parse.cpp`, the `ast` module's `make_program`) {#built-programs}

A built program is a program whose modules come from code, not from source text: a host builds
each module out of AST nodes, and the compiler does the rest. It follows `compileDaScript` step
for step, with one change - a block fills each module where the parser would.

- `requireDefaultModules` is the start of `compileDaScript`: `builtin.das` and the access's extra
  modules, parsed into the module group through the same prerequisite walk and the same
  dependency parse (`addDefaultDependencies`, `parseRequiredModules`). It answers null on
  success and the failed program otherwise.
- `buildDaScriptModule` is the parse of one required file. It makes a program, runs the fill,
  then the same inference and finalization a parsed module gets (`inferAndFinalizeModule`, the
  second half of `parseDaScriptEx`), as a dependency with every symbol exported, and moves the
  module into the group (`addNewModules`). The module is named and is a module (`isModule`), as
  a `module <name>` line makes it; the name is an identifier that no module in the group, the
  process or the deferred-module manifest has. It has no file name.
- `requireDefaultModules` and `buildDaScriptModule` take the policies by reference and OR a
  dependency's `options threadlock_context` into
  them, as `compileDaScript` does, so the main module is built with the flag its dependencies
  asked for.
- `buildDaScriptProgram` is the parse of the main file: the fill builds the program's own
  nameless module, which is inferred with the policies' `export_all`, and the program then gets
  the tail `compileDaScript` gives an executable (`finalizeCompiledProgram`, then the
  post-compile macros).

The parser and a fill are two front ends of one pipeline, so a change to inference or
finalization reaches both. Both open a module the same way (`ModuleCompileScope`: the
compilation callbacks, the program, its gc root, the reuse cache and the timers).
`parseDaScriptEx` keeps what only text has: the module-cache read, and the module-cache record a
parse writes. A built module is never cached - it has no file to key a record by - so the `ast`
module's `make_program` hides the environment's serializer for its whole scope, and the default
modules it parses are read and written by no stream either.

**Requires.** A fill says what its module requires with `add_module_require`, which only marks
the module's `requireModule`, and refuses a null module and a module requiring itself. After the
fill, `connectBuiltRequires` does the rest of what the parser's `require` does for each marked
module, in name order: it adds the module to the program's library, runs
`Module::addDependency`, which also reaches the modules the required one re-exports, and records
the require in `allRequireDecl`. The implicit `$` is skipped, as the parser never writes it. A
module is reachable when it is a C++ or shared module of the process (`get_module("math")`;
`get_module` does not load a deferred dynamic module) or a module built earlier in the same
`make_program`. A module from another group, and two modules that require each other, fail the
build with a compile error.

**Gc roots.** A module owns its nodes as a parsed module does. The fill runs with the module's
own `module_gc_root` as the active root (`gc_active_scope`), so every node the fill allocates
lands on that root, and at the end of the module `ModuleGcFinalize` collects what the module
reaches into a fresh root and drops the old one, which frees everything the fill built but did
not attach. Every node a module uses is built inside that module's fill. A function, generic,
global, structure or enumeration the module holds that sits on another root fails the build
(`verifyBuiltOwnership`); a node a fill did not attach is freed when its module ends, so a later
fill that uses it reads freed memory. `buildDaScriptProgram` adds the outer `gc_guard` and
`GcCollectOnExit` that `compileDaScript` has, for what the executable tail allocates.

**The environment.** A build binds the program it compiles to the environment
(`g_Program`, the compiler log, the compiling names), as a parse does. `buildDaScriptModuleEx`
and `requireDefaultModules` save and restore the outer binding (`CompileEnvScope`), so a build
started inside another module's fill, or from a macro, leaves the outer compile bound.

**The das entry.** `make_program(cop) { ... }` owns the scope: a file-system access, the module
group, the policies, the first failure, and the programs it returned. The scope is a
thread-local stack, so a `make_program` nested inside another starts clean and restores the
outer one on exit. `make_module(name) $(mod) { ... }` answers the finished module, or null when
it failed; `make_main_module() $(mod) { ... }` answers the program, which carries its own errors
(`Program.failed`, `Program.errors`). Once a module built by `make_module` fails, every later
`make_module` answers null and `make_main_module` answers that failed program, neither running
its block, so the host checks errors in one place; a failed main module ends nothing. Text the
compiler logs during a build (warnings, the `log_*` options) goes to the host's log at the
`warning` level when the build ends. A panic in a fill stops that module with an error and
leaves the build as the panic, after every compiler scope has closed. The modules live in the
scope's module group and are freed when the block returns, so a program is simulated and run
inside the `make_program` that built it; a program that is still referenced when the block
returns makes `make_program` panic.
