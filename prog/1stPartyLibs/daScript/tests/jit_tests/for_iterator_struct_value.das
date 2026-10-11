options gen2
options jit_opt_level = 0
require dastest/testing_boost public

// a struct yielded BY VALUE from a lambda iterator: the loop slot is the struct itself,
// not a reference, so its zero-init must be struct-sized - an 8-byte memset on a 4-byte
// slot spills into whatever the O0 frame packs next to it
struct Small {
    v : int
}

[test]
def test_struct_value_loop_variable_keeps_neighbours(t : T?) {
    t |> run("by-value struct from each(lambda)") @(t : T?) {
        var count = 0     // nolint:LINT003 - the lambda's captured copy would be const under let
        let gen <- @(var s : Small) : bool {
            if (count < 6) {
                s.v = count++
                return true
            }
            return false
        }
        let sentinel_a = 0x11111111
        let sentinel_b = 0x22222222
        var acc = 0
        for (y, i in each(gen), range(6)) {
            let sentinel_c = 0x33333333
            t |> equal(y.v, i)
            acc += y.v
            t |> equal(sentinel_c, 0x33333333)
        }
        t |> equal(acc, 15)
        t |> equal(sentinel_a, 0x11111111)
        t |> equal(sentinel_b, 0x22222222)
    }
}
