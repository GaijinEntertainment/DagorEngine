// unknown type name
// verifies compiler rejects using an undefined type name
options gen2
expect 30301    // type_not_found

def test {
    let a : wtf
    debug(a)
}
