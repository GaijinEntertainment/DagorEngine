// uncomment any of the blah to see where error is
// blah
// inlcuding that in include_file.inc

include include_file.das_inc

include include_file.das_inc

[export]
def test
    let y = inc_add_2(3)
    assert(y==5)    // blah
    return true

