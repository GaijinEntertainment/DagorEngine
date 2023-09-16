// note: AOT is disabled for this sample
//  this is due to the fact, that in AOT mode assert does not cause panic, and does C++ assert instead
options no_aot = true

require daslib/assert_once

[export]
def test
    var t = 0
    var count = 0
    label 0:
    try
        assert_once(t==0)
    recover
        count ++
    t ++
    if t != 5
        goto 0
    assert(count==1)
    return true

