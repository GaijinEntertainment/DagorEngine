// A mismatched const initializer must not be substituted into expressions as
// though it had the declared type while the compiler is collecting errors.
options gen2
expect 30344

def f {
    let x : string = 2
    var z = x + x
}
