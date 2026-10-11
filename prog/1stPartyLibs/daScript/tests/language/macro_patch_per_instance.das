options gen2
options no_unused_function_arguments = false

require dastest/testing_boost public
require _helper_double_patch public

[count_patch(tag = "first"), count_patch(tag = "second")]
def probe_twice() {
    feint("probe\n")
}

[test]
def test_each_annotation_instance_patches_once(t : T?) {
    t |> run("two [count_patch] on one function both patch") @(t : T?) {
        double_patch_marks = 0
        probe_twice()
        t |> equal(double_patch_marks, 2)
    }
}
