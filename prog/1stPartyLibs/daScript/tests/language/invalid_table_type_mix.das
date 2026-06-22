// invalid table key/value types
// verifies compiler rejects ref keys, non-hashable keys (array),
// and void local variables
options gen2
expect 30202, 30248, 30254    // invalid_table_type

def test {
    let a : table<int&; string>        // can't have table key ref
    debug(a)
    let b : table<array<int>; string>  // key has to be basic hashable type
    debug(b)
    let c : void                       // 30108: local variable can't be void
    debug(c)
}

