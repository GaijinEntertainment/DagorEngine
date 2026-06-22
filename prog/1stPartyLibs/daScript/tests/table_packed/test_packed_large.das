options gen2

require dastest/testing_boost

// Large (open-addressed) table regression for the non-string control-byte representation
// (commit B): N > TABLE_MAX_LINEAR_CAPACITY (8) forces promotion past the packed regime, so a
// non-string table stores a 1-byte control state per slot and compares keys directly. Strings keep
// their 32-bit hash; the string lane here guards that the shared (if-constexpr-split) large-table
// find/reserve/erase still works.

// Distinct-pointer string key (fresh each call) so queries differ from the stored pointers,
// forcing a real KeyCompare on the large-table string path.
def dkey(i : int) : string {
    return "key_{i}"
}

def sorted_int_keys(tab : table<int; int>) : array<int> {
    var got <- [for (k in keys(tab)); k]
    sort(got)
    return <- got
}

def check_int_keys(t : T?; tab : table<int; int>; expected : array<int>) {
    var got <- sorted_int_keys(tab)
    t |> equal(length(got), length(expected))
    if (length(got) == length(expected)) {
        for (a, b in got, expected) {
            t |> equal(a, b)
        }
    }
    delete got
}

[test]
def test_large_int_hit_miss(t : T?) {
    var tab : table<int; int>    // nolint:STYLE027  index-assign builds the large table under test
    for (i in range(40)) {        // 40 > 8 -> open-addressed
        unsafe(tab[i]) = i * 10
    }
    t |> equal(length(tab), 40)
    for (i in range(40)) {
        t |> equal(tab?[i] ?? -1, i * 10)        // hit
    }
    t |> equal(tab?[999] ?? -1, -1)              // miss past the end
    t |> equal(tab?[-7] ?? -1, -1)               // miss (negative key)
    t |> equal(key_exists(tab, 17), true)
    t |> equal(key_exists(tab, 100), false)
    var expected <- [for (i in range(40)); i]
    check_int_keys(t, tab, expected)
    delete expected
    unsafe { delete tab; }
}

[test]
def test_large_int_erase_tombstone(t : T?) {
    var tab : table<int; int>    // nolint:STYLE027  index-assign + erase under test
    for (i in range(40)) {
        unsafe(tab[i]) = i
    }
    // erase every third key -> tombstones scattered through the probe chains
    for (i in range(40)) {
        if (i % 3 == 0) {
            t |> equal(erase(tab, i), true)
        }
    }
    for (i in range(40)) {
        if (i % 3 == 0) {
            t |> equal(tab?[i] ?? -1, -1)        // erased -> miss (probe skips the tombstone)
            t |> equal(key_exists(tab, i), false)
        } else {
            t |> equal(tab?[i] ?? -1, i)         // survivor still findable
        }
    }
    // reinsert the erased keys -> tombstone reuse
    for (i in range(40)) {
        if (i % 3 == 0) {
            unsafe(tab[i]) = i + 1000
        }
    }
    for (i in range(40)) {
        let want = (i % 3 == 0) ? i + 1000 : i
        t |> equal(tab?[i] ?? -1, want)
    }
    t |> equal(length(tab), 40)
    unsafe { delete tab; }
}

[test]
def test_large_int_grow_rehash(t : T?) {
    // grow through several doublings; each grow rehashes the control-byte source, recomputing
    // every live key's hash to re-bucket into the larger target.
    var tab : table<int; int>    // nolint:STYLE027  index-assign drives repeated grow
    for (i in range(300)) {
        unsafe(tab[i]) = i * 2
    }
    t |> equal(length(tab), 300)
    for (i in range(300)) {
        t |> equal(tab?[i] ?? -1, i * 2)
    }
    t |> equal(tab?[300] ?? -1, -1)
    unsafe { delete tab; }
}

[test]
def test_large_int_churn(t : T?) {
    // insert/erase churn builds tombstones -> same-capacity rehash on the control-byte path.
    var tab : table<int; int>    // nolint:STYLE027  churn under test
    let lastRound = 5
    for (round in range(lastRound + 1)) {
        for (i in range(50)) {
            unsafe(tab[i]) = round * 1000 + i
        }
        for (i in range(50)) {
            if (i % 2 == 0) {
                erase(tab, i)
            }
        }
    }
    // odd keys carry the last round's value; even keys were erased last
    for (i in range(50)) {
        if (i % 2 == 0) {
            t |> equal(key_exists(tab, i), false)
        } else {
            t |> equal(tab?[i] ?? -1, lastRound * 1000 + i)
        }
    }
    unsafe { delete tab; }
}

[test]
def test_large_string(t : T?) {
    var tab : table<string; int>    // nolint:STYLE027  index-assign builds the large string table
    for (i in range(40)) {
        unsafe(tab[dkey(i)]) = i * 7
    }
    for (i in range(40)) {
        t |> equal(tab?[dkey(i)] ?? -1, i * 7)   // distinct-pointer query (real KeyCompare)
    }
    t |> equal(tab?[dkey(999)] ?? -1, -1)
    t |> equal(key_exists(tab, dkey(13)), true)
    t |> equal(erase(tab, dkey(13)), true)
    t |> equal(key_exists(tab, dkey(13)), false)
    t |> equal(tab?[dkey(14)] ?? -1, 98)
    t |> equal(length(tab), 39)
    // reinsert after erase: a new key reuses the freed tombstone slot (no grow)
    unsafe(tab[dkey(1000)]) = 7000
    t |> equal(length(tab), 40)
    t |> equal(tab?[dkey(1000)] ?? -1, 7000)
    t |> equal(tab?[dkey(13)] ?? -1, -1)
    t |> equal(tab?[dkey(0)] ?? -1, 0)
    unsafe { delete tab; }
}

struct LargeVal {
    a : int
    b : int
    c : int
}

[test]
def test_large_struct_value(t : T?) {
    // struct value larger than a workhorse: exercises valueTypeSize movement in the large-table
    // erase + rehash copy.
    var tab : table<int; LargeVal>    // nolint:STYLE027  struct-value large table under test
    for (i in range(40)) {
        unsafe(tab[i]) = LargeVal(a = i, b = i * 2, c = i * 3)
    }
    t |> equal(erase(tab, 10), true)
    t |> equal(erase(tab, 20), true)
    let v = unsafe(tab?[30])
    t |> success(v != null)
    if (v != null) {
        t |> equal(v.a, 30)
        t |> equal(v.b, 60)
        t |> equal(v.c, 90)
    }
    t |> equal(key_exists(tab, 10), false)
    t |> equal(length(tab), 38)
    unsafe { delete tab; }
}

[test]
def test_promote_packed_to_large(t : T?) {
    // 8 packed entries, then the 9th promotes to open addressing: the packed source copies into the
    // control-byte target. Every key must survive.
    var tab : table<int; int>    // nolint:STYLE027  index-assign exercises packed->large promotion
    for (i in range(8)) {
        unsafe(tab[i]) = i * 100
    }
    t |> equal(length(tab), 8)
    unsafe(tab[8]) = 800     // 9th insert -> promote
    t |> equal(length(tab), 9)
    for (i in range(9)) {
        t |> equal(tab?[i] ?? -1, i * 100)
    }
    unsafe { delete tab; }
}

[test]
def test_keys_iter_packed_no_overread(t : T?) {
    // Regression: TableKeysIterator must not read the key one-past the keys block. A non-string
    // packed table has no hash region after keys, so the past-end read on exhaustion / empty
    // iteration ran off the allocation (ASan heap-buffer-overflow). Cover a full iteration (the
    // final next lands on table_end) and an emptied table (first lands on table_end).
    var tab : table<int; int>    // nolint:STYLE027  packed table iterated under test
    for (i in range(5)) {
        unsafe(tab[i]) = i
    }
    var n1 = 0
    for (_k in keys(tab)) {
        n1++
    }
    t |> equal(n1, 5)
    for (i in range(5)) {
        erase(tab, i)
    }
    t |> equal(length(tab), 0)
    var n2 = 0
    for (_k in keys(tab)) {
        n2++
    }
    t |> equal(n2, 0)
    unsafe { delete tab; }
}
