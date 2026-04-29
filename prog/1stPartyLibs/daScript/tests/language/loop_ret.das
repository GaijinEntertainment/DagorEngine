options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/random

var lr_seed : int4

typedef LrFoo = tuple<int; float3>

def lr_foo(var attached_fires : array<LrFoo>) {
    let tf = random_int(lr_seed) % 100
    for (x in attached_fires) {
        if (x._0 == tf) {
            return
        }
    }
    push(attached_fires, (tf, float3(0)))
}

[test]
def test_loop_ret(t : T?) {
    t |> run("loop early return fills unique entries") @(t : T?) {
        var a : array<LrFoo>
        for (x in range(1000)) {
            lr_foo(a)
        }
        t |> equal(length(a), 100)
    }
}
