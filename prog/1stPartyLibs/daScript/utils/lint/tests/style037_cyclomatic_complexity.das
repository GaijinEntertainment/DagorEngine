options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _cyclomatic_complexity = 5      // keep fixture functions small
// STYLE037: function or closure cyclomatic complexity exceeds the limit
//
// Problem: a function with too many independent decision points is hard to
// read, test, and modify. The score starts at 1 per function (or closure) and
// adds 1 for every if/elif (including postfix 'return X if (cond)'), for,
// while, ternary '?:', try/recover, match arm, and comprehension loop/where.
//
// NOT counted: '&&' / '||', '??', '?.', '?[', '?as', 'static_if', and
// macro-generated control flow. Lambda/generator bodies are lowered to
// generated functions before the lint pass, so they are not checked
// (pre-existing style-lint behavior); block arguments are.
//
// Override the default limit per module with
// 'options _cyclomatic_complexity = N' (this fixture uses 5); 0 disables the
// rule for the module (see style037_cyclomatic_complexity_off.das).
// Suppress a deliberate keep with '// nolint:STYLE037' on the def line.

expect 31209:9

require daslib/style_lint
require daslib/match

struct Foo {
    v : int = 0
}

// --- Bad patterns (must fire — 8 total) ---

// 5 ifs -> complexity 6
def bad_if_chain(x : int) : int {                       // STYLE037
    if (x == 1) return 10
    if (x == 2) return 20
    if (x == 3) return 30
    if (x == 4) return 40
    if (x == 5) return 50
    return 0
}

// if + 4 elifs -> complexity 6 (each elif is its own decision)
def bad_elif_chain(x : int) : int {                     // STYLE037
    var r = 0
    if (x == 1) {
        r = 10
    } elif (x == 2) {
        r = 20
    } elif (x == 3) {
        r = 30
    } elif (x == 4) {
        r = 40
    } elif (x == 5) {
        r = 50
    }
    return r
}

// for + while + nested for + 2 ternaries -> complexity 6
def bad_loops_ternary(src : array<int>) : int {         // STYLE037
    var total = 0
    for (x in src) {
        total += x
    }
    var i = 0
    while (i < 10) {
        i++
        for (j in range(i)) {
            total += j
        }
    }
    total = total > 100 ? total - 1 : total + 1
    total = total < 0 ? 0 : total
    return total
}

// 5 postfix conditionals desugar to ExprIfThenElse -> complexity 6
def bad_postfix_ifs(x : int) : int {                    // STYLE037
    return 10 if (x == 1)
    return 20 if (x == 2)
    return 30 if (x == 3)
    return 40 if (x == 4)
    return 50 if (x == 5)
    return 0
}

// try/recover + 4 ifs -> complexity 6
def bad_recover(x : int) : int {                        // STYLE037
    var r = 0
    try {
        if (x == 1) {
            r = 10
        }
        if (x == 2) {
            r = 20
        }
        if (x == 3) {
            r = 30
        }
        if (x == 4) {
            r = 40
        }
    } recover {
        r = -1
    }
    return r
}

// match arms expand to plain if-chains -> each arm is a decision
def bad_match(x : int) : int {                          // STYLE037
    match (x) {
        if (1) {
            return 10
        }
        if (2) {
            return 20
        }
        if (3) {
            return 30
        }
        if (4) {
            return 40
        }
        if (5) {
            return 50
        }
        if (6) {
            return 60
        }
    }
    return 0
}

// trivial host, complexity-6 block argument -> exactly one warning, at the closure
def bad_closure(x : int) : int {
    var r = 0
    with_it(x) $(y) {                                   // STYLE037 (closure)
        if (y == 1) { r = 10; }
        if (y == 2) { r = 20; }
        if (y == 3) { r = 30; }
        if (y == 4) { r = 40; }
        if (y == 5) { r = 50; }
    }
    return r
}

def with_it(x : int; blk : block<(y : int) : void>) {
    invoke(blk, x)
}

// comprehension loop + where filter each count: 3 x 2 -> complexity 7
// (counted the same whether or not the optimizer inlines the lowering)
def bad_comprehension_heavy(src : array<int>) : int {   // STYLE037
    var a <- [for (x in src); x * x; where x > 0]
    var b <- [for (x in src); x + 1; where x < 0]
    var c <- [for (x in src); x - 1; where x != 0]
    return length(a) + length(b) + length(c)
}

// generic instanced twice -> exactly one warning (location dedup)
def bad_generic(x) : int {                              // STYLE037
    if (x > type_zero(x)) return 1
    if (x < type_zero(x)) return 2
    if (x == type_zero(x)) return 3
    if (x != type_zero(x)) return 4
    if (x >= type_zero(x)) return 5
    return 0
}

def type_zero(x) => default<typedecl(x)>

[export]
def use_bad_generic() : int {
    return bad_generic(1) + bad_generic(1.0)
}

// --- Good patterns (must stay silent) ---

// exactly at the limit: 4 ifs -> complexity 5, not above
def good_at_threshold(x : int) : int {
    if (x == 1) return 10
    if (x == 2) return 20
    if (x == 3) return 30
    if (x == 4) return 40
    return 0
}

// '&&' / '||' are not decision points
def good_short_circuit(x, y, z : int) : bool {
    if (x > 0 && y > 0 && z > 0 && x < 100 && y < 100 && z < 100) {
        return x + y + z > 10 || x * y * z > 1000 || x - y - z < -10
    }
    return false
}

// '??' / '?.' null-safe chains are not decision points
def good_null_chains(p : Foo?; q : Foo?) : int {
    var total = p?.v ?? 0
    total += q?.v ?? 1
    total += p?.v ?? 2
    total += q?.v ?? 3
    total += p?.v ?? 4
    total += q?.v ?? 5
    return total
}

// lambda bodies lower to generated functions before lint and are not checked
def lambda_body_not_checked(x : int) : int {
    let f = @(y : int) : int {
        if (y == 1) return 10
        if (y == 2) return 20
        if (y == 3) return 30
        if (y == 4) return 40
        if (y == 5) return 50
        return 0
    }
    return invoke(f, x)
}

// deliberate keep, suppressed on the def line
def suppressed_hairy(x : int) : int {                   // nolint:STYLE037
    if (x == 1) return 10
    if (x == 2) return 20
    if (x == 3) return 30
    if (x == 4) return 40
    if (x == 5) return 50
    return 0
}
