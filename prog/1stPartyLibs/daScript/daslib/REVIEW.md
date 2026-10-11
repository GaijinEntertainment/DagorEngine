# daslib Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture
docs: `ARCHITECTURE.md` and its `ARCHITECTURE_<CONCERN>.md` companions in this folder.
A diff touching the linq family - `linq*.das`, `sql_*.das` - applies `REVIEW_LINQ.md` too. A
diff that changes compile-time AST-building code - a class deriving an `Ast*Annotation` or
`Ast*Macro`, an `AstVisitor` a macro or an annotation runs at compile time, a
`[macro_function]`, or any function that calls `qmacro*`, `apply_template`, `macro_error`, or
`macro_sticky_error` - applies `REVIEW_MACROS.md` too. A
diff touching `daspkg.das` - the functions a `.das_package` manifest body calls - applies
`utils/daspkg/REVIEW.md` (repo root) too. A diff touching `shader_lingua_franca.das` - the
declarations both shader emitters compile - applies `modules/REVIEW_SHADER_EMITTERS.md` (repo
root) too.

**A diff that changes a `.das` here that an `AOT_*_MODULE_FILES` variable of
`tests/aot/CMakeLists.txt` (repo root) picks up, and that is not marked `options no_aot`, states
in the PR body that the full `test_aot` lane (`preflight --full`, or a manual dispatch of
`build.yml` on the branch) ran green on the diff's head commit.** Per-PR CI compiles only the
language subset.

**A diff that changes the body of `perf_warning` (`perf_lint.das`), `style_warning`
(`style_lint.das`), or `lint_error` (`lint.das`) runs the suppression check before the body
records or reports the finding.** A nolint'd finding that takes the (rule, location) slot
silences a co-located sibling rule.

**A diff that changes the dedup key keeps the rule code in it.** A key without the code
collides across rules.

**Never narrow PERF026-028's `// nolint` check to the reported sink line - honor a
`// nolint` at any frame of the reported chain.** The sink often bottoms out in daslib while
the actionable line is an intermediate call site.

**A diff that moves an `in_closure` / `in_deferred` guard to a different statement position
inside a lint visitor method is a defect.** The guard's position decides which rules apply
inside a lambda; the per-rule policy is in `ARCHITECTURE_LINT.md` sec. 1.

**Never move PERF023's `add_ptr_ref` splice-depth tracking (`perf_lint.das`) behind the
`in_closure` gate.** The rules block is itself a closure, so a gated tracker never sees the
splice.

**A lint warning that anchors anywhere but the code its fix rewrites is a defect.** A remedy
that deletes a statement reports the statement; a remedy that edits one variable's
initializer reports the variable.

**Never exit a construct's visitor on a path that skips resetting or restoring the visitor
state scoped to that construct.** State an early return leaves set poisons the next
construct's visit.

**Visitor state that must be restored to a value saved when its construct was entered lives in
a stack, never in a bare scalar.** A saved value kept in a scalar survives into the sibling
construct's exit path and unbalances it.

**A diff that adds or changes something a second site must agree with - a daslib fact (code or a
`//!` contract), or what a C++ name `aot_cpp.das` writes into generated code does or returns -
records the pair in an `ARCHITECTURE*.md` of this folder, naming both sides, whenever nothing
fails on a mismatch: no lint, no compile error, no default-suite test (what `dastest` runs over
`tests/` (repo root) with no flags).** The second site can sit anywhere, inside or outside this
repository. The section a pair lands in is the one holding its daslib file (for a C++ name, the
emitter file that writes it), or a new one in `ARCHITECTURE.md` or a companion when none does; a
pair whose two sides are one syntax's spellings lands in `ARCHITECTURE_SYNTAX.md` with the other
pairs of that syntax.

**When a diff changes the daslib side of a recorded pair so the two no longer match, it changes
the other side in the same diff.** A change to the other side is reviewed under that side's
checklist, which routes here.

**A diff that renames or deletes a daslib name an `ARCHITECTURE*.md` in this folder spells, or
changes what that name names so that the entry's claim stops holding, changes that entry in the
same change.** The doc is read as the current tree, and a name that no longer resolves or a claim that
no longer holds is trusted until someone reads the code.

**A diff adds an architecture-doc entry only for a fact that no lint, compile error, or
default-suite test reports when it goes false, and that the code's own names do not make
self-evident** - a test gated on an environment variable, or run only in a nightly lane, does
not count.

**A diff that changes daslib's nolint scanning - suppression parsing, staleness detection, or
the header window - keeps `tests/lint/test_nolint_suppression.das` and
`tests/lint/test_stale_nolint.das` (repo root) green without weakening them.** They pin that a string
literal, a URL, and a mid-comment `nolint:` do not suppress while a first-token directive
after `//` or `//!` does, and that a `lint-skip-file` past the header window is prose.

**A diff that changes `build_lint_macro_disabled` keeps every layer beaten by the one after it,
with the per-file `options _nolint`/`_lint` overrides last.** Env after the repo `on` erase lets
a one-run `DAS_LINT_DISABLE` beat a `CODE = true`.

**A diff that changes `options _enable_default_off_rules` keeps it skipping BOTH the default
seeding and the repo `off` directives.** Repo policy must not silence the rule a fixture
exists to exercise.

**A diff that changes daslib's `.lint_config` resolution keeps `tests/lint/test_lint_config.das` (repo root)
green without weakening it** - it pins that each `[format]` key resolves nearest-wins
independently up to the `.git` root, so a nearer `.lint_config` declaring one key does not
reset the other.

**A daslib module that emits a lint rule id joins `RULE_MODULES` in `utils/lint/REVIEW.das`
in the same change.** An unlisted module's ids are never scanned, so its fixture-and-rst
check never runs.

**A diff that deletes a rule id's last emitting spelling from a rule module also removes that
id's fixture and its `doc/source/reference/language/lint.rst` (repo root) section, in the same
change** - the gate checks only ids it can still find, so a dead id's residue outlives it
silently.

**Never record a LINT010 store in `preVisitExprVar` - record it in the POST-visit of
`ExprCopy`/`ExprClone`/`ExprMove`.** The LHS's variable fires before the RHS is walked, so
an early record lets the RHS's own read clear the store it just recorded.

**Never match a lint rule's callee name against the instance - match the root generic.**
Instantiation mangles names and strips witness arguments.

**A lint rule that fires on the source of the module that implements the idiom it suggests
is a defect.**

**A diff that adds or changes a collapse check - a lint rule that suggests replacing a run of
statements with one expression - gates the suggestion on receiver type, arity, cloneability,
and duplicate constant keys.**

**A diff that adds or changes the rewrite a lint message suggests adds that rewrite to the
rule's fixture in `utils/lint/tests/` (repo root) as a good shape that compiles and does not
fire.**

**A diff that makes two collapse checks fire on one shape is a defect.** Overlaps stay
partitioned: STYLE021 owns the const-key run over STYLE031, STYLE032 owns fresh-empty
declarations over STYLE033, and STYLE033's chain-receiver check leaves a plain-variable
receiver to STYLE033's variable-form check.

**Never reword a lint message so the rule code stops being everything before the FIRST
colon.** Suppression, disable lists, and dedup all parse the code from the message, so a
message that leads with anything else makes the rule unsuppressable and self-colliding.

**A diff that adds or changes a `canVisit*` override in style_lint keeps the unsafe-frame
count balanced across the subtree the override skips.** The walk pushes one frame per
expression and pops it, so a skipped subtree unbalances the count; the balance panic in each
entry point is the tripwire.

**Never cache `blk.list` index state across style_lint visitor callbacks.** The
per-statement callback carries no index and a shared stack would have to push/pop with
nested blocks; block statement lists are short, so the rescan is the cheap form.

**A diff that changes an arity cap or the overload set it mirrors - `MAX_CONCAT_ARITY` <->
linq's `concat` variadics, `MAX_VARIADIC_PUSH_ARITY` <-> builtin
`push_from`/`push_clone_from` - changes both sides in the same change.** Growing an overload
set without the cap is a silently missed finding; raising a cap without the overloads is a
suggestion that does not compile.

**A diff that adds or changes an emit entry point - a function that runs the emit visitor
(`CppAot` or any subclass of it) and then writes the generated C++ to a file - keeps the
error check ahead of that file write.** The error check reads the program's failed state:
`program.failed`, both `program.flags.macroException` and `program.flags.failToCompile`, or
`log_aot_emit_errors`, which reads those two flags. A codegen exception mid-visit leaves
partial C++.

**Never gate a visitor override in `CppAot` or any subclass of it on the program's failed
state - `program.failed`, `program.flags.macroException`, `program.flags.failToCompile`, or
`log_aot_emit_errors` - the function that runs the visitor owns that check.** An override that
returns early on the error state emits truncated C++ that the caller still writes out as
complete.

**Never report an unreachable emit state with `panic` - write `#error` into the output.**
`runMacroFunction` swallows a panic, so the emitter never reports through it.

**A diff that changes how a struct's C++ name is built changes every site that spells
that name the same way.** The name is the module namespace (`aotModuleName`, or
`aotModuleNameFromString` in `describeCppStructInfoFields`) plus the `_S` suffix; the sites
that spell it are `aotStructName` and `describeCppStructInfoFields`'s inline
`aotSuffixNameEx(info.name, "_S", ...)` (both `aot_cpp.das`). One site changed alone writes
`offsetof`s that name a struct declared under a different name.

**A diff that adds or changes a function that emits a struct or enum C++ name - anything
reaching `aotStructName` / `aotEnumName`, a `CppAot` subclass and `ArgsConverter` alike -
keeps `buildStructEnumCollisions` running before it, directly or in a helper it calls.**
The table decides when a name gets its collision suffix, and a run that skips the seeding
spells structs differently from the run that seeded it.

**Never index a UTF-8 byte-class table with a raw `ch` - index through `uint(uint8(ch))`.**
`for (ch in string)` yields a SIGNED byte under JIT, so a raw index reads out of bounds for
every byte >= 0x80.

**A diff that adds or changes a parser result that borrows a view into a tree the parser
allocated names the field that owns the tree and ships the function the caller runs to free
it.** The tree must outlive the caller's read, and daslang finalizes neither a raw pointer
field nor a local container at scope exit, so a borrowed view with no named owner leaks the
whole document per call.

**Never convert a string with `int64` / `uint64` / `double` in a lexer or decoder reachable
from a file, a socket, or a model - use the non-throwing `to_*` twin and report through the
code's own error channel.** Those three throw on out-of-range input, and a throw on
untrusted bytes is a panic.

**Never branch a parser on the text of its own diagnostics - branch on the token that caused
the failure.** A message carries user data, so `starts_with` on an error string is an
input-controlled branch.

**A diff that adds or changes a flatten_opt rewrite arm ships a read-only residual predicate
with that arm** - the oracle the residual visitors call to prove the pass complete.

**Never let a residual oracle's gate differ from its arm's gate.** A gate the transform
takes as a parameter is threaded into the oracle too, and the tests that call the oracle
pass the same value; a narrower oracle is a false pass, a wider one a false miss, a
re-spelled parameter default a false pass on every non-default run.

**Never call the transform from an oracle** - calling it aliases the live tree.

**A diff that adds a fold/fuse arm gates it on `no_fast_math` when the rewrite changes
Inf/NaN, rounding, or association, and leaves it ungated when the rewrite is bit-exact per
lane.** An ungated value-changing arm silently changes output under `_flatten_no_fast_math`.

**Never emit a fused call before proving the target module can resolve it - fall back to the
unfused shape on a miss.** Otherwise the pass turns a shader that compiled into an
unresolvable call on a narrower backend.

**Never materialize a `string` per `ExprVar` in a flatten visitor callback.**

**A new store spelling joins `MutCollect` in the same change.** CSE reads a name outside the
mutable set as constant for the whole block, so an uncollected store is a silently shared
subexpression.

**Never drop a statement the lowering cannot predicate - refuse it.** `lower_stmt`'s
fall-through drop is licensed only because every call that survives lowering is pure.

**A diff that reorders `flatten_function`'s passes is a defect - the order is dse ->
copy-prop -> mask-const-prop -> dse -> ssa-rename.** Each pass produces the next one's
input, and a reorder leaves scaffolding in the generated `<name>_flat` twin, which must come
out branchless and call-free.

**Never use `make_visitor` in `lift_expr` or the lowering it drives - use plain recursion.**
Inlining recurses back through the pipeline, and a nested visitor traversal corrupts the
visitor machinery.

**A new daslib spelling that reads the environment joins the env_registry marker lists in
the same change.** The scanners match daslang source text; a missing spelling makes every
enforcement test pass vacuously.

**A diff that adds or changes an `ast_verify` check names, in `ARCHITECTURE.md` sec. 8, the
C++ `::visit` site that dereferences the slot unguarded.** A check with no such site invents
an invariant the compiler does not hold, and the two passes disagree - a slot infer fills in
is checkable only post-infer.

**A diff that adds or changes an `ast_verify` check repairs the slot it reports, not only
reports it.** A broken slot left in place trips every later pass that walks the same node.

**Never sum a range's start and count as `uint64` before proving both non-negative** - a
signed sum wraps, the bounds test passes, and the walk leaves the array silently.

**A diff that adds a function to `cross_context.das` wrapping `invoke_in_context` - taking the
`unsafe` and the `addr` off the caller - is a defect; the caller writes the call itself.** The
wrapper's signature checks nothing the call does not: the target is found by name and argument
count, and a pointer crosses into a context that holds its lock for the whole run, so the
`unsafe` belongs at the site that hands the pointer over.

**A diff that adds or changes a buffer-I/O overload returns before taking `addr(buf[0])` on
an empty buffer.** The address is out of bounds and the call sits inside `unsafe`, so an
overload without the guard passes every non-empty test.

**Never decide lvalue-ness in `apply`'s inline path from a ref flag - decide it by node
KIND.** An rvalue stays materialized, because a reference to a temporary dangles.

**Never generate a `let` on `apply`'s inline path without `alwaysSafe`** - that flag, not an
`unsafe` block, licenses the variant access.

**Never reuse a swizzle rewrite's source node in a second output lane without cloning it** -
the first appearance moves it, and skipping the clone gives one node two parents.

**A diff that adds or changes a site in `rst.das` that spells a symbol's RST label (the
`.. _name:` target) or topic key (the `|detail/...|` / `|handmade/...|` substitution name) keeps
every site in `rst.das` that spells the label or key of that same symbol byte-for-byte equal.**
When two such sites diverge, the page prints a bare signature and the symbol re-stubs.

**A diff that changes the category or the name a topic key carries - in `rst.das`'s `topic()`
sites or in `rst_comment.das`'s `write_to_detail` names - makes the same change on the other
side, in the same change.** The reader writes a detail file under one key and the page reads
under the other; a mismatch is a bare signature.

**A diff that changes what `rst.das` or `rst_comment.das` places on a page states, in the PR
body, that every name the generator newly places names a symbol, module or file that exists in
the repository at merge, and where that was checked** - a name the generator lists is a name a
reader will type.

**A diff that adds a numeric value form to the toml lexer routes it through `rewind_to_bare`
on a bare-key character.** Without the rewind, a bare key that starts like a number lexes as
a value.

**A diff that adds or renames a public intrinsic in `aarch64_neon.das`, `x64_avx.das` or
`f16_cvt.das` changes its row in the lookup table of `modules/dasLLVM/daslib/llvm_jit_intrin.das`
(repo root) that is gated on the CPU feature the instruction needs, in the same change.** The JIT
recognizes these calls by function name alone, so a missing row runs the portable fallback on
every target with every test green, and a row in a table gated on a weaker feature emits an
instruction the generic machine cannot select - the `tests/jit_tests` twins compare hardware
against a reference body and catch a wrong emitter, never a missing or misplaced one.

**A function in `aarch64_neon.das`, `x64_avx.das` or `f16_cvt.das` that is the wider- or
newer-tier variant of the same math another function in that file computes calls that function -
never a hand-written scalar loop.** A box whose CPU lacks the tier then runs the recognized
narrower path instead of scalar code. A tier function with no same-math sibling (`smmla`'s 2x8
by 8x2 shape) keeps its own fallback body.
