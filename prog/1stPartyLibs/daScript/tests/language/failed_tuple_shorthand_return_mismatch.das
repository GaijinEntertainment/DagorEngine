options gen2
expect 30343    // incompatible return type: shorthand var names don't match declared named-tuple result

require dastest/testing_boost public

def fail_return_name_mismatch() : tuple<eid : int; distSq : float> {
    let foo = 1
    let bar = 1.1
    return (foo, bar)
}
