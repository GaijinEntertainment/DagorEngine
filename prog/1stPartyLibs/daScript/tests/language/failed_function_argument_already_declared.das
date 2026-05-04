// duplicate function argument name
// verifies compiler rejects a function with two arguments of the same name
options gen2
expect 30202    // function_argument_already_declared

def test(a, a : int) {
    debug(a)
}
