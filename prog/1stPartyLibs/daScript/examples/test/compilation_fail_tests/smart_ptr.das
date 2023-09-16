expect 31300:1

require UnitTest

def pass_by_value_is_unsafe ( p:smart_ptr<TestObjectSmart> )    // this will fail if enabled in CodeOfPolicies
    pass

def move_smart_ptr_is_unsafe_when_not_ref
    var a : smart_ptr<TestObjectSmart>
    a <- new TestObjectSmart

[export]
def test
    return true
