// ref type as table value
// verifies compiler rejects reference types in table values
options gen2
expect 30248    // invalid_table_type

def test {
    let a : table<int; string&>// can't have table value ref
    debug(a)
}



