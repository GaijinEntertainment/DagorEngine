// Verifies _abstract and inherited bits on Structure::FieldDeclaration for
// abstract class methods through clone/override.
//
// - Foo.add: declared `def abstract` here   -> _abstract=true,  inherited=false
// - Bar.add: cloned from Foo, no redeclare  -> _abstract=true,  inherited=true
// - Far.add: overridden with a body         -> _abstract=false, inherited=false
//
// The [check_add_field_flags] structure_macro asserts the bits at compile time.
// A bit mismatch becomes a macro_error and fails the compile, so successful
// compilation = passing test. The runtime [test] confirms the override chain
// dispatches correctly.
options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require _field_decl_flags_helper

[check_add_field_flags(is_abstract = true, is_inherited = false)]
class Foo {
    def abstract add(a, b : int) : int
}

[check_add_field_flags(is_abstract = true, is_inherited = true)]
class Bar : Foo {
}

[check_add_field_flags(is_abstract = false, is_inherited = false)]
class Far : Bar {
    def override add(a, b : int) : int {
        return a + b
    }
}

[test]
def test_far_overrides_abstract(t : T?) {
    var f = new Far()
    t |> equal(f.add(2, 3), 5)
    unsafe {
        delete f
    }
}
