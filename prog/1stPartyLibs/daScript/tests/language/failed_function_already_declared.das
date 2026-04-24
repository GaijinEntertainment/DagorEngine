// duplicate function declaration
// verifies compiler rejects two functions with identical signatures
options gen2
expect 30201   // function_already_declared

def test {
    assert(false)
}

def test {
    assert(false)
}

