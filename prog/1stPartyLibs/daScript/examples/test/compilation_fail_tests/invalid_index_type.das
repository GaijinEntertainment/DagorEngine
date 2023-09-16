expect 30110:3, 30106    // invalid_index_type

def test
    let a:int[10]
    a["blah"]   // index is int or uint
    a[0]        // this one ok
    let b:array<string>
    b["blah"]   // index is int or uint
    b[0]        // this one ok
    let c:table<string;int>
    c["blah"]   // 30106: can't index in the constant table, use find instead
    c[0]        // index type mismatch
