options gen2

require dastest/testing_boost

// Const-string table keys exercise the SimNode_*_WithHash interpreter path: the key bytes and
// their hash are baked at simulate time, so the lookup skips the per-call hash walk. tab["lit"]
// and tab?["lit"] are direct intrinsics (ExprAt / ExprSafeAt); find / key_exists go through daslib
// generic wrappers, so the WithHash nodes for those only fire when __builtin_table_* is called
// directly (until the wrappers inline) — tested here via the __builtin_ forms.

[test]
def test_constkey_index(t : T?) {
    var tab : table<string; int>
    tab["foo"] = 1                                   // TableIndex_WithHash (insert)
    tab["bar"] = 2
    tab["baz"] = 3
    t |> equal(length(tab), 3)
    t |> equal(tab["foo"], 1)                         // TableIndex_WithHash (hit, r2v)
    t |> equal(tab["bar"], 2)
    t |> equal(tab["baz"], 3)
    tab["foo"] = 11                                   // overwrite, no new slot
    t |> equal(tab["foo"], 11)
    t |> equal(length(tab), 3)
    unsafe { delete tab; }
}

[test]
def test_constkey_safe_index(t : T?) {
    var tab : table<string; int>
    tab["foo"] = 1
    tab["bar"] = 2
    t |> equal(tab?["foo"] ?? -1, 1)                  // SafeTableIndex_WithHash (hit)
    t |> equal(tab?["bar"] ?? -1, 2)
    t |> equal(tab?["missing"] ?? -1, -1)             // SafeTableIndex_WithHash (miss, no insert)
    t |> equal(length(tab), 2)                        // safe-index miss did not grow the table
    unsafe { delete tab; }
}

[test]
def test_constkey_builtin_key_exists(t : T?) {
    var tab : table<string; int>
    tab["foo"] = 1
    tab["bar"] = 2
    t |> equal(__builtin_table_key_exists(tab, "foo"), true)    // KeyExists_WithHash (hit)
    t |> equal(__builtin_table_key_exists(tab, "bar"), true)
    t |> equal(__builtin_table_key_exists(tab, "missing"), false) // KeyExists_WithHash (miss)
    unsafe { delete tab; }
}

[test]
def test_constkey_builtin_find(t : T?) {
    var tab : table<string; int>
    tab["foo"] = 10
    tab["bar"] = 20
    let pf = __builtin_table_find(tab, "foo")        // TableFind_WithHash (hit)
    t |> success(pf != null)
    if (pf != null) {
        t |> equal(*(unsafe(reinterpret<int?>(pf))), 10)
    }
    let pm = __builtin_table_find(tab, "missing")    // TableFind_WithHash (miss)
    t |> equal(pm == null, true)
    unsafe { delete tab; }
}

// "k1832" and "k16303" share the same 32-bit table hashKey but differ in the full 64-bit hash.
// As literal keys the WithHash node bakes each one's 64-bit hash, so a packed table holds both
// with no strcmp and no promotion; each must be found with its own value, and a 32-bit-colliding
// third literal must miss.
[test]
def test_constkey_hashkey32_collision(t : T?) {
    var tab : table<string; int>
    tab["k1832"] = 111                               // TableIndex_WithHash
    tab["k16303"] = 222
    t |> equal(length(tab), 2)
    t |> equal(tab?["k1832"] ?? -1, 111)             // SafeTableIndex_WithHash, exact 64-bit hash
    t |> equal(tab?["k16303"] ?? -1, 222)
    t |> equal(__builtin_table_key_exists(tab, "k1832"), true)
    t |> equal(__builtin_table_key_exists(tab, "k16303"), true)
    t |> equal(tab?["k9999"] ?? -1, -1)
    unsafe { delete tab; }
}
