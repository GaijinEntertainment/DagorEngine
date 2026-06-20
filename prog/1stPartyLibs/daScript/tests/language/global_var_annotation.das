options gen2

require dastest/testing_boost public
require _global_var_annotation_helper

// Regression test: parser must propagate the `@name` field annotation onto
// the resulting Variable for BOTH the single-form and block-form global
// declarations. Earlier, ast_globalLetList (the var{...} block path) dropped
// the annotation on the floor — only the single-form path attached it.
// The CheckColorAnnotation lint_macro fires at compile time and fails the
// build via macro_error if any `assert_color_*` global is missing @color.

// single-form
var @color assert_color_single = 1

// block-form (the previously broken path)
var {
    @color assert_color_block_a = 2
    @color assert_color_block_b = 3
}

[test]
def test_global_var_annotation(t : T?) {
    t |> run("globals carry their field annotations") @(t : T?) {
        // If this file compiles, the lint_macro's macro_error did not fire,
        // which means all assert_color_* globals had @color attached.
        t |> equal(assert_color_single, 1)
        t |> equal(assert_color_block_a, 2)
        t |> equal(assert_color_block_b, 3)
    }
}
