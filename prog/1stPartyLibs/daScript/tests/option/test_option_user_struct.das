options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/option
require _test_option_user_struct_mod

// Regression for feedback_option_cross_module_template_bug.md (splitTypeName
// must be bracket-aware so Option<_mod::UserStruct> doesn't split at the
// inner ::). With Option as a structural tuple (typemacro_function), tuples
// are not module-keyed, so this should be even more robust — the test exists
// to prove that conclusion under future refactors.

[test]
def cross_module_option_some(t : T?) {
    t |> run("Option<Foo> across module boundary — some path") @(t : T?) {
        let hit = lookup_foo(7)
        t |> success(hit |> is_some)
        let v = hit |> unwrap
        t |> equal(v.id, 7)
        t |> equal(v.name, "name-7")
    }
}

[test]
def cross_module_option_none(t : T?) {
    t |> run("Option<Foo> across module boundary — none path") @(t : T?) {
        let miss = lookup_foo(0)
        t |> success(miss |> is_none)
    }
}

[test]
def cross_module_option_equality(t : T?) {
    t |> run("Option<Foo> structural equality across module boundary") @(t : T?) {
        t |> success(lookup_foo(3) == some(make_foo(3, "name-3")))
        t |> success(!(lookup_foo(0) == some(make_foo(3, "name-3"))))
        t |> success(!(lookup_foo(3) == some(make_foo(4, "name-4"))))
    }
}
