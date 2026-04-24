options gen2
// test for circular assume detection
expect 30127:2, 30305:4

def test_direct_loop {
    assume x = y + y
    assume y = x + x
    print("x = {x}, y = {y}\n")
}

def test_transitive_loop {
    assume a = b + 1
    assume b = c + 1
    assume c = a + 1
    print("a = {a}\n")
}
