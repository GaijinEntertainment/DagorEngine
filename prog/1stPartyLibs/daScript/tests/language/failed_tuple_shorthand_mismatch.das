options gen2
expect 30341    // function_not_found: shorthand var names don't match target tuple field names

require dastest/testing_boost public

def fail_name_mismatch() {
    var arr : array<tuple<eid : int; distSq : float>>
    let foo = 1
    let bar = 1.1
    arr |> push((foo, bar))
}
