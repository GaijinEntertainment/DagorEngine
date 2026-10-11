# daslib architecture notes - emission: AOT C++, standalone contexts

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.

## 5. aot_cpp

- **`addr()` of a block local erases its type**: `preVisitExprRef2Ptr` writes
  `das_ref(__context__, <expr>)` and the address goes on as `void *`, and a block literal is
  a local of a `das_make_block<...>` type (`emitBlockLet`), whose `Block` base is not at
  offset 0 - `das_make_block_base : Block, SimNode_ClosureBlock` puts the polymorphic base
  first. So `das_ref` in `include/daScript/simulate/aot.h` returns the `Block` subobject for a
  `Block`-derived type; the C++ data walker reads that address as a `Block`. The pair moves
  together; nothing but the full `test_aot` lane fails on a mismatch.
- **A `try`/`recover` restores what the interpreter's handler restores**: the emitter writes
  the pair as `das_try_recover(__context__, [&](){...}, [&](){...})`, whose catch path puts
  back `fastCallDepth` before the recover body runs, then `abiArg`, `abiCMRES` and the stack
  watermark after it - the same state `SimNode_TryCatch` and `jit_try_recover` restore
  (`include/daScript/simulate/ARCHITECTURE.md#fastcall-depth-guard`). The three move together;
  only `test_aot` over a capped program whose recursion stays interpreted fails on a mismatch.
- **C++ identifier mangling**: `aotSuffixNameEx` prepends `_S`/`_E`/`_V`/`_f_` when a das
  name is a C++ keyword, holds a non-alnum char, or is `DELETE` (winnt.h). Structs and
  enums share ONE C++ namespace while daslang keeps separate tables, so `struct X` +
  `enum X` in one module is legal das and a C++ redefinition - `buildStructEnumCollisions`
  finds the pairs per module and forces both suffixes.
- **The emitter targets the MSVC/GCC/Clang intersection; the redundant-looking spellings
  are the portable ones** - `INT64_C`, the explicit `0x8000000000000000` enum arm,
  `to_cpp_double`'s named non-finite tokens (mirrors `src/simulate/runtime_string.cpp` and
  moves with it), elaborated `struct X`, double parens around `das_iterator` sources
  (most vexing parse), non-const value loop variables (`first()`/`next()` write into it).
- **Sequenced binary operands**: any op2 whose operands are not both side-effect-free is
  emitted through `das_ordered2{ L, R }` plus an immediately-invoked lambda. The struct and
  its member names live in `include/daScript/simulate/aot.h` and move with the emitter's
  `__lr.left`/`__lr.right` spellings.
- **Stack-frame `new`/ascend**: per-block storage declared once, the USE site
  re-initializes per evaluation (memset for `new`, whole-value overwrite for ascend) -
  dropping the reinit reuses the previous iteration's value.
- **fp16 and the 8/16-bit lattice go through the vec4f policy ABI**: `SimPolicy_HalfVec` is
  vec4f-shaped even at width 1, so scalar fp16 always casts across the policy boundary,
  and a lattice-vector policy result must come back typed for `das_equ_val` deduction.
  Lattice swizzles use element-typed helpers - generic `das_swizzle` caps at 4 fields,
  `v_extract` is 32-bit-lane only.
- **A struct needs a COMPLETE C++ type more often than field access suggests** - globals
  need `sizeof` even untouched, and by-value field types emit depth-first before their
  owner, or the type degrades to a forward declaration.
- **`aotModuleName` has a C++ twin that moves in lockstep** -
  `src/ast/ast_typedecl.cpp`'s `aotModuleName`/`describeCppTypeEx` spell the same type
  names into the same TU through `DebugInfoHelper`'s cached cpp names
  (`debug_helper_find_*_cppname`, the cross-platform `TypeSize<...>` sinks). The
  main-module namespace override is therefore one knob with two ends:
  `set_aot_main_module_name` writes the daslib global AND forwards to
  `set_aot_main_module_name_cpp`; a spelling rule changed on one side only produces a TU
  where the definition and its debug-info references disagree.
- **`fromExtraDependency` describes the host process, not the program being emitted.** The
  flag is set when the host first loaded a shared module as an extra dependency (a `-jit`
  host loads `strings`, `fio_core` and `math` that way) and stays on the shared module for
  every later compile in the process. `getRequiredModulesFor` skips such modules only on the
  regular AOT path; a standalone context decides by its linked set alone, otherwise a
  generator running under a `-jit` host (the JIT test lane) prunes the modules the program
  calls.
- **`DEFAULT_MODULE_ORDER` mirrors `register_builtin_modules_impl`** in
  `src/builtin/modules.cpp` - the same builtin C++ modules in the same order, a
  daslib-to-C++ pair `tests/aot/test_standalone_emit.das` reads back from the C++ source and
  pins. A standalone context registers these first because
  module constructors `Module::require` earlier ones by name (`fio_core` takes `strings`,
  dasHV takes `rtti_core`). A module missing from the daslib list still registers, only in
  the dependencies-first pass that follows; a module added to the C++ side joins the list.
- **The AnnotationInfo table resets at the START of the debug-info dump, not its end.** The
  globals' `VarInfo`s are written after that dump and a handled global's info refers to an
  `AnnotationInfo` by the name the dump minted, so clearing on the way out would leave `&` with
  nothing after it. Only that walk reaches a global's annotation - `writeHandledAnnotations` iterates
  types, structs and functions.
- **A member pointer is qualified with `aotModuleName`, never the raw module name.** The main
  module is unnamed, so `_module.name` is empty for every type a script declares itself, while
  `describeCppType` resolves those types through `g_aot_main_module_name`. Where one emitter writes
  both - `das_safe_navigation<T, F, &T::field>` - they must agree, or the type argument names the
  context's namespace while the member pointer names nothing. Only a standalone context sets a
  main-module name, so regular AOT never sees it.

## 6. aot_standalone

- **The entry module's structures are visited SORTED.** `visitModule` takes `sortStructures`,
  which runs `topoSortStructures` so a by-value field's structure is complete before the structure
  holding it; regular AOT passes it through `visit(program, adapter, true)` and a standalone
  context, which visits the entry module by itself, has to ask for it too. Declaration order is the
  author's, and nothing else re-derives it.
- **The context name is an identifier; the file stem is not the same string.** A stem reaches C++
  as a namespace and C as a symbol prefix, so `while.das` or `3d-math.das` would open
  `namespace while {`. `context_name` is the stem through `cpp_context_ident`, while `file_stem`
  keeps the raw name - a build system predicts the generated file names from the input path and
  cannot be told they were sanitized.
- **The generated constructor IS the init protocol** - a standalone context never calls
  `Context::runInitScript`, so the ctor reproduces its observable semantics inline:
  `memset(context.globals, 0, context.getGlobalSize())` mirrors runInitScript's globals
  memset (`src/runtime/context.cpp`), then the same-TU `__init_script(&context, true)`,
  then each `[init]` function directly. `aot_cpp.das`'s `preVisitGlobalLet` writes the
  `__init_script` signature - a daslib-to-daslib pairing nothing checks.
- **`__init_shared` is hardcoded `true`** - a fresh standalone context always owns its
  shared globals.
- **The ctor's base `Context(N)` folds the init headroom in, because there is no separate
  init stack** - init locals are C++ locals in AOT. A script that sets `options stack` to a
  positive value gets `N = options stack + globalInitStackSize`, honored exactly, the
  interpreter's floor included: the floor protects INTERPRETED init, while standalone init
  is AOT, so an embedded target's stack budget wins. Zero and negative fall through to
  `N = max(policies.stack, 16384) + globalInitStackSize`; `Context(0)` owns no stack, so any
  prologue would assert. The fall-through arm mirrors the interpreter's init-stack formula
  in `src/ast/ast_simulate.cpp` - a pair, deliberately divergent on the explicit-option arm.
  `globalInitStackSize` reaches the emitter through the rtti `Program` binding in
  `src/builtin/module_builtin_rtti.cpp`, so an `invoke` during init still has das stack for
  its prologue.
- **The ctor wires what the collector walks, the way `Program::simulate` does** - two pairs
  with `src/ast/ast_simulate.cpp`: `context.gcEnabled = true` is emitted for a program with
  `options gc` (simulate reads the same option into the same field), and every used global gets
  a `VarInfo __gvar_info_<index>` through the emitter's debug helper with
  `globalVariables[<index>].debugInfo` pointing at it (simulate's `makeVariableDebugInfo`);
  the collector's global walk in `src/simulate/simulate_gc.cpp` dereferences that info
  unguarded, so a context that collects with a null one crashes. The flag line is gated on
  the option because the nano context (`nano/include/daScript/simulate/simulate.h`) has no
  `gcEnabled`; it does carry `GlobalVariable::debugInfo`, so the infos are emitted for every
  standalone program. `tests/aot/test_standalone_emit.das` pins the emitted text; the
  watchdog (`utils/watchdog/main.das`, `options gc`, globals) is the build that compiles and
  runs it, collecting on its first tick.
- **There is no `!stopFlags` guard between `[init]` calls** - a panic propagates out of the
  ctor instead of soft-stopping the sequence.
- **Global init order is fixed by the program's variable index table**: `StandaloneContextGen`'s
  `preVisitGlobalLet` emits required modules' globals via ordered `for_each_module`
  before the adapter walks the entry module's own - correct only because
  `ast_allocate_stack.cpp` fills `Program::variableIndices` (read back as
  `variable_index(program, var)`) through the same dependency-first module order with the
  entry module last, and `runInitScript` executes ascending index. `[init]` function order is not re-derived at all: the emitter reads the
  simulated context's list through rtti `for_each_init_function`, so the C++ late-init
  sort stays the single source of truth.
- **The TU holds every used function of every module.** A standalone context is one
  translation unit with no other to link, so `prepareProgramForEmission`'s markers, the
  block-variable collector, `registerAotCpp`'s `ArgsConverter` thunks and
  `StandaloneContextGen` each walk the entry module through `visit_module` and then every
  foreign used function through `visit(fn, adapter)` (`foreignUsedFunctions`, aot_cpp): a
  builtin is C++ already, a template never emits, a used `[no_aot]` function was refused by
  name before the walk. A generic instance several modules instantiated has one AOT name,
  so declarations, bodies, thunks and table rows are emitted once per name. A foreign
  global's initializer temporaries collect under the collector's null key, the one
  `__init_script` declares. `UseTypeMarker` walks the foreign functions too, so an extern
  or handled type reached only from one still links its module.
- **A call into a required module is a direct C++ call.** `genStandaloneSrc` builds
  `StandaloneContextGen` with the `foreignUsedFunctions` list it then walks to emit the foreign
  bodies; the generator keeps their mangled-name hashes in `emittedForeignHashes` and answers
  `CppAot.emitsForeignBody` from them, so `isHybridCall` spells the call in the same-module
  direct form. One list feeds the table and the emission loop, so a hash in the table is a
  body in the unit; the prototype the call binds against comes from the declaration list,
  `collectProgramUsedFunctions(prog, true, false)` in `preVisitProgramBody`, a superset of that
  list deduplicated by the same `aotFuncName`. The regular `-aot` and `-lib` units answer false and keep the
  `fnByMangledName` lookup: the callee's body is in another unit. `[hybrid]` and `[jit]`
  functions keep the lookup in every unit because `isHybridCall` tests those flags before
  asking, and every Func slot (`@@fn`, class methods, finalizers) keeps it because
  `preVisitExprAddr` never asks. The dependency dump's second `CppAot` emits no expression
  and needs no table.
- **Every used `[init]` is called from the ctor, whatever module declares it.** The TU holds
  every used function of every module (above), so a required module's `[init]` has an AOT body
  like any other and needs no special case; the call order is the simulated context's own, read
  back through rtti. A `[no_aot]` one stays a collected emit error, because there is no body to
  call. An engine that registers itself from its modules - dasLLAMA's architecture registry is
  sixty `[init]` functions spread over its arch files - has no standalone form under any
  narrower rule.
- **Every used function must have an AOT body** - a standalone context has no
  interpreter, so a used `noAot` function (the `[no_aot]` annotation, or `NoAotMarker`
  finding a type AOT cannot express) is a collected emit error, never a
  `fnByMangledName` call that would crash at runtime. `prepareProgramForEmission` runs
  `NoAotMarker` first (the regular AOT paths run it too; standalone must match) and
  then `checkAllUsedFunctionsCanAot` walks used, non-builtin functions.
- **The function table is dense** - `addFunctionInfo` numbers the emitted rows in emission
  order and the ctor sizes `context.functions` by that count, not by the program's
  `totalFunctions`. The program count includes every function the interpreter simulated
  (externs among them), while the context emits only what it still reaches; a table sized
  by the larger count leaves rows of uninitialized memory that the destructor's shutdown
  walk (`runShutdownScript`) reads. Nothing reads a function by program index: AOT calls
  are direct, function pointers resolve through `fnByMangledName`, and `FillFunction`
  matches rows by AOT hash.
- **A context links the modules the program reaches, and registers the C++ ones itself.**
  `aot_cpp.das`'s `collectLinkedModules` marks the module of every used function, of every
  function a used function or global calls (`UseTypeMarker` reads the callee of each call,
  operator and `@@` address - externs carry no `used` flag of their own), and of every
  struct, enum and handled type; a program that reaches no C++ module beyond the builtin one
  links exactly that set. A nano program reaches none by contract (every builtin module is
  absent there), so its output stays registry-free.
  Once it reaches one, every linked C++ module's `module_for_each_dependency` closure joins
  the set, because module constructors `Module::require` those by name (dasHV takes
  `rtti_core` this way). A default C++ module the program never reaches stays out, however
  the compiler loaded it: a macro module's `daslib/ast` brings `rtti_core` and `ast_core`
  into the compiler, and a context that registered them would run two constructors and carry
  their code for nothing. The pruned modules (`compile time only, not linked`) get no
  `aotRequire` include and no registration.
  `standaloneModuleRegistration` orders the C++ subset and ranks it: `DEFAULT_MODULE_ORDER`
  first (the pair it mirrors is recorded in sec. 5), then the rest dependencies-first, ranked
  by dependency depth. Each generated TU emits its list as a `StandaloneModule` table added to
  the process-wide list before main, and the context class takes `StandaloneModuleScope`
  (`include/daScript/simulate/standalone_modules.h`, inline so every TU shares one copy) as
  its FIRST base, ahead of `Context`: constructed before it, destroyed after it. The first
  scope constructed registers the union, rank order, and initializes once; the last scope
  destroyed shuts down - inside main, after the last context's own teardown and while the
  runtime's statics still stand. A static destructor cannot do this: it runs after main,
  when statics constructed later than it (libhv's, the runtime's own) are already gone, and
  on Windows that shutdown corrupts the heap - the exit-time race the `-exe` path avoids by
  shutting down before main returns. Two contexts with different module sets in one binary
  therefore share one lifetime. A host that registered the builtin module owns the registry
  instead: the context registers nothing and stops the program, by name, on a linked module
  the host did not register - the runtime cannot add to an initialized registry and keep
  `Initialize`/`Shutdown` balanced, and a second shutdown would walk a registry the first
  one deleted. The shutdown is `Module::ShutdownStandalone`, never `Module::Shutdown`: the
  latter resets the fusion engine through a function pointer only the interpreter's
  `simulate_fusion.cpp` installs, and a standalone binary links no interpreter. Handled
  types need the registry because `TypeInfo::resolveAnnotation` walks the bound
  environment's module list; a context with none dereferences a null environment.
- **Type definitions live in the header, once** - struct/enum definitions (the
  dependency dump plus the entry module's own `declarations` capture) are emitted into
  the `.das.h`, which the `.das.cpp` includes; the source never redefines them. They sit
  in module-named namespaces directly under `das`, NOT under the per-program
  `_anon_<hash>` namespace - that hash changes with the program, so it can never be a
  public name. The entry module is promoted with an empty name, which would put its
  types in an anonymous namespace (a distinct type per TU - an ODR/link break for any
  struct in an exported signature); `aot_cpp`'s `set_aot_main_module_name` names it
  after the context for the duration of standalone emission, so class signatures, the
  source TU, and the embedder all resolve the same `das::<ctx>::T`. The header carries
  `#pragma once` and the required modules' `aotRequire` includes so it stands alone in
  an embedder TU; two DIFFERENT contexts' headers sharing a das dependency still cannot
  be included in one TU (the shared types have no per-type guards).
