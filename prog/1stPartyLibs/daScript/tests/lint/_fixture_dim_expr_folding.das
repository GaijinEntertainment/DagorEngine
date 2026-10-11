options gen2

require _dim_expr_folding_consts

let ROWS = 3
let SLOTS = LANES + 2

// Nothing demands a constant of this one, so its initializer stays whole and
// its redundant cast stays visible to lint.  The only PERF020 site here.
let UNDEMANDED = int(5)

// Undemanded as well, so both initializers stay arithmetic -- the shape a backend reads
// off a global, and the one only the compiler can put a value to.  Nothing else reads
// STEP, so a fold recorded against it can only have come from asking.
let STEP = 2 + 2
let SPAN = ROWS * STEP

// A vector constructor is a call, so this one folds through neither the operators nor
// an already-constant node -- the shape a shader spells a basis matrix's column in.
//! a multiply, not a divide: a -ffast-math host divides a vector by a runtime splat through rcpps, one ULP off
let VEC_HALF = float4(1.0, 2.0, 3.0, 4.0) * 0.5

// A flat sum nests one operator per term, so this one is deeper than the bound that
// stops a cycle of consts defined off each other.  Depth in the AST is not a cycle.
let DEEP_SUM = (1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1
    + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1
    + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1
    + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1)

// Every dimension here needs a fold the lint policies switch off: an
// expression at either end of a nested one, a const whose own initializer
// is arithmetic, and one that reads that const across a module.
struct Grid {
    cells : int[ROWS + 1][2]
}

struct Stack {
    slots : int[2][ROWS * 2]
}

struct Lanes {
    own : int[SLOTS]
    across : int[LANES + 1]
}

// A CALL in a dimension, at either end of a nested one.  An operator chain the
// old dim-site fold already reached; a call it did not, so this is the shape
// that says the fold reaches a constructor at all -- and the one that says the
// force is restored, since the inner dimension is folded under the same flag.
struct Called {
    outer : int[int(4.9) + 1][2]
    inner : int[2][int(4.9) + 1]
}

[export]
def main {
    let g : Grid
    let s : Stack
    let l : Lanes
    let c : Called
    print("{length(g.cells)} {length(g.cells[0])}\n")
    print("{length(s.slots)} {length(s.slots[0])}\n")
    print("{length(l.own)} {length(l.across)} {UNDEMANDED} {SPAN}\n")
    print("{length(c.outer)} {length(c.outer[0])} {length(c.inner)} {length(c.inner[0])}\n")
}
