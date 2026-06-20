// Regression test: apply_imm always inlines, so it has no invoke fallback and cannot accept a block
// that uses `return` to skip a field (an inlined bare `return` would escape the caller). The macro
// rejects it with 50503; the unresolved call leaves the block arg uninferred (cascade 30303). Use
// `apply` for a return-block — it falls back to the invoke codegen where `return` stays block-local.
options gen2
expect 50503, 30303

require daslib/apply

struct Triple {
    a : int
    b : int
    c : int
}

def sum_skip_b(tr : Triple) : int {
    var sum = 0
    apply_imm(tr) $(name : string; field) {
        if (name == "b") return
        sum += field
    }
    return sum
}

[export]
def main {
    print("{sum_skip_b(Triple(a = 1, b = 2, c = 3))}\n")
}
