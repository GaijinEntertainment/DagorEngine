# daslib architecture notes

Design rationale not evident in code; numbered module sections are anchored to symbols.

Companion documents carry separate concerns.
- `ARCHITECTURE_LINT.md` - sec. 1-4: perf_lint, lint_config, lint, style_lint.
- `ARCHITECTURE_EMIT.md` - sec. 5-6: aot_cpp, aot_standalone; `ARCHITECTURE_SHADER.md` - sec. 7, 28-29: flatten, the shader rails.
- `ARCHITECTURE_CAPI.md` - sec. 30: c_api_header, the C surface both backends emit.
- `ARCHITECTURE_LINQ.md` - sec. 11-17, 33, 37: the linq family, sql_linq, sql_migrate.
- `ARCHITECTURE_CURSOR.md` - sec. 40: ast_cursor, the cursor module the LSP and MCP navigation tools share.
- `ARCHITECTURE_SYNTAX.md` - sec. 42: the raw assignment spellings, the formatter / printer / builtin sides of the lexer, parser and inference pairs.
- `ARCHITECTURE_MEMORY.md` - sec. 43: runtime memory snapshots.

## 8. ast_verify

- **One node set answers two opposite questions**: a node reached TWICE has two parents
  (broken unique ownership); a gc-owned node reached ZERO times sits in a slot no visitor
  enters, so every visitor-driven pass edits a tree it cannot see in full. Slots C++
  `::visit` skips are walked by hand or recorded as whole trees.
- **Reporting is repair**: diagnostics go through `macro_sticky_error` (infer clears plain
  errors from a repaired tree); null entries in per-entry-dereferenced lists compact in
  preVisit; a self-reachable node is CUT, not reported - a cycle kills every later walk.
  Both passes use the sticky form, post-infer included. `Program::stickyError` records the
  error AND calls `Program::error`, so it is a strict superset of `macro_error` - the plain
  form buys nothing. A loss is unreachable while every re-entry into
  `inferTypes`/`inferTypesDirty` after a post-infer firing stays guarded by
  `!program->failed()`, but repair is what makes the guarantee necessary rather than
  redundant: the guard that found the damage cannot fire twice, so an error cleared once is
  gone for good.
- **`visitExpression` is the only place an ancestor-path entry is dropped.** The C++
  adapter's `VISIT_EXPR` macro (`aot_builtin_ast.h`) calls `visitExpression(that)` before
  every `visitExprXXX`, so a derived visit that erased again would erase nothing - a
  derived visit returns its node and leaves the path alone.
- **The two passes' skip sets are complementary**: pre-infer skips `generated` (filled in
  across passes); post-infer skips only `[template]` bodies and dasbind `[extern]` stubs -
  so generated bodies ARE checked post-infer, by nothing else.
- **A structure's field-default init tree is a declaration, not code**: it never simulates -
  the generated ctor carries its own inferred clone, which stays fully checked. Infer types
  the decl init in place but binds its calls only when construction generates the ctor, so a
  struct nothing constructs legitimately keeps a typed-but-unbound cross-module ctor call
  there. The post-infer func invariant therefore tolerates calls inside field inits
  (`in_field_init`, set/cleared around the field walk); every other invariant (types, ats)
  still applies to them. The same-module shape resolves eagerly and never had the hole -
  the selftest fixture is cross-module for exactly that reason.
- **Each check's licensing C++ site**: `ExprOp1.subexpr` -
  `SimulateVisitor::visit(ExprOp1*)` dereferences;
  `TypeDecl.dim` entries - `TypeDecl::dimConst` sentinel (`ast_typedecl.h`); `ExprFor`
  sources/body - `ExprFor::visit` walks sources and body, never the iterator tags;
  `ExprBlock.arguments` - `ExprBlock::visit` walks `arguments` only when `isClosure`;
  `ExprBlock.returnType` - `ExprBlock::visit` walks it only for a closure.

## 9. quote

- **A lowered quote is a generated leaf function, not an inline expression** - one frame
  per quote instead of inflating every caller's; the result type clones the quote's static
  type because `autoinfer` would leak the concrete node type and break `Expression?`
  identity consumers.
- **`blacklist` entries naming back-references break reconstruction cycles - removing one
  hangs the walk** (`Function.classParent`, `ExprReturn._block`, `ExprVar.pBlock`,
  `EnumEntry.value`); the rest are post-infer bookkeeping absent from quoted trees.

## 10. templates_boost

- **A `Template`'s substitution tables are freed once, by whichever mechanism the local
  declares** - `var inscope rules` finalizes at scope exit; a plain `var rules` does not and
  needs an explicit `delete rules`. Declaring both double-finalizes, and the double is
  silent: inscope-only, inscope+delete and plain+delete all leak nothing, so a heap count
  cannot tell a double-finalize from a correct form; only a plain local with no delete
  leaks. Both `apply_qmacro_template_*` appliers are inscope-only; the three plain-local
  `apply_template` overloads pair the declaration with the delete.
- **`apply_template` returns a possibly-NEW root.** `apply_template(rules, at, expr,
  forceAt)` takes `expr` by value, and the block-form overloads take `Expression?&` only to
  forward it - none writes the new root back. A substitution that replaces the ROOT node -
  an identity key body, `_order_by(_)` - is visible only through the return value; non-root
  replacements leave the pointer unchanged.
- **`stamp_missing_at` fills only MISSING locations**, unlike the force-at of `$e()`;
  `carry_tag_safe_flags` copies the parser's unsafe-wrap flags across `$c` substitution or
  the safety is lost where the result lands.

## 18. dupe_detect

- **A run is unextractable when its meaning depends on the scope around it** - the gate
  counts `unsafe`-authorized operations, `assume` aliases, `defer` (lowered away), and
  `break`/`continue` targeting a loop the run does not own.
- **The index is a preorder node array over the LIVE AST** - records keep node ids and
  read `.at` off the node; `ExprConst*` re-enters the base pre-visit hook and the second
  entry pushes a `-1` marker - lose it and every preorder interval is wrong.

## 19. only_nttp

- **C++ mirror pair**: `only_nttp`'s `moreFlags2.nttp` read <-> `ExternalFnInline`'s
  `this->nttp = true` (`include/daScript/ast/ast_interop.h`), plus the positional
  `MoreFunctionFlags2` argNames list (`src/builtin/module_builtin_ast_flags.cpp`) that
  gives the bit its daslang name. Nothing fails when one side moves alone.

## 20. interfaces

- **The implements-marker IS the generated getter field** - `is`/`as`/`?as` key purely on
  its presence; parent interfaces get their own deduped getter fields.
- **The const getter's `unsafe(addr<$t(st)? -const>(self))` is the one sanctioned
  const-strip write**, and the dead-store eliminator cannot remove it on two counts, not on
  the `unsafe`: the constness sits on the PARAMETER BINDING while the object behind it is an
  ordinary mutable allocation, and the store is re-read through the same pointer two lines
  later while the new proxy escapes into it - no tier can prove it dead. Emitted AOT C++
  keeps the cast and the store verbatim. Caching into anything the caller owns by const
  VALUE would not survive this.

## 21. flat_hash_table

- **`hashes[i]` is the slot state** - 0 never-used (probe stops), 1 tombstone (probe
  continues), above 1 live; a hash function that can return 0 or 1 loses entries silently.

## 22. coverage

- **Instrumentation is `generated`** (lint-invisible, inliner-safe) and asserts become
  verify so their spliced counters survive release.

## 23. regex

- **Zero-width nodes (lookahead, Bos/Eos, word boundaries) leave `subexpr.next` null on
  purpose** - chaining them would consume the continuation.
- **The literal matchers compare byte-by-byte and return on the first mismatch** - a
  fixed-`textLen` block compare (memcmp) reads past the terminator whenever the terminator
  falls before the literal's last byte (the tail at least two bytes shorter than the
  literal).

## 24. debugger

- **`g_installed_agents` is the GC root for every installed agent** - the C++ adapter holds
  a raw classPtr the das GC cannot see.

### 24.1 Debugger readiness query {#debugger-readiness-query}

- **Non-allocation debug-agent callbacks and cross-context invocations share the debug-agent
  context mutex** - callbacks can run on debuggee or debugger worker threads, while pinvokes can
  arrive on the main thread. Allocation instrumentation callbacks remain direct because they run
  inside allocator operations, where re-entering the same allocation path cannot be serialized.
- **The debugger-ready query uses an explicit pinvoke** - generated `apply_in_context`
  verification would acquire the agent registry while holding the context mutex, opposite to the
  debugger tick's registry-to-context order.
- **A hook never waits on a thread that has yet to create a context** - the tick holds the
  agent registry (`g_DebugAgentMutex`, `src/runtime/context.cpp`) for the whole callback, and
  every `Context` construction walks that registry for `onCreateContext`, so a `new_thread`
  started while a hook blocks cannot begin. `try_invoke_in_context` is the non-blocking attempt
  on the context mutex - false while a hook runs - so the threadlock test proves the
  callback/pinvoke serialization from one thread, with no timing window and no thread started
  under a blocked hook.

- **The stepping debugger is armed by an `[init]` that tells the debuggee's context apart from a
  macro context by `is_compiling_macros()`, not by the macro-module test alone** - `Program::simulate`
  binds the program while init scripts run, so `is_compiling_macros_in_module("debug")` answers
  false in the debuggee's own init exactly as it does in another module's macro context; a
  runtime init gated on that test alone never runs where it matters. The `[_macro]` functions
  keep the bare test: they never run at simulate time.
- **A folding context is never a debuggee thread** - the optimizer evaluates a pure call over
  constants in a context of its own (`ContextCategory.folding_context`), during the compile;
  `onCreateContext` skips it, so a breakpoint inside a folded call reports nothing rather than a
  stop inside the compiler. The call itself is gone from the program: a debugger that must stop
  there launches with `-no-optimization` (the DAP bridge's `optimize=false`).

### 24.2 Debugger worker startup {#debugger-worker-startup}

- **The statement-debugger worker executes its lambda only after the source context reaches
  `onSimulateContext`** - its cloned context shares the finalized SimNode graph, so executing the
  lambda while `Program::simulate` is still hashing that graph is a data race. With
  `--das-wait-debugger`, the main thread pumps DAP requests until client configuration completes;
  without that flag, source-context simulation proceeds while the worker waits for it to finish.
- **Only one statement-debugger worker may be active** - a second launch fails immediately rather
  than blocking the source thread that must deliver the first worker's readiness notification.
  Destroying that source context cancels the pending worker so its raw identity cannot outlive it.

### 24.3 Statement breakpoint lookup {#statement-breakpoint-lookup}

- **Statement stepping releases a breakpoint table borrow before entering the stopped command
  pump** - requests handled while stopped may replace that table entry.

## 25. typemacro_boost {#typemacro-boost}

- **The parser does not run annotation `apply` for macro-added functions** - the add/erase
  pair in `tsi_stamp_methods` IS the trigger, not bookkeeping.

- **A derived structure carries every ancestor level's fields before annotations run.**
  `st.fields` on the instance already lists what the parser materialized from the whole parent
  chain, so the reifier binds constants, gates and calls from the instance alone and never walks
  an ancestor's field list for them. Only methods need the per-level walk, nearest ancestor
  first, where the first name seen wins; a slot the surviving concrete parent declares keeps the
  parser's `cast<auto>` init (the parent's slot type binds it), and a slot only the cut chain declared - an override at any template level, the finalizer - drops it, as infer does on a parentless class.

## 26. constant_expression

- **The generated specialization is called through `__::`** - a plain name resolves in the
  call site's DEFINING module, where it does not exist.

## 27. toml

- **The writer is hand-rolled because the builtins do not round-trip** (`\v`, raw 0x7f,
  float exponent thresholds); a value scan that runs into a bare-key char rewinds and
  re-lexes as a key.

## 30. archive

- **`MemSerializer.write` grows capacity eagerly** because under a very_safe_context each
  doubling generation is abandoned, not reused; no alias into `data` survives a write.

## 31. json

- **`is_json_white_space` is deliberately not the shared `is_white_space`** - RFC 8259
  admits exactly space/tab/CR/LF.

## 32. jsonrpc

- **Parsing a request is a SCOPE, not a value.** `ParsedRequest.params` is a borrowed view
  into the parse tree, so the tree has to outlive the handler and cannot be freed inside the
  parser - instead the owner is named in the result (`document` on `ParsedRequest` for
  `parse_request`, on `ParsedBatch` for `parse_batch`, where the entries borrow and own
  nothing) and the scope is closed by `free_request` / `free_batch`; `dispatch_line` owns
  the whole scope and frees after the dispatcher has run over every entry. Freeing is safe
  for the string fields because daslang never finalizes a `string` field
  (`TypeDecl::needDelete` is false for `tString`), so `method` / `id_str` / `params_json` /
  `error_envelope` outlive the tree - only the `params` pointer dangles, which is why the
  free nulls it. The response side has no such scope: `parse_one_response` copies everything
  into strings, so `parse_response` / `parse_response_batch` free their own tree before
  returning.

## 34. fio

- **The glob matcher follows POSIX fnmatch on degenerate patterns** (unterminated `[`, `]`
  as first class member, `**/` slash rules) - conformance, not quirks.

## 35. decs

- **Component finalizers are lambdas over the component's own untyped storage** - deleting
  the lambda IS the finalization event; capacity checks compare the highest allocated value
  (`base + count - 1`), not the exclusive end.

## 36. builtin

- **`_table_index_and_init` exists for infer's `default_init_containers` rewrite of
  non-store `tab[key]`** - it has no daslib call site and is not dead.

## 37. tune {#tune-shells-module-exists}

- **Every tune annotation - `[tune]`, `[tuned]`, `[tune_perm]`, `[tune_scope]`, `[tune_policy]`,
  `[llvm_code]` - registers HERE and nowhere else, as a shell that forwards to the framework in
  `llvm/daslib/llvm_tune` when that module is visible and answers "unchanged" when it is not.** A
  module's macro state is per module, so a shell and the `[tune_scope]` bank it reads have to
  share one macro context; `tune_status` and `log_tune_status` are shells of the same kind.
- **The shells decide with `typeinfo module_exists(llvm_tune)`, a pair with the trait's C++ half
  in `src/ast/ast_infer_type.cpp` (`module_exists`): visible from the compiling module, which is
  what the `require [tune_framework] public` line above them decided.** Nothing fails
  when the two sides drift - `tests/language/optional_require.das` pins the trait's visibility
  half on fixtures, and `tests/daslib/test_tune_shells.das` is tier-agnostic by design - so the pair
  is recorded here: a trait that answered off the process registry instead would take the
  framework arm inside a tool's nested compile, where the framework is not mounted.
- **The `llvm/daslib/*` files resolve in every build that carries `modules/dasLLVM/`; only a
  build configured with dasLLVM joins the `tune_framework` group with `llvm_tune` - the `llvm`
  witness module's constructor for a static host, the descriptor's row guarded on `llvm` for a
  DLL build - so the group, and with it `module_exists(llvm_tune)`, follows the configure and
  never the das files on disk.** `llvm_code` is not a member: `llvm_tune` requires
  it `public`, so every requirer of the group sees the `[llvm_code]` annotation through it, as
  every requirer of `llvm_tune` always did. The framework's das files compile without their C++ module, so a direct
  `require llvm/daslib/llvm_tune` in a build without dasLLVM compiles with the shells inert and no
  `<name>_variants()` registry - a program that reads one is framework-only and says so with that
  direct require. `daslib/just_in_time` keeps its direct require for the opposite reason: a static
  host that never registered the witness still runs the JIT through the LLVM library, and the
  group would switch it off.

## 38. cross_context

- **A function reached by name in the context `macro_context_of` answers is `[export]`ed, a pair
  with `Program::markMacroSymbolUse` (`src/ast/ast_export.cpp`), whose roots are `exports`, `init`,
  `shutdown` and `macroInit` - every other function is dropped from the macro context.** Nothing
  fails when one side moves alone: the tests name an `[export]`ed function, so a narrower or wider
  C++ root set passes them. The module itself is not the caller's dependency (`src/ast/ARCHITECTURE.md`
  sec.3): its macro context is the caller's only handle on it.

## 39. ast

- **Every `[macro_interface]` hook's `args` in `ast.das` is `var` because its C++ twin takes
  `AnnotationArgumentList &`: `FunctionAnnotation`, `StructureAnnotation` and
  `EnumerationAnnotation` in `include/daScript/ast/ast.h` (repo root) hand the adapters in
  `src/builtin/module_builtin_ast_adapters.cpp` the declaration's own `arguments`, and
  `include/daScript/builtin/ast_gen.inc` - regenerated from `ast.das` by
  `utils/internal/dasgen/gen_bind.das` - carries the same reference type.** Nothing fails when
  one side moves alone: an override compiles under either constness and the adapter resolves a
  hook by field name, so a C++ side back on `const` leaves daslang writing through a const
  reference with every suite green. `progArgs` (the program's `options`), `simulate` and the
  `aot*` hooks stay const on both sides.

## 40. Package release symbols {#package-release-symbols}

`daspkg.das:release_include_symbols` sets `ReleaseSpec.include_symbols`.
`utils/daspkg/commands.das` consumes that field: the native release copies Windows
PDBs, while `cmd_release_wasm` passes it to `release_one_wasm_app` to retain DWARF,
logical frames and the emcc symbol map. The manifest contract and these release
implementations describe the same option.

## 41. ast_boost

- **C++ mirror pair**: `table_key_storage_base_type` / `peel_distinct` <-> `TypeDecl::getR2VType` and `TypeDecl::isTableKeyType` (`src/ast/ast_typedecl.cpp`), the key-node rule in `src/ast/ast_simulate.cpp`: a table key is stored and hashed as its base type under every distinct level, a handled key as its value type, and the JIT emitters (`modules/dasLLVM/daslib/llvm_jit.das`, `llvm_exe.das`, `llvm_jit_plan.das`) read the daslib twin. Nothing in the default suite fails when one side moves alone (the JIT lane needs `DAS_LLVM_DISABLED=OFF`); `tests/language/distinct_table_keys.das` under `-jit` is the check.
