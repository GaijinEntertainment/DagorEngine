options gen2

module _test_option_user_struct_mod shared public

require daslib/option public

struct public Foo {
    id : int
    name : string
}

def public make_foo(id : int; name : string) : Foo {
    return Foo(id = id, name = name)
}

def public operator ==(a, b : Foo) : bool {
    return a.id == b.id && a.name == b.name
}

// Cross-module Option<UserStruct> path. Exercises the bracket-aware
// splitTypeName fix from feedback_option_cross_module_template_bug, and
// guards against re-introducing it under the typemacro+tuple shape.
def public lookup_foo(id : int) : Option<Foo> {
    if (id <= 0) {
        return none(type<Foo>)
    }
    return some(make_foo(id, "name-{id}"))
}
