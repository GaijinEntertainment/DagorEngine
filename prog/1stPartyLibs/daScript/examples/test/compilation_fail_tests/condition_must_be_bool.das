expect 30601:2  // condition_must_be_boolean
expect 30303    // operator_not_found
expect 30506    // cant_write_to_non_reference

def test
    var a:int
    var b:float
    if a                // not bool
        b = 1.0
    a ? 1.0 : 2.0       // not bool either
    true ? a : b        // not the same type
    true ? a : 1 = 0    // can only copy to reference

