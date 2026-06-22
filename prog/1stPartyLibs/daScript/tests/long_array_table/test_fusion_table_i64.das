options gen2
require dastest/testing_boost public

// Slice B audit: int64/uint64-keyed table indexing already has fusion variants
// registered via IMPLEMENT_SETOP_NUMERIC(TableIndex) at
// src/simulate/simulate_fusion_tableindex.cpp:81. These tests verify the
// fused path produces correct values for int64/uint64 keys.

[test]
def test_table_i64_key_index_write_read(t : T?) {
    var tab : table<int64; int>
    tab[1_l] = 10
    tab[1000000000000_l] = 20
    tab[-1_l] = 30
    t |> equal(tab[1_l], 10)
    t |> equal(tab[1000000000000_l], 20)
    t |> equal(tab[-1_l], 30)
}

[test]
def test_table_i64_key_overwrite(t : T?) {
    var tab : table<int64; string>
    tab[42_l] = "first"
    tab[42_l] = "second"
    t |> equal(length(tab), 1)
    t |> equal(tab[42_l], "second")
}

[test]
def test_table_u64_key_index(t : T?) {
    var tab : table<uint64; int>
    tab[1_ul] = 100
    tab[18446744073709551615_ul] = 200
    t |> equal(tab[1_ul], 100)
    t |> equal(tab[18446744073709551615_ul], 200)
}

[test]
def test_table_i64_key_via_local(t : T?) {
    var tab : table<int64; int>
    let k1 : int64 = 5_l
    let k2 : int64 = 100_l
    tab[k1] = 50
    tab[k2] = 1000
    t |> equal(tab[k1], 50)
    t |> equal(tab[k2], 1000)
}

[test]
def test_table_i64_key_via_arg(t : T?) {
    var tab : table<int64; int>
    fill_table(tab, 7_l, 77)
    fill_table(tab, 13_l, 133)
    t |> equal(read_table(tab, 7_l), 77)
    t |> equal(read_table(tab, 13_l), 133)
}

def fill_table(var tab : table<int64; int>; k : int64; v : int) {
    tab[k] = v
}

def read_table(var tab : table<int64; int>; k : int64) : int {
    return tab[k]
}
