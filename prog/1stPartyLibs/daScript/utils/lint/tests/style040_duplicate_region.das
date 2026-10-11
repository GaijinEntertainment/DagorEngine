options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _dupe_min_nodes = 16            // keep fixture functions small
// STYLE040: a run of statements duplicated elsewhere in the module
//
// Problem: the same run of statements appears in two (or more) places and a
// helper function could absorb it verbatim. Detection Merkle-hashes every AST
// node, then verifies each candidate pair by an exact lockstep walk under a
// variable bijection - so names may differ, but structure, types, called
// functions and literals must match exactly.
//
// The rule only fires when the extraction is mechanically valid: no escaping
// local, no return / break out of the region, no `assume` alias, and any
// written free variable must propagate through a parameter.
//
// Suppress a deliberate repetition with '// nolint:STYLE040'.

expect 31209:1

require daslib/style_lint

// --- the hit: same four statements, different variable names ------------------

def bad_alpha(var acc : array<int>; base : int) : int {   // STYLE040 reported here
    acc |> push(base * 2)
    acc |> push(base + 7)
    acc |> push(base - 1)
    acc |> push(base * base)
    return length(acc)
}

def bad_beta(var out : array<int>; seed : int) : int {
    out |> push(seed * 2)
    out |> push(seed + 7)
    out |> push(seed - 1)
    out |> push(seed * seed)
    return 0
}

// a third occurrence merges into the SAME finding rather than opening a new one, and the
// message must name both other sites - a strict-containment bug once truncated this to one
def bad_gamma(var dst : array<int>; k : int) : int {
    dst |> push(k * 2)
    dst |> push(k + 7)
    dst |> push(k - 1)
    dst |> push(k * k)
    return 2
}

// --- negative: one literal differs, so it is NOT the same code ---------------

def near_miss(var out : array<int>; seed : int) : int {
    out |> push(seed * 2)
    out |> push(seed + 8)
    out |> push(seed - 1)
    out |> push(seed * seed)
    return 1
}

// --- negative: the run declares locals that are read after it ----------------

def escapes_one(value : int) : int {
    let doubled = value * 2
    let shifted = doubled + 11
    return doubled + shifted
}

def escapes_two(other : int) : int {
    let doubled = other * 2
    let shifted = doubled + 11
    return shifted - doubled
}

// --- negative: a write to a free int would not survive the call --------------

def writes_scalar(value : int) : int {
    var total = 0
    total += value * 3
    total += value * 5
    total += value * 7
    return total
}

def writes_scalar_too(other : int) : int {
    var total = 0
    total += other * 3
    total += other * 5
    total += other * 7
    return total
}
