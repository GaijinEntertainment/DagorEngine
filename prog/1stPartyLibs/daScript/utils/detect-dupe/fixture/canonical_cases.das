options gen2


// ── Alpha-rename equivalence ──────────────────────────────────────────
// These three functions differ only in identifier names and parameter
// types; CanonicalVisitor must collapse them all to the same tag stream
// (var slots numbered by declaration order; TYP for any concrete type).
def case_add_baseline(a, b : int) : int {
    return a + b
}

def case_add_renamed(x, y : int) : int {
    return x + y
}

def case_add_typed_float(a, b : float) : float {
    return a + b
}


// ── Literal collapse ──────────────────────────────────────────────────
// All numeric/string literals collapse to LIT regardless of value.
def case_lit_one(a : int) : int {
    return a + 1
}

def case_lit_big(a : int) : int {
    return a + 999
}


// ── Call-name preservation ────────────────────────────────────────────
// User identifier names collapse, but called function names are kept —
// CALL:push vs CALL:reserve is real signal we want to surface.
def case_call_push(var arr : array<int>) {
    arr |> push(1)
}

def case_call_reserve(var arr : array<int>) {
    arr |> reserve(1)
}


// ── Field access (.FLD) ───────────────────────────────────────────────
struct CanonField {
    value : int
}

def case_field_read(s : CanonField) : int {
    return s.value
}


// ── Swizzle (.SWZ) ────────────────────────────────────────────────────
def case_swizzle(v : float4) : float2 {
    return v.xy
}


// ── While vs for ──────────────────────────────────────────────────────
// for and while emit distinct loop markers (FOR vs WHILE).
def case_while_loop(start : int) : int {
    var x = start
    while (x > 0) {
        x--
    }
    return x
}

def case_for_loop(arr : array<int>) : int {
    var n = 0
    for (v in arr) {
        n = v
    }
    return n
}


// ── Lambda body (MK_BLK) ──────────────────────────────────────────────
def case_lambda_use(var arr : array<int>) {
    arr |> sort() <| $(a, b : int) => a < b
}


// ── let / var (LET_VAR) ───────────────────────────────────────────────
// Initialize from a parameter so the optimizer can't fold the local
// away — we want LET_VAR to appear in the canonical stream. The
// canonical visitor emits LET_VAR for both `let` and `var` locals.
def case_let_var(seed : int) : int {
    let x = seed
    return x + 1
}
