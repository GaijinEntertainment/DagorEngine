// non-bool condition errors
// verifies compiler rejects non-bool conditions in if/ternary,
// type mismatches in ternary branches, and writes to non-references
options gen2
expect 30183, 30411, 30915, 31401  // condition_must_be_boolean

def test {
    var a : int
    var b : float
    if (a) {// not bool
        b = 1.0
    }
    a ? 1.0 : 2.0       // not bool either
    true ? a : b        // not the same type
    true ? a : 1 = 0    // can only copy to reference
}

