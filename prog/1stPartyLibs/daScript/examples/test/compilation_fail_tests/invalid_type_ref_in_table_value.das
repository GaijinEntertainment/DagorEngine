expect 30106    // invalid_table_type

def test
    let a:table<int;string&>   // can't have table value ref
    debug(a)



