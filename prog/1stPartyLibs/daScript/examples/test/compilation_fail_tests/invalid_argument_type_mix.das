options optimize=false

expect 30103:4   // invalid_argument_type

def test
    // assert(bool,comment)
    assert(1,"sure")            // 30103,   can only assert on boolean expressions
    assert(true,"blah")         // this one fine
    assert(true,1)              // even this is not a string constant
    let a:string="comment"
    assert(true,a)          // even this is not a string constant
    debug(0,a)              // 30103,   not for debug either
    debug(a,"blah")         // this one good


