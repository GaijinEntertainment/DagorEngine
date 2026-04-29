// wrong argument count in function calls
// verifies compiler rejects calling assert and debug with
// too few or too many arguments
options gen2
expect 30107:4    // invalid_argument_count

def test {
    // assert 1 or 2 arguments
    assert()
    assert(false, "blah", 1)
    // debug 1 or 2 arguments
    debug()
    debug(0, "blah", 1, "")
}


