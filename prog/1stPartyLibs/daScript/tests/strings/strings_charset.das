options gen2
require dastest/testing_boost
require daslib/strings_boost
require daslib/faker
require daslib/fuzzer

[test]
def test_charset(t : T?) {
    t |> run("charset") @@(t : T?) {
        var ch : uint[8]
        t |> success(!is_char_in_set('A', ch))
        ch['A' / 32] |= 1u << uint('A' & 31)
        t |> success(is_char_in_set('A', ch))
        t |> equal(set_total(ch), 1u)
        t |> equal(set_element(0, ch), 'A')
    }
    t |> run("fuzz charset") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let ch = fake |> any_set
            var total = 0u
            for (c in ch) {
                var bits = c
                while (bits != 0u) {
                    bits &= bits - 1u
                    total ++
                }
            }
            let stotal = set_total(ch)
            t |> equal(total, stotal)
            var elem = 0
            for (i, c in urange(8), ch) {
                let bits = c
                for (j in urange(32)) {
                    if ((bits & (1u << j)) != 0u) {
                        let Char = int(i * 32u + j)
                        t |> success(is_char_in_set(Char, ch))
                        let EChar = set_element(elem, ch)
                        t |> equal(Char, EChar)
                        elem ++
                    }
                }
            }
        }
    }
    t |> run("fuzz set_element") @@(t : T?) {
        var fake <- Faker()
        var count = 0
        fuzz(1000) {
            let ch = fake |> any_set
            let total = int(set_total(ch))
            let idx = fake |> random_int
            let elem = set_element(idx, ch)
            t |> success(elem == -1 || (elem >= 0 && elem < total))
            count ++
        }
        t |> equal(count, 1000)
    }
    t |> run("fuzz is_char_in_charset") @@(t : T?) {
        var fake <- Faker()
        fuzz() {
            let ch = fake |> any_set
            let total = int(set_total(ch))
            let idx = fake |> random_int
            t |> accept(is_char_in_set(idx, ch))
        }
    }
}

