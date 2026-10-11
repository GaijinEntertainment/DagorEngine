options gen2
require dastest/testing_boost public

[export]
def zero_function_ref() => type <function&>

[test]
def test_ref_make_local(t : T?) {
    t |> success(zero_function_ref() == null, "zeroed function reference")
}
