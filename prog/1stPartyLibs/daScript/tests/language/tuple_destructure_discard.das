options gen2
require dastest/testing_boost public

// `_` is the discard in tuple destructuring: it binds nothing, so it can repeat -
// within one pattern, across patterns, and in for-loop destructures - while every
// real name still follows the `let` shadowing rules.

def two_of_three() : tuple<int; float; string> {
    return (1, 2.0, "three")
}

[test]
def test_tuple_destructure_discard(t : T?) {
    t |> run("discard repeats freely") @(t : T?) {
        let (_, _, third) = two_of_three()
        t |> equal("three", third)
        let (first, _, _) = two_of_three()
        t |> equal(1, first)
        let (_, mid, _) = two_of_three()
        t |> equal(2.0, mid)
    }
    t |> run("discard repeats across for-loop destructures") @(t : T?) {
        var pairs <- [(1, "one"), (2, "two")]
        var total = 0
        for ((k, _) in pairs) {
            total += k
        }
        for ((_, v) in pairs) {
            total += length(v)
        }
        t |> equal(9, total)
    }
}
