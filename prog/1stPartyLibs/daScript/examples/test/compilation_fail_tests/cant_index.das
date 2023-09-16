expect 30502:2  // cant_index

def test_index(a:int)
    a[1]        // not a ref (and not an array)

def test
    let a:int
    a[0]    // not an array
