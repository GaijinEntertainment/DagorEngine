options gen2
require dastest/testing_boost public
require daslib/fuzzer
require daslib/faker

[test]
def test_bit_counting_everything(t : T?) {
    t |> run("clz,ctz 32") @(t : T?) {
        for (i in urange(32)) {
            let bits = 1u << i
            let lz = clz(bits)
            let tz = ctz(bits)
            t |> equal(31u, lz + tz)
            t |> equal(bits, 1u << tz)
            t |> equal(bits, 0x80000000u >> lz)
            t |> equal(1u, popcnt(bits))
        }
    }
    t |> run("clz,ctz 64") @(t : T?) {
        for (i in urange(64)) {
            let bits = 1ul << uint64(i)
            let lz = clz(bits)
            let tz = ctz(bits)
            t |> equal(63ul, lz + tz)
            t |> equal(bits, 1ul << tz)
            t |> equal(bits, 0x8000000000000000ul >> lz)
            t |> equal(1ul, popcnt(bits))
        }
    }
    t |> run("popcnt") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            var x = fake |> random_uint()
            var bits = x
            var count = 0u
            while (bits != 0u) {
                bits &= bits - 1u
                count ++
            }
            t |> equal(count, popcnt(x))
        }
    }
    t |> run("popcnt64") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            var x = fake |> random_uint64()
            var bits = x
            var count = 0ul
            while (bits != 0ul) {
                bits &= bits - 1ul
                count ++
            }
            t |> equal(count, popcnt(x))
        }
    }
}
