# Builtin Modules Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`ARCHITECTURE.md`.

- **Weakening `review_nttp.das`'s bind-flavor scan, which `REVIEW.das` runs, is a defect - fix a
  bind the scan reports by switching the bind.** The Inline modules are `$` (builtin), `math`,
  `strings` and `jit`. What the scan enforces, and which shared generic helpers it exempts, is
  read from the scan itself.

- **A diff that adds or changes a bind - any `addExtern*` call - under this folder rebuilds
  the binary from that diff before the folder's gate runs** - the scan reads the binds
  compiled into the running binary, so a stale binary is a false green.

- **A diff that adds a module under this folder adds it to `review_nttp.das`'s `require` list in
  the same change - directly, or through the daslib wrapper that requires it.** A module the list
  does not reach is a module the scan never sees.

- **Never drop a module from `review_nttp.das`'s `require` list.** The list is what sets the
  modules the scan covers.

- **A diff that changes the bytes a module-cache record carries or what they resolve to - a
  field added, removed, reordered or re-typed in `module_builtin_ast_serialize.cpp`, an encoding
  changed there, or a change to which module owns a streamed annotation, function or type -
  bumps the version `getVersion()` returns in `include/daScript/ast/ast_serializer.h`, in the
  same change** - a reader accepts a stream only when its stored version equals `getVersion()`,
  so without the bump an older cache passes that check and decodes the changed bytes as
  something else. A C++ layout change to a handled type or an AST class is not a record byte.

- **A diff that makes a record written before it wrong - the bytes still decode, but what they
  encode is no longer what this build would write - bumps the version `getVersion()` returns in
  `include/daScript/ast/ast_serializer.h`, in the same change** - a record reached through an
  explicit `-module-cache <path>` carries no binary stamp in its key, so the version is the only
  thing that discards it.

- **A version bump REPLACES its comment, never appends to it** - `getVersion()` in
  `include/daScript/ast/ast_serializer.h` states what THIS version changed and nothing about the
  ones before it. A diff that keeps the old note in a parenthesis is a defect; git carries the
  history.

- **A diff that streams or compares a `CodeOfPolicies` field in `module_builtin_ast_serialize.cpp`
  outside `DAS_MODULE_CACHE_POLICY_FIELDS` is a defect - put the field on the list instead** - the
  list drives both the record's policy stream and the compare that refuses a record written under
  other policies, so a field handled outside it is written without being compared, or compared
  without being written.

- **A diff that adds to `AstSerializer::serializeProgram`, `AstSerializer::serializeProgramImpl`
  or `ModuleFileCache` (`module_builtin_ast_serialize.cpp`) a diagnostic the default build
  compiles, without the serializer's `quietCache` gate - or drops that gate from one already
  there - is a defect** - the module cache is on by default, so an ungated line prints on an
  ordinary run.

- **A diff that adds a builtin a `.das_module` descriptor can call whose effect outlives the
  descriptor's own program - a row in a process-wide registration table a warm start must
  reproduce without running the descriptor - records the call between
  `begin_dynamic_module_recording` and `end_dynamic_module_recording` (`module_builtin_fio.cpp`)
  as a row `read_manifest` reads back and `init_dyn_modules` replays (`src/ast/dyn_modules.cpp`),
  in the same change.** A replayed start never runs the descriptor, so an effect the recorder
  does not see is an effect every warm start silently lacks.

- **A diff that moves a bind between modules - any `addExtern*` call whose module changes, or a
  builtin whose `vector<T>` functions follow a type to another module - bumps
  `LLVM_JIT_CODEGEN_VERSION` in `modules/dasLLVM/daslib/llvm_jit_plan.das` (repo root), in the
  same change.** The JIT's DLL cache key folds the codegen version and each function's AST
  hash, never the module an extern lives in, so a cached DLL binds the old name and crashes on
  the hit.

- **A diff that changes how an `[extern]` call reaches its bind - the wrapper tables, the
  `systemV_extra` list, the arm64 layout, the `das_arm64_call` trampoline, or when and from
  what the `__dasbind__` proxy is registered and the call retargeted - updates
  `ARCHITECTURE.md#interpreter-extern-call` in the same change.** Comments in `module_builtin_dasbind.cpp` cite that
  section instead of restating it, so a stale section is what the next reader trusts.

- **A diff that changes what `ModuleFileCache::defaultPath` or `ModuleFileCache::embeddedHostOptions`
  folds into the module-cache key - the binary, the command line, the environment names, or
  which script arguments count - updates `ARCHITECTURE.md#default-module-cache-path` in the
  same change.** The key is what stops a native-compiled module serving a cross compile, so a
  wrong description of it gets trusted.

- **A diff that adds a `std::filesystem` call in `module_builtin_fio.cpp` passes every path into
  it through `das_to_path` and every path out of it through `path_to_das`.** On Windows a path
  built from a narrow string decodes through the ANSI codepage, so a UTF-8 name the codepage
  cannot represent is misread on the way in and throws on the way out.

- **A diff that changes the function or block a citing comment annotates - a comment naming a
  section of an architecture document, this folder's (`// src/builtin/ARCHITECTURE.md#<anchor>`)
  or another's (`// src/ast/ARCHITECTURE.md#<anchor>`) - updates that section in the same
  change.** C++ carries no `[arch]` annotation; LINT026 checks that the pointer resolves, and
  nothing but this rule keeps the cited section true.

- **In a C++ type das binds through an annotation with `addField` in this folder, a member
  whose size differs between the standard libraries the repo's targets use
  (`std::mutex`, `std::function`, `condition_variable`; not `std::string`) or a platform struct
  embedded by value (`struct stat`) is declared after the last das-visible field.** A
  cross-compiled exe bakes the host's field offsets into the code it generates, and the
  target's standard library sizes such a member differently, so every das-visible field behind
  one is read at the wrong address.

- **A type das holds by value (`isLocal`, `canCopy` or `canMove` true in its annotation) is
  built from members whose size is the same under every target's standard library -
  fixed-width scalars and `std::string`: copy what das needs out of a platform struct instead of
  embedding one.** The exe bakes the host's `sizeof` for such a type, so a target that sizes an
  embedded member differently gives every das local of it the wrong length.

- **A diff that changes the data-member order or data-member set of a C++ type das binds
  through an annotation states its `--jit-check-abi` result for a cross target in its own PR
  description.** The check reports a mismatch at the bundle's first launch, for the types the
  bundle links; nothing native can observe one.

- **A `string` streamed in `module_builtin_ast_serialize.cpp` whose storage does not outlive the
  module-cache record being written or read - a computed mangled name, a lookup key, a container
  element, a field of a local - goes through `serializeTemp`, never plain `operator<<`.** The
  string table holds pointers into the caller's own bytes, so an entry left pointing at a dead
  local is what every later mention of that string writes or reads.

- **A diff that adds a per-record table or vector the serializer numbers into
  (`module_builtin_ast_serialize.cpp`) clears it in `AstSerializer::clearNodeIds`, in the same
  change.** A later compile reuses a freed address, so a table that outlives its record answers a
  new node with the old node's number or name.

- **A diff that reads a writing `SerializationStorageVector`'s `buffer` outside
  `AstSerializer::serializeProgram` (`module_builtin_ast_serialize.cpp`) - to hand the bytes out,
  hash them, or write them - calls `flush()` first.** The writer grows the vector by doubling and
  counts the bytes in `writePos`, so before a flush the vector is longer than the stream.
