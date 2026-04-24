// unsafe smart_ptr move via <-
// verifies compiler rejects direct <- move on smart_ptr values,
// which requires move() or move_new() instead
options gen2
expect 31300:1

require daslib/ast

def pass_by_value_is_unsafe(p : smart_ptr<TypeDecl>) {// this will fail if enabled in CodeOfPolicies
    pass
}

def move_smart_ptr_is_unsafe_when_not_ref {
    var inscope a : smart_ptr<TypeDecl>
    a <- new TypeDecl(baseType = Type.autoinfer) // 31300: moving values which contain smart pointers is unsafe
}

[export]
def test {
    return true
}
