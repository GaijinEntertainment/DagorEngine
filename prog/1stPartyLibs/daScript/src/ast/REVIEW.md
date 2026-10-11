# AST Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture docs:
`ARCHITECTURE.md`, `ARCHITECTURE_INFER.md`. A diff touching `promoteInitToAssign`
(`ast_infer_type_function.cpp`) or `isPromotedInitCall` (`ast_generate.cpp`) applies
`daslib/REVIEW.md` (repo root) too -
the `daslib/builtin.das` (repo root) helpers they name are the other side of a recorded pair
(`daslib/ARCHITECTURE_SYNTAX.md`, repo root).

- **Weakening `REVIEW.das` (beside this file) is a defect:** dropping its scan of the prints in
  `trySerializeProgramModule` (`ast_parse.cpp`, `ARCHITECTURE.md` sec.1) or of the module-cache
  read diagnostics elsewhere in that file, or a finding text that no longer names what failed.
  What the gate enforces is read from the gate itself.

- **A diff that adds a module-cache read diagnostic outside `trySerializeProgramModule`
  (`ast_parse.cpp`), or moves one out of it - into a helper it calls, or another function in this
  folder - extends `REVIEW.das`'s scan to the new home in the same change.** A module-cache read
  diagnostic is a line reporting one record read of the module cache, the kind
  `trySerializeProgramModule` prints; the module scan's `[module]` trace is not one. A print the
  gate does not scan is a print nobody checks.

- **A `string` local that `trySerializeProgramModule` (`ast_parse.cpp`) reads from the
  module-cache record header - the bytes read before the record payload - goes through the
  serializer's `serializeTemp`, never plain `operator<<`.** The serializer's string table keeps a
  pointer to the string it read into, so a header local read plainly is a dead pointer at the
  next mention.

- **A diff that adds a `[module]` line to the module scan's trace (`dyn_modules.cpp`) prints it
  only behind `trace_scan()`, the switch `DAS_TRACE_MODULE_LOAD` sets.** The scan runs on every
  start, so a line outside the switch is output every user sees.

- **A diff that changes what a manifest written by an earlier binary replays to - what
  `read_manifest` and `write_manifest` (`dyn_modules.cpp`) carry, in what order, in what encoding,
  a key line the manifest did not carry before, or a change to what an existing row registers -
  bumps the version in `MANIFEST_HEADER` in the same change.** A reader accepts a manifest whose
  first line equals `MANIFEST_HEADER`, so without the bump an older manifest decodes the changed
  bytes as a wrong registration with no diagnostic.

- **A diff that gives `read_manifest` a new kind of row - one it pushes into `rows` - gives the
  replay loop in `init_dyn_modules` (`dyn_modules.cpp`) a branch for it in the same change.** The
  loop dispatches on `row.dynamic` and then on whether the row carries a das-visible name, so a
  kind it does not know replays as a native path or waits under a name nothing requires.

- **A diff that gives `require` a new spelling - a guard form, a group form, a path prefix, the
  whitespace a form allows - or changes when a guarded require or a guarded group member is
  skipped, teaches every site that decides it the same spelling and the same skip-or-take
  decision, in the same change: the text collector `getAllRequireReq` (`ast_parse.cpp`), the
  parser's `ast_requireGuardAvailable` (`src/parser/parser_impl.cpp`), and the member walk
  `moduleGroupMemberAvailable` (`src/builtin/module_builtin_rtti.cpp`).** The prerequisite walk collects
  requires from the source text before any parse, so a spelling only the parser reads is a
  module the walk never compiles, a decision the two make differently is a require the parse
  takes with no module behind it, and a member the calls take but the require did not is a call
  into a module that is not there.

- **A diff that makes a compile entry point in `ast_parse.cpp` rebind a `daScriptEnvironment`
  field has that entry point hold a `CompileEnvScope` for the whole compile and puts that field in
  it, in the same change. A compile entry point is any
  function there that daslang code reaches through a builtin that compiles, builds or requires a
  module (`src/builtin/module_builtin_ast.cpp`, repo root).** A macro can start such a compile
  inside another module's parse or build, and a field restored by a plain statement after the
  compile is not restored when an exception unwinds through it.

- **A diff that changes the bytes the module-cache record header `writebackModules` writes and
  `trySerializeProgramModule` reads (`ast_parse.cpp`) - their content, order or encoding - bumps
  `AstSerializer::getVersion()` (`include/daScript/ast/ast_serializer.h`, repo root) in the same
  change.** A reader accepts a stream whose stored version equals `getVersion()`, so without the
  bump an older cache decodes the changed header as the old one with no diagnostic.

- **The `setDeferredModuleLoader` call in `require_dynamic_modules` (`dyn_modules.cpp`) stays,
  and stays above the first `init_modules_for_folder` call - removing it or moving it below is a
  defect.** That call compiles the descriptors, and a descriptor can require a module an earlier
  replay deferred, which fails with no loader installed.

- **A diff that changes how `Function::getMangledName` (`ast.cpp`) forms a name, or which module
  `Module::addFunction` (`ast_module.cpp`) files a builtin function under, bumps
  `LLVM_JIT_CODEGEN_VERSION` in `modules/dasLLVM/daslib/llvm_jit_plan.das` (repo root), in the
  same change.** The JIT's DLL cache key folds the codegen version and each function's AST hash,
  never the name an extern binds under, so a cached DLL binds the old name and crashes on the hit.

- **A diff that builds an `ExprCopy`, `ExprMove` or `ExprClone` standing in for no `=`, `<-`
  or `:=` the user wrote - an inliner result store, a `let` relocated into an assignment, a
  loop counter or control flag, a yield, finally-return or capture store - passes `true` as the
  constructor's `no_promo` argument, or sets `no_promotion` from the node it rewrites
  (`ARCHITECTURE_INFER.md` sec.5).** A store that performs a user-written `:=` one field at a
  time, as a generated clone does, is the user's store and passes no flag. A compiler-made
  store that reaches the user's `operator =` runs it at a site the user never wrote, and only
  in the build configuration that builds the store.
