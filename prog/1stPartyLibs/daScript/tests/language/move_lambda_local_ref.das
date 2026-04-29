options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct MoveLamFoo {
    bar : lambda<() : array<int>>
}

def MakeBar(var arr : array<int>) {
    return @ capture(<- arr) () {
        return <- clone_to_move(arr)
    }
}

let {
    g_move_lam_foo = new MoveLamFoo(
        bar <- MakeBar([1, 2, 3])
    )
}

[test]
def test_move_lambda_local_ref(t : T?) {
    t |> run("capture move array into lambda") @(t : T?) {
        let l <- invoke(g_move_lam_foo.bar)
        t |> equal(l[0], 1)
        t |> equal(l[1], 2)
        t |> equal(l[2], 3)
    }
}
