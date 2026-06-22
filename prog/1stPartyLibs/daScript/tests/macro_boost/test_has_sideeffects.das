options gen2
require dastest/testing_boost public
require _has_sideeffects_probe public

// ── Side-effect-bearing helpers (used as test sources) ────────────────────

var g_proj_hits = 0

def side_effect_fn(_x : int) : int {
    g_proj_hits ++
    return _x * 2
}

struct Foo {
    a : int
    b : int
}

// ── SAFE cases — has_sideeffects must return false ───────────────────────
//
// Note: `let _x = 5` lets the compiler fold expressions using `_x` into constants
// before the call_macro runs (so the macro sees ExprConstInt, not the original
// ExprOp2). To exercise the operator paths explicitly, tests below use `var`.

[test]
def test_const_int(t : T?) {
    t |> equal(_test_has_sideeffects(42), false)
}

[test]
def test_const_string(t : T?) {
    t |> equal(_test_has_sideeffects("hello"), false)
}

[test]
def test_const_bool(t : T?) {
    t |> equal(_test_has_sideeffects(true), false)
}

[test]
def test_var_read(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(_x), false)
}

[test]
def test_arith_pure(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(_x + 1), false)
}

[test]
def test_arith_nested(t : T?) {
    var _x = 5
    var _y = 3
    t |> equal(_test_has_sideeffects(_x * 2 + _y - 3), false)
}

[test]
def test_field_access(t : T?) {
    var _s = Foo(a = 1, b = 2)
    t |> equal(_test_has_sideeffects(_s.a), false)
}

[test]
def test_array_index(t : T?) {
    var _arr = [1, 2, 3, 4, 5]
    t |> equal(_test_has_sideeffects(_arr[0]), false)
}

[test]
def test_safe_table_lookup(t : T?) {
    var tab : table<string; int>
    tab |> insert("k", 1)
    t |> equal(_test_has_sideeffects(tab?["k"]), false)
}

[test]
def test_comparison(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(_x == 0), false)
}

[test]
def test_ternary(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(_x > 0 ? 1 : 2), false)
}

[test]
def test_null_coalescing(t : T?) {
    var _p : int? = null
    t |> equal(_test_has_sideeffects(_p ?? 0), false)
}

[test]
def test_logical_and(t : T?) {
    var _x = 5
    var _y = 10
    t |> equal(_test_has_sideeffects(_x > 0 && _y < 100), false)
}

[test]
def test_unary_neg(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(-_x), false)
}

[test]
def test_string_builder_safe(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects("hello {_x}"), false)
}

// ── UNSAFE cases — has_sideeffects must return true ──────────────────────

[test]
def test_user_call_unsafe(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(side_effect_fn(_x)), true)
}

[test]
def test_table_insert_subscript(t : T?) {
    var _tab : table<string; int>
    // _tab[k] auto-inserts a default value if k is missing — side effect.
    t |> equal(_test_has_sideeffects(_tab["k"]), true)
}

[test]
def test_division_unsafe(t : T?) {
    var _x = 10
    var _y = 2
    // `/` can panic on div-by-zero — kept on the unsafe side by explicit blacklist.
    t |> equal(_test_has_sideeffects(_x / _y), true)
}

[test]
def test_modulo_unsafe(t : T?) {
    var _x = 10
    var _y = 3
    t |> equal(_test_has_sideeffects(_x % _y), true)
}

[test]
def test_array_literal_alloc(t : T?) {
    t |> equal(_test_has_sideeffects([1, 2, 3]), true)
}

[test]
def test_struct_construct_alloc(t : T?) {
    t |> equal(_test_has_sideeffects(Foo(a = 1, b = 2)), true)
}

[test]
def test_string_builder_unsafe_part(t : T?) {
    var _x = 5
    // The string interpolation itself is safe, but a side-effecting operand propagates.
    t |> equal(_test_has_sideeffects("hello {side_effect_fn(_x)}"), true)
}

// ── Mutation operators — must be unsafe regardless of resolved-builtin flags ──
//
// `++`/`--` (ExprOp1) and compound-assignment ops (`+=`/`-=`/… ExprOp2) mutate
// their operand. has_sideeffects blacklists these up front so a builtin
// mistakenly missing `knownSideEffects`/`unsafeOperation` can't classify them
// as pure. Covers Copilot review concern from PR #2691.

[test]
def test_postfix_increment_unsafe(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(_x ++), true)
}

[test]
def test_postfix_decrement_unsafe(t : T?) {
    var _x = 5
    t |> equal(_test_has_sideeffects(_x --), true)
}

// ── User-defined operator overload — must be unsafe (non-builtin func) ───
//
// Overloads of `+`, `*`, etc. on user types can carry arbitrary side effects.
// The op-name allowlist must NOT bypass the func-flag check when func != null
// — otherwise a custom `def operator +` slips through as pure. Covers Copilot
// review concern from PR #2691.

var g_op_overload_hits = 0

struct private SideEffectingNumber {
    v : int
}

def operator +(a : SideEffectingNumber; b : int) : SideEffectingNumber {
    g_op_overload_hits ++
    return SideEffectingNumber(v = a.v + b)
}

[test]
def test_user_op_overload_unsafe(t : T?) {
    var _c = SideEffectingNumber(v = 5)
    t |> equal(_test_has_sideeffects(_c + 1), true)
}

// ── Conservative-unsafe cases — daslang-generic helpers fall through ─────
//
// `length`, `key_exists`, etc. are defined as daslang generics in builtin.das
// (`def length(a : auto | #) ...`). The compile-time func resolution doesn't
// always reach a `flags.builtIn=true` C++ overload before the call_macro runs,
// so the conservative classifier rejects them. A future Function-level
// `[no_side_effects]` annotation could let user-defined helpers opt in.

[test]
def test_generic_length_unresolved(t : T?) {
    var _arr = [1, 2, 3]
    t |> equal(_test_has_sideeffects(length(_arr)), true)
}

[test]
def test_key_exists_unresolved(t : T?) {
    var tab : table<string; int>
    tab |> insert("k", 1)
    t |> equal(_test_has_sideeffects(key_exists(tab, "k")), true)
}
