expect 30106:2, 30108    // invalid_table_type

def test
    let a:table<int&;string>           // can't have table key ref
    debug(a)
    let b:table<array<int>;string>     // key has to be basic hashable type
    debug(b)
    let c:void                         // 30108: local variable can't be void
    debug(c)

