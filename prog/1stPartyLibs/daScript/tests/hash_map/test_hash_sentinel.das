options gen2
require dastest/testing_boost public

require daslib/cuckoo_hash_table public

[test]
def test_cuckoo_hash_never_returns_the_empty_marker(t : T?) {
    var zeros = 0
    for (i in range(0, 65536)) {
        if (hash0(i) == 0ul) {
            zeros ++
        }
        if (hash_extra(i) == 0ul) {
            zeros ++
        }
    }
    t |> equal(zeros, 0, "0 marks an empty slot, so neither cuckoo hash may return it")
}

[test]
def test_hash_extra_rotates_rather_than_shifts(t : T?) {
    for (i in range(0, 256)) {
        let h = hash(i)
        let rotated = (h >> 32ul) | (h << 32ul)
        t |> equal(hash_extra(i), rotated, "hash_extra keeps the low 32 bits, so a hash that fits in 32 cannot collapse to 0")
    }
}
