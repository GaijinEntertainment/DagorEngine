options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

def iaa_fold(a : array<auto(numt)>; var from : numt) : numt {
    for (x in a) {
        from += x
    }
    return from
}

[test]
def test_infer_alias_argument(t : T?) {
    t |> run("fold with auto alias argument") @(t : T?) {
        var a : array<float>
        push(a, 1.0)
        push(a, 2.0)
        push(a, 3.0)
        t |> equal(iaa_fold(a, 13.0), 19.0)
    }
}
