options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct CloneTempFoo {
    a : int = 10
    s : string = "bar"
}

def test_clone_temp_c() : CloneTempFoo const# {
    unsafe {
        return struct<CloneTempFoo#>(uninitialized a = 4)
    }
}

[test]
def test_clone_temp(t : T?) {
    t |> run("clone temporary struct") @(t : T?) {
        let foo = test_clone_temp_c()
        let cop := foo
        t |> equal(foo.a, 4)
        t |> equal(cop.a, 4)
    }
}
