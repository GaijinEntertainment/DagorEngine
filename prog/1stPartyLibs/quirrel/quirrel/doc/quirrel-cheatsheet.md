# Quirrel cheat-sheet

Quick rules for writing and reviewing Quirrel (.nut) code. Aimed at reviewers,
agents and tooling; not a language manual. Full docs: `doc/source/` in this repo
(start with `reference/language/`). Analyzer diagnostics list:
`squirrel/compiler/compilationcontext.h` (DIAGNOSTICS macro). Facts below are
verified against csq 1.0.37 behavior, not just docs.

## Strings

- NEVER use `+` for string building. `+` with a string operand silently
  concatenates instead of throwing, and left-associativity makes results
  order-dependent (verified): `1 + "2"` == `"12"`, `2 + 3 + "1"` == `"51"`,
  but `"1" + 2 + 3` == `"123"`, `1.5 + "2"` == `"1.52"`.
  All other arith ops (`- * / %`) DO throw on strings; only `+` hides the bug.
  - RIGHT: `$"score {score} of {total}"` (interpolation, compiles to subst),
    `"".concat(a, b, c)`, `", ".join(arr)`, `"{0}:{1}".subst(a, b)`.
  - Analyzer: w264 (plus-string).
- `$"..."` cannot nest; escape literal braces as `\{ \}`.
- Format/subst argument counts are checked: w231, w334.

## Bindings and declarations

- `let` = non-reassignable binding, `local` = reassignable. Default to `let`;
  use `local` only when you actually reassign. `let t = {}` still allows
  `t.x <- 1` (the binding is fixed, the object stays mutable; use `freeze()`
  for immutable data).
- `let name;` forward-declares a binding; a later statement of the *same* scope
  must define it exactly once. Reading it earlier is an error except from a
  nested closure, which sees null until the definition runs.
- Declare functions as `function foo() {}`. `let function foo() {}` and
  `let foo = function foo() {}` mean the same thing and are legacy style; plain
  `function foo() {}` is already a non-reassignable declaration.
- `const` for compile-time scalar constants (folded into bytecode, no lookup;
  also usable inline inside expressions). `global const` / `global enum` when
  it must be visible across the module boundary.
- `static <expr>` evaluates once and memoizes; expression must be const-like
  (w316, w317).
- Mark side-effect-free functions `function [pure] f()` / `@[pure](x) ...`:
  enables const folding and use in constant context.
- Classes: `class Baz(Bar)`, not `extends`. Do not add methods via
  `class::method`; declare in place or add slots with `<-`.
- Prefix intentionally unused vars/params with `_` (w228 declared-never-used,
  w291 fires if a `_name` is actually used).
- `@@"..."` docstrings document exported APIs.

## Type annotations (see reference/language/type_annotations.rst)

- Syntax: `function f(x: int, s: string|null = null): float {}`,
  lambdas `@(a: number): number a * 2`, combined `function [pure] g(v: number): int`.
- Types: `int float number bool string null table array function userdata
  generator userpointer thread instance class weakref any`; unions via `|`;
  `number` == `int|float` (prefer it for numerics).
- Checks are enforced at RUNTIME (calls, returns, assignments, destructuring):
  a wrong annotation is a crash, a missing one costs nothing.
- Annotate new code freely. When retrofitting a shared/library function,
  annotate a param only if a value outside that type would already fail in the
  body today; otherwise you may break an existing caller that static checks
  cannot see. Watch out: params used only via `+`, `.tostring()`, `.tofloat()`,
  equality or truthiness tolerate "wrong" types at runtime.
- Return type only when every path agrees; valueless `return` and falling off
  the end yield null, so use `T|null` or skip.
- Type checks add a small per-call cost: skip annotating trivial helpers in
  per-frame/per-item hot loops.

## Modules

- Prefer `import` over `require`: it is compile time, so the analyzer verifies
  the module exists and every imported field (missing-field w312,
  imported-never-used w230, duplicate-import w332), and imported names are
  constant bindings, so member access resolves without runtime lookup, and
  pure/const exports can fold.
  - RIGHT: `import "foo.nut" as foo` / `from "%sqstd/string.nut" import utf8ToUpper`
  - `require()` stays for conditional/optional loading (`require_optional`).
- Module body runs once; return the export table, usually `return freeze({...})`.
- `persist("key", @() init)` keeps state across hot reloads; unique keys (w293).

## Containers and iteration

- `foreach` over a table has NO defined order (verified: insertion order is not
  preserved). Never rely on it for output, hashing or UI; iterate a sorted key
  array when order matters.
- A `map()` callback may `throw null` to drop the element (verified): single
  pass instead of `.filter().map()`. Note `throw` is a statement, not an
  expression: `arr.map(function(v) { if (!v.enabled) throw null; return mk(v) })`
  == `arr.filter(@(v) v.enabled).map(mk)`.
- `arr.append(a, b, c)`, not `arr.extend([a, b, c])` (w270) and not `push`
  (removed). `array(n)`, not `[].resize(n)` (w319). `t.clone()`, not
  `t.__merge({})` (w318). `.indexof()`, not `.find()`.
- Membership: `x in t`, `x not in t`, `arr.contains(v)`.
- Do not modify a container inside its own foreach (w292).
- `delete` operator is deprecated: use `t.$rawdelete(key)` (`$` = type-method
  access, avoids clashing with a slot named `rawdelete`).

## Null safety

- Use `?.` `?[]` `?()` for potentially-null chains and `??` for defaults; the
  analyzer tracks nullability (w200/208/210/220/248/339 family) and useless
  checks (w285 expr-cannot-be-null).
- Priority trap (w240): `a ?? b > c` parses as `a ?? (b > c)`; parenthesize.
- Default param values evaluate at call time, and a mutable default (table or
  array) is shared between calls: never mutate it (w335).

## Misc correctness

- `&&` binds tighter than `||` (w202); parenthesize mixed chains. Same for
  ternary (w215) and shifts (w236).
- Integer division truncates: `a / b` with two ints is int (w235 round-to-int).
- No implicit bool coercion in arithmetic: `1 + true` throws; bitwise ops
  require ints (floats throw).
- async: returning an async call without `await` settles with the Future, not
  its value; `return await f()` (w336).
- Numeric literal separators help review: `1_000_000`.

## Style enforced by the analyzer

- Egyptian braces (w263), consistent indentation (w315), no trailing
  whitespace (w277), no statement on the same line after `}` (w192).

## Running the tooling

- Single file: `csq-dev --static-analysis --absolute-path <file.nut>`
  (exit 0 and zero warnings expected; warnings fail validation gates).
- Whole tree: `python -m drey . --use-configs` (uses nearest `.dreyconfig`,
  which may disable specific checks via `__disabled_checks`).
