options gen2
require dastest/testing_boost public
require _lambda_vis_mid

// A generic instance resolves unqualified calls from its HOME module (where the generic
// was written). Functions outlined from blocks inside that instance — lambdas, generators,
// local functions — must inherit that scope: this file never requires _lambda_vis_inner,
// so inner_dot is reachable only through _lambda_vis_mid's own require.
// Regression: lifted lambdas/generators used to re-resolve from the instantiation module
// (error 30341 "module not visible directly"); local functions carried the fix alone.

[test]
def test_lambda_in_generic_module_vis(t : T?) {
    t |> run("direct call in generic resolves from home module") @(t : T?) {
        t |> equal(9.0, vis_direct(4))
    }
    t |> run("lambda in generic resolves from home module") @(t : T?) {
        t |> equal(13.0, vis_lambda(4))
    }
    t |> run("generator in generic resolves from home module") @(t : T?) {
        t |> equal(17.0, vis_generator(4))
    }
    t |> run("local function in generic resolves from home module") @(t : T?) {
        t |> equal(21.0, vis_local_function(4))
    }
}
