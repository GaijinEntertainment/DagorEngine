options gen2
require daslib/safe_addr

struct GameState {
    scores : table<string; table<int; int>>
    weights : table<int; float>
    counters : table<string; int>
}

def get_context() : GameState? {
    return unsafe(addr(GameState()))
}

// --- simple table: tab.insert(key, tab.get_value(key) OP expr) ---

[export]
def test_simple_table {
    var tab : table<int; int>
    var key = 1
    var val = 10

    // += pattern
    tab[key] += val

    // -= pattern
    tab[key] -= val

    // *= pattern
    tab[key] *= val

    // /= pattern
    tab[key] /= val

    // ++ pattern (add 1)
    tab[key] += 1

    // -- pattern (sub 1)
    tab[key] -= 1

    // reversed operand order: expr OP get_value
    tab[key] += val
    tab[key] *= val
}

// --- single-indexed table: tab[i].insert(key, tab[i].get_value(key) OP expr) ---

[export]
def test_indexed_table {
    var tabtab : table<int; table<int; int>>
    var idx = 1
    var scoreSrc = 0
    var score = 5

    // the original pattern from the question
    tabtab[1][scoreSrc] += score

    // variable index
    tabtab[idx][scoreSrc] += score

    // subtract
    tabtab[idx][scoreSrc] -= score

    // multiply
    tabtab[idx][scoreSrc] *= score

    // increment
    tabtab[idx][scoreSrc] += 1
}

// --- double-indexed: tab[i][j].insert(key, tab[i][j].get_value(key) OP expr) ---

[export]
def test_double_indexed {
    var deep : table<int; table<int; table<string; int>>>
    var i = 0
    var j = 1
    var key = "hits"
    var delta = 3

    deep[i][j][key] += delta
    deep[i][j][key] -= 1
}

// --- complex expression as table source ---

[export]
def test_complex_expr {
    var ctx = get_context()

    // method-like access: ctx.field.insert(...)
    ctx.counters["logins"] += 1
    ctx.counters["errors"] -= 1

    // chained field + index
    ctx.scores["team_a"][42] += 100

    // float table
    ctx.weights[7] += 0.5
    ctx.weights[7] *= 2.0
}

// --- pipe syntax: tab |> insert(key, tab |> get_value(key) OP expr) ---

[export]
def test_pipe_syntax {
    var tab : table<int; int>
    var key = 1
    var val = 10

    tab[key] += val
    tab[key] -= val
}

// --- free-function syntax: insert(tab, key, get_value(tab, key) OP expr) ---

[export]
def test_free_function {
    var tab : table<int; int>
    var key = 1
    var val = 10

    tab[key] += val
    tab[key] -= val
    tab[key] *= val
}

// --- compound expressions: get_value result used in larger expression ---

def foo() : int {
    return 42
}

[export]
def test_compound_expr {
    var tab : table<int; int>
    var key = 1
    var val = 10

    // get_value + literal + call
    tab[key] += 1 + foo()

    // get_value + multiple terms
    tab[key] += val + foo() + 3

    // get_value multiplied by expression
    tab[key] *= (val + foo())

    // get_value in subtraction with compound rhs
    tab[key] -= val * 2

    // get_value with ternary-like: conditional add
    tab[key] += (val > 5 ? 1 : 0)

    // deeply nested: tab[i].insert with compound expr
    var tabtab : table<int; table<int; int>>
    tabtab[0][key] += val * foo()
    tabtab[0][key] += foo() - val

    // get_value is not the leftmost operand — buried in expression
    tab.insert(key, foo() + tab.get_value(key) + val)
    tab[key] += val * 2

    // bitwise ops
    tab[key] |= val
    tab[key] &= int(0xFF)
    tab[key] ^= val
    tab[key] <<= 1
    tab[key] >>= 2
}

// --- negative: patterns that should NOT match ---

[export]
def test_negative {
    var tab : table<int; int>
    var other : table<int; int>
    var key = 1
    var val = 10

    // different tables — NOT foldable
    tab.insert(key, other.get_value(key) + val)

    // different keys — NOT foldable
    tab.insert(key, tab.get_value(key + 1) + val)

    // no arithmetic — just reassign from get_value (this is just insert(k, get_value(k)), a no-op copy)
    tab.insert(key, tab.get_value(key))

    // completely unrelated insert
    tab.insert(key, val)
}
