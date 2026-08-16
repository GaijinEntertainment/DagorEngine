// wrong index type for containers
// verifies compiler rejects non-integer indices for fixed arrays,
// dynamic arrays, and constant table indexing
options gen2
expect 30184:2, 30251, 30934    // invalid_index_type

def test {
    let a : int[10]
    a["blah"]   // index is int, int64, uint, or uint64
    a[0]        // this one ok
    let b : array<string>
    b["blah"]   // index is int, int64, uint, or uint64
    b[0]        // this one ok
    let c : table<string; int>
    c["blah"]   // 30106: can't index in the constant table, use find instead
    c[0]        // index type mismatch
}
