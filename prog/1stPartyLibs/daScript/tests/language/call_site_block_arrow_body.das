options gen2
require dastest/testing_boost public

def apply_to_7(a : block<(x : int) : int>) : int {
    return a(7)
}

def build_with_7(a : block<(x : int) : array<int>>) : array<int> {
    return <- a(7)
}

[test]
def test_call_site_block_arrow_body(t : T?) {
    t |> run("call-site block arrow value form") @(t : T?) {
        let res = apply_to_7() $(x) => x * 2
        t |> equal(res, 14)
    }

    t |> run("call-site block arrow move form") @(t : T?) {
        let xs <- build_with_7() $(x) => <- [x, x + 1]
        t |> equal(length(xs), 2)
        t |> equal(xs[0], 7)
        t |> equal(xs[1], 8)
    }

    t |> run("braced call-site block still works") @(t : T?) {
        let res = apply_to_7() $(x) {
            return x + 5
        }
        t |> equal(res, 12)
    }

    t |> run("pipe-based block arrow still works") @(t : T?) {
        let res = apply_to_7() <| $(x) => x - 3
        t |> equal(res, 4)
    }
}
