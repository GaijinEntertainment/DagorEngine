options gen2

require dastest/testing_boost public
require daslib/linq_boost
require strings

// Table source (each_kv / keys / values) through _fold — fused TableAdapter lanes must agree with
// hand loops over the same table. Slot order is unspecified but stable per table instance, so
// order-sensitive expectations compare against a keys()/values() walk of the same table.

struct Pt {
    x : int
    y : int
}

def make_int_table(n : int) : table<int; int> {
    var t <- { for (i in range(n)); i => i * 10 }
    return <- t
}

def make_pt_table : table<string; Pt> {
    return <- {
        "a" => Pt(x = 1, y = 10),
        "b" => Pt(x = 2, y = 20),
        "c" => Pt(x = 3, y = 30),
        "d" => Pt(x = 4, y = 40)
    }
}

[test]
def test_table_fold_count_shortcuts(t : T?) {
    t |> run("bare count is length") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab).count()), 10)
        t |> equal(_fold(keys(tab).count()), 10)
        t |> equal(_fold(values(tab).count()), 10)
        t |> equal(_fold(each_kv(tab).long_count()), 10l)
        delete tab
    }
    t |> run("count with predicate walks") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._where(_.key % 2 == 0).count()), 5)
        t |> equal(_fold(each_kv(tab)._where(_.value > 50).count()), 4)
        delete tab
    }
    t |> run("empty table") @(t : T?) {
        let e : table<int; int>
        t |> equal(_fold(each_kv(e).count()), 0)
        t |> equal(_fold(each_kv(e).any()), false)
        t |> equal(_fold(each_kv(e)._where(_.key > 0).count()), 0)
    }
}

[test]
def test_table_fold_accumulators(t : T?) {
    t |> run("sum/min/max over values and keys") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(values(tab).sum()), 450)
        t |> equal(_fold(keys(tab).sum()), 45)
        t |> equal(_fold(each_kv(tab)._select(_.value).sum()), 450)
        t |> equal(_fold(each_kv(tab)._select(_.key).min()), 0)
        t |> equal(_fold(each_kv(tab)._select(_.value).max()), 90)
        // body touches both sides — zipped walk
        t |> equal(_fold(each_kv(tab)._select(_.key + _.value).sum()), 495)
        delete tab
    }
    t |> run("early exit: any/all/contains") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab).any()), true)
        t |> equal(_fold(each_kv(tab)._where(_.value > 80).any()), true)
        t |> equal(_fold(each_kv(tab)._where(_.value > 90).any()), false)
        t |> equal(_fold(each_kv(tab)._select(_.key)._all(_ >= 0)), true)
        t |> equal(_fold(values(tab).contains(40)), true)
        t |> equal(_fold(values(tab).contains(41)), false)
        delete tab
    }
}

[test]
def test_table_fold_to_array_agreement(t : T?) {
    t |> run("kv where+select agrees with hand loop, in slot order") @(t : T?) {
        var tab <- make_pt_table()
        var expected : array<int>
        for (_k, v in keys(tab), values(tab)) {
            if (v.x > 1) {
                expected |> push(v.y)
            }
        }
        var got <- _fold(each_kv(tab)._where(_.value.x > 1)._select(_.value.y).to_array())
        t |> equal(length(got), length(expected))
        for (i in range(length(expected))) {
            t |> equal(got[i], expected[i])
        }
        delete got
        delete expected
        delete tab
    }
    t |> run("keys to_array in slot order") @(t : T?) {
        var tab <- make_pt_table()
        var expected : array<string>
        for (k in keys(tab)) {
            expected |> push(k)
        }
        // bare `<src>.to_array()` is not a recognized chain (any source) — keep a where on it
        var got <- _fold(keys(tab)._where(_ != "zzz").to_array())
        t |> equal(length(got), length(expected))
        for (i in range(length(expected))) {
            t |> equal(got[i], expected[i])
        }
        delete got
        delete expected
        delete tab
    }
    t |> run("whole-kv escape: identity to_array") @(t : T?) {
        var tab <- make_pt_table()
        var got <- _fold(each_kv(tab)._where(_.value.x >= 3).to_array())
        t |> equal(length(got), 2)
        for (kv in got) {
            let expectedPt = tab?[kv.key] ?? Pt()
            t |> equal(expectedPt.x, kv.value.x)
            t |> equal(expectedPt.y, kv.value.y)
        }
        delete got
        delete tab
    }
}

[test]
def test_table_fold_order_distinct_take(t : T?) {
    t |> run("order_by key descending") @(t : T?) {
        var tab <- make_int_table(10)
        var got <- _fold(each_kv(tab)._select(_.key).order_by_descending(@(k : int) => k).to_array())
        t |> equal(length(got), 10)
        for (i in range(10)) {
            t |> equal(got[i], 9 - i)
        }
        delete got
        delete tab
    }
    t |> run("redundant distinct over keys/kv is dropped but correct") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab).distinct().count()), 10)
        t |> equal(_fold(each_kv(tab)._where(_.key > 4).distinct().count()), 5)
        t |> equal(_fold(keys(tab).distinct().count()), 10)
        delete tab
    }
    t |> run("values distinct stays real") @(t : T?) {
        var dup : table<int; int>
        dup |> insert(1, 7)
        dup |> insert(2, 7)
        dup |> insert(3, 8)
        t |> equal(_fold(values(dup).distinct().count()), 2)
        t |> equal(_fold(each_kv(dup)._select(_.value).distinct().count()), 2)
        delete dup
    }
    t |> run("take/skip ride the walk") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._select(_.value).take(3).count()), 3)
        t |> equal(_fold(each_kv(tab)._select(_.key).skip(4).count()), 6)
        var firstTwo <- _fold(keys(tab).take(2).to_array())
        var expected : array<int>
        for (k in keys(tab)) {
            if (length(expected) < 2) {
                expected |> push(k)
            }
        }
        t |> equal(length(firstTwo), 2)
        t |> equal(firstTwo[0], expected[0])
        t |> equal(firstTwo[1], expected[1])
        delete firstTwo
        delete expected
        delete tab
    }
    t |> run("first / first_or_default") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._where(_.key == 7)._select(_.value).first()), 70)
        t |> equal(_fold(each_kv(tab)._where(_.key == 99)._select(_.value).first_or_default(-1)), -1)
        delete tab
    }
}

[test]
def test_table_fold_iterator_result(t : T?) {
    t |> run("chain consumed as iterator") @(t : T?) {
        var tab <- make_int_table(6)
        var s = 0
        for (v in _fold(each_kv(tab)._where(_.key % 2 == 0)._select(_.value))) {
            s += v
        }
        t |> equal(s, 0 + 20 + 40)
        delete tab
    }
}

[test]
def test_table_fold_set_form(t : T?) {
    t |> run("keys over table<K> set") @(t : T?) {
        var s : table<string>
        s |> insert("alpha")
        s |> insert("beta")
        s |> insert("gamma")
        t |> equal(_fold(keys(s).count()), 3)
        t |> equal(_fold(keys(s)._where(_ |> length() > 4).count()), 2)
        delete s
    }
}

def private bump_key(var c : int&) : int {
    c++
    return 2
}

// Point-lookup folds: `where(kv.key == X [&& residual])` + terminator → O(1) key probe (key_exists /
// `tab?[X]`), residual conjuncts evaluated on the probed element only. Probes must agree with the scan
// on hit AND miss; non-probe shapes (key-eq not the leftmost conjunct) must keep riding the scan.

[test]
def test_table_point_lookup(t : T?) {
    t |> run("kv any/count/first probes, hit and miss") @(t : T?) {
        var tab <- make_int_table(10)
        let k = 7
        t |> equal(_fold(each_kv(tab)._where(_.key == k).any()), true)
        t |> equal(_fold(each_kv(tab)._where(_.key == 99).any()), false)
        t |> equal(_fold(each_kv(tab)._any(_.key == k)), true)
        t |> equal(_fold(each_kv(tab)._where(_.key == k).count()), 1)
        t |> equal(_fold(each_kv(tab)._where(_.key == 99).count()), 0)
        t |> equal(_fold(each_kv(tab)._count(_.key == 3)), 1)
        let f = _fold(each_kv(tab)._where(k == _.key).first())   // flipped operand side
        t |> equal(f.key, 7)
        t |> equal(f.value, 70)
        let m = _fold(each_kv(tab)._where(_.key == 99).first_or_default(default<tuple<key : int; value : int>>))
        t |> equal(m.key, 0)
        delete tab
    }
    t |> run("probe + trailing select projects the probed element") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._where(_.key == 4)._select(_.value).first()), 40)
        t |> equal(_fold(each_kv(tab)._where(_.key == 99)._select(_.value).first_or_default(-1)), -1)
        t |> equal(_fold(each_kv(tab)._where(_.key == 4)._select("{_.key}:{_.value}").first()), "4:40")
        delete tab
    }
    t |> run("keys lane probes + set form contains") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(keys(tab)._where(_ == 5).any()), true)
        t |> equal(_fold(keys(tab)._where(_ == 5).first()), 5)
        t |> equal(_fold(keys(tab)._where(_ == 5)._select(_ * 100).first()), 500)
        t |> equal(_fold(keys(tab).contains(5)), true)
        t |> equal(_fold(keys(tab).contains(55)), false)
        delete tab
        var s : table<string> <- { "x", "y" }
        t |> equal(_fold(keys(s).contains("y")), true)
        t |> equal(_fold(keys(s).contains("z")), false)
        delete s
    }
    t |> run("residual conjuncts route to the probed element only") @(t : T?) {
        var tab <- make_int_table(10)
        let h = _fold(each_kv(tab)._where(_.key == 5 && _.value > 0).first_or_default(default<IKV>))
        t |> equal(h.key, 5)
        t |> equal(h.value, 50)
        let rf = _fold(each_kv(tab)._where(_.key == 5 && _.value > 500).first_or_default(default<IKV>))
        t |> equal(rf.key, 0, "residual-false on a hit routes to the miss path")
        let ms = _fold(each_kv(tab)._where(_.key == 99 && _.value > 0).first_or_default(default<IKV>))
        t |> equal(ms.key, 0)
        t |> equal(_fold(each_kv(tab)._any(_.key == 3 && _.value == 30)), true)
        t |> equal(_fold(each_kv(tab)._any(_.key == 3 && _.value == 31)), false)
        t |> equal(_fold(each_kv(tab)._count(_.key == 3 && _.value == 30)), 1)
        t |> equal(_fold(each_kv(tab)._count(_.key == 3 && _.value == 31)), 0)
        // three conjuncts AND-rebuild in order; trailing select projects the probed element
        t |> equal(_fold(each_kv(tab)._where(_.key == 6 && _.value > 0 && _.value < 100)._select(_.value).first_or_default(-1)), 60)
        // `key == 2 && key == 3` probes 2, then the residual re-check fails — agrees with the scan
        t |> equal(_fold(each_kv(tab)._where(_.key == 2 && _.key == 3).count()), 0)
        delete tab
    }
    t |> run("consecutive wheres AND-merge before conjunct extraction") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._where(_.key == 5)._where(_.value > 0).any()), true)
        t |> equal(_fold(each_kv(tab)._where(_.key == 5)._where(_.value > 500).any()), false)
        t |> equal(_fold(each_kv(tab)._where(_.key == 4)._where(_.value > 0)._select(_.value).first()), 40)
        delete tab
    }
    t |> run("keys lane residual") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(keys(tab)._where(_ == 8 && _ % 2 == 0).first_or_default(-1)), 8)
        t |> equal(_fold(keys(tab)._where(_ == 7 && _ % 2 == 0).first_or_default(-1)), -1)
        delete tab
    }
    t |> run("residual evaluates once, like the scan (keys are unique)") @(t : T?) {
        var tab <- make_int_table(4)
        var evals = 0
        t |> equal(_fold(each_kv(tab)._where(_.key == 2 && bump_key(evals) > 0).count()), 1)
        t |> equal(evals, 1, "residual must run once, on the probed element")
        delete tab
    }
    t |> run("first probe panics on a missing key or a failed residual, like the scan") @(t : T?) {
        var tab <- make_int_table(4)
        var panicked = false
        try {
            let _r = _fold(each_kv(tab)._where(_.key == 99).first())
        } recover {
            panicked = true
        }
        t |> equal(panicked, true)
        panicked = false
        try {
            let _r = _fold(each_kv(tab)._where(_.key == 2 && _.value > 999).first())
        } recover {
            panicked = true
        }
        t |> equal(panicked, true)
        delete tab
    }
    t |> run("non-probe shapes stay scans and stay correct") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._where(_.key != 5).count()), 9)                       // wrong operator
        t |> equal(_fold(each_kv(tab)._where(_.key == _.value / 10).count()), 10)           // X references the binder
        t |> equal(_fold(each_kv(tab)._where(_.value > 0)._where(_.key == 5).any()), true)  // key-eq not leftmost
        delete tab
    }
    t |> run("impure X stays a scan — per-element evaluation preserved") @(t : T?) {
        var tab <- make_int_table(4)
        var evals = 0
        t |> equal(_fold(each_kv(tab)._where(_.key == bump_key(evals)).count()), 1)
        t |> equal(evals, 4, "side-effectful X must evaluate per element, not once")
        delete tab
    }
}

typedef IKV = tuple<key : int; value : int>

def flip_kv(kv : IKV) : tuple<int; int> {
    return (kv.value => kv.key)
}

// Joins: a table in the srcB slot joined on its bare key (`d.key` / bare set element) probes the user's
// table instead of building the join's internal hash; a table lead rides the same emitter through the
// pruned slot walk. Either way must agree with the hash/hand-loop semantics: inner joins drop misses,
// keep lead duplicates; group joins stay outer.

[test]
def test_table_join_srcb_probe(t : T?) {
    t |> run("inner join on the table key: count + rows in lead order") @(t : T?) {
        var ids <- [0, 1, 1, 4, 9, 2, 0, 7]   // dups stay, 9/7 miss
        var dtab <- make_int_table(5)
        let n = _fold(ids |> _join(unsafe(each_kv(dtab)),
                                   $(a : int, d : IKV) => a == d.key,
                                   $(a : int, d : IKV) => d.value)
                          |> count())
        t |> equal(n, 6)
        var got <- _fold(ids |> _join(unsafe(each_kv(dtab)),
                                      $(a : int, d : IKV) => a == d.key,
                                      $(a : int, d : IKV) => (A = a, V = d.value))
                             |> to_array())
        var expected <- [(A = 0, V = 0), (A = 1, V = 10), (A = 1, V = 10), (A = 4, V = 40), (A = 2, V = 20), (A = 0, V = 0)]
        t |> equal(length(got), length(expected))
        for (i in range(length(expected))) {
            t |> equal(got[i].A, expected[i].A)
            t |> equal(got[i].V, expected[i].V)
        }
        delete got
        delete expected
        delete dtab
        delete ids
    }
    t |> run("trailing where + select on the probed pair") @(t : T?) {
        var ids <- [0, 1, 1, 4, 9, 2, 0, 7]
        var dtab <- make_int_table(5)
        var got <- _fold(ids |> _join(unsafe(each_kv(dtab)),
                                      $(a : int, d : IKV) => a == d.key,
                                      $(a : int, d : IKV) => (A = a, V = d.value))
                             |> _where(_.V > 5)
                             |> _select("{_.A}:{_.V}")
                             |> to_array())
        t |> equal(length(got), 4)
        t |> equal(got[0], "1:10")
        t |> equal(got[1], "1:10")
        t |> equal(got[2], "4:40")
        t |> equal(got[3], "2:20")
        delete got
        delete dtab
        delete ids
    }
    t |> run("empty table srcB matches nothing") @(t : T?) {
        var ids <- [0, 1, 2]
        let e : table<int; int>
        let n = _fold(ids |> _join(unsafe(each_kv(e)),
                                   $(a : int, d : IKV) => a == d.key,
                                   $(a : int, d : IKV) => d.value)
                          |> count())
        t |> equal(n, 0)
        delete ids
    }
    t |> run("set srcB joins on membership") @(t : T?) {
        var ids <- [0, 1, 1, 4, 9]
        var s : table<int> <- { 1, 4 }
        let n = _fold(ids |> _join(unsafe(keys(s)),
                                   $(a : int, k : int) => a == k,
                                   $(a : int, k : int) => a)
                          |> count())
        t |> equal(n, 3)
        delete s
        delete ids
    }
    t |> run("non-bare b key stays hashed and agrees") @(t : T?) {
        var ids <- [0, 1, 1, 4, 9, 2, 0, 7]
        var dtab <- make_int_table(5)
        // value/10 == key for this fixture, so the hashed walk must find the same 6 pairs
        let n = _fold(ids |> _join(unsafe(each_kv(dtab)),
                                   $(a : int, d : IKV) => a == d.value / 10,
                                   $(a : int, d : IKV) => d.value)
                          |> count())
        t |> equal(n, 6)
        delete dtab
        delete ids
    }
    t |> run("group join over a table srcB stays outer") @(t : T?) {
        var ids <- [0, 9, 4]
        var dtab <- make_int_table(5)
        var got <- _fold(ids |> _group_join(unsafe(each_kv(dtab)),
                                            $(a : int, d : IKV) => a == d.key,
                                            $(a : int, ds : array<IKV>) => (A = a, N = length(ds)))
                             |> to_array())
        t |> equal(length(got), 3)
        t |> equal(got[0].N, 1)
        t |> equal(got[1].N, 0)
        t |> equal(got[2].N, 1)
        delete got
        delete dtab
        delete ids
    }
    t |> run("table lead joining a table srcB probes both sides") @(t : T?) {
        var ctab <- make_int_table(8)
        var dtab <- make_int_table(5)
        let n = _fold(each_kv(ctab) |> _join(unsafe(each_kv(dtab)),
                                             $(c : IKV, d : IKV) => c.key == d.key,
                                             $(c : IKV, d : IKV) => c.value + d.value)
                                    |> count())
        t |> equal(n, 5)
        delete dtab
        delete ctab
    }
}

[test]
def test_table_join_lead(t : T?) {
    t |> run("table lead, array srcB: count keeps bucket multiplicity") @(t : T?) {
        var ctab <- make_int_table(10)
        var darr <- [0, 2, 2, 4, 11]
        let n = _fold(each_kv(ctab) |> _join(darr,
                                             $(c : IKV, d : int) => c.key == d,
                                             $(c : IKV, d : int) => (K = c.key, V = c.value))
                                    |> count())
        t |> equal(n, 4)
        delete darr
        delete ctab
    }
    t |> run("table lead to_array agrees with a hand loop in slot order") @(t : T?) {
        var ctab <- make_int_table(10)
        var darr <- [0, 2, 2, 4, 11]
        var expected : array<int>
        for (k, v in keys(ctab), values(ctab)) {
            for (d in darr) {
                if (k == d) {
                    expected |> push(v + d)
                }
            }
        }
        var got <- _fold(each_kv(ctab) |> _join(darr,
                                                $(c : IKV, d : int) => c.key == d,
                                                $(c : IKV, d : int) => c.value + d)
                                       |> to_array())
        t |> equal(length(got), length(expected))
        for (i in range(length(expected))) {
            t |> equal(got[i], expected[i])
        }
        delete got
        delete expected
        delete darr
        delete ctab
    }
    t |> run("lead where filters before the join") @(t : T?) {
        var ctab <- make_int_table(10)
        var darr <- [0, 2, 2, 4, 11]
        let n = _fold(each_kv(ctab) |> _where(_.key > 1)
                                    |> _join(darr,
                                             $(c : IKV, d : int) => c.key == d,
                                             $(c : IKV, d : int) => c.value)
                                    |> count())
        t |> equal(n, 3)
        delete darr
        delete ctab
    }
    t |> run("values lane lead") @(t : T?) {
        var ctab <- make_int_table(10)
        var darr <- [0, 2, 2, 4, 11]
        let n = _fold(values(ctab) |> _join(darr,
                                            $(v : int, d : int) => v == d * 10,
                                            $(v : int, d : int) => v)
                                   |> count())
        t |> equal(n, 4)
        delete darr
        delete ctab
    }
    t |> run("table lead group_join stays outer over every slot") @(t : T?) {
        var ctab <- make_int_table(10)
        var darr <- [0, 2, 2, 4, 11]
        var got <- _fold(each_kv(ctab) |> _group_join(darr,
                                                      $(c : IKV, d : int) => c.key == d,
                                                      $(c : IKV, ds : array<int>) => length(ds))
                                       |> to_array())
        t |> equal(length(got), 10)
        var total = 0
        for (n in got) {
            total += n
        }
        t |> equal(total, 4)
        delete got
        delete darr
        delete ctab
    }
}

// Tier-2 over the raw each_kv iterator (no _fold) — the [unsafe_outside_of_for] contract requires the
// explicit unsafe(...) wrap at a bare chain head; fused chains rewrite the head before inference.

[test]
def test_each_kv_tier2(t : T?) {
    t |> run("where/select/to_array over each_kv") @(t : T?) {
        var tab : table<string; int>
        tab |> insert("a", 1)
        tab |> insert("b", 2)
        tab |> insert("c", 3)
        var vals <- unsafe(each_kv(tab)) |> _where(_.value > 1) |> _select(_.value) |> to_array()
        vals |> sort()  // slot order is unspecified
        t |> equal(length(vals), 2)
        t |> equal(vals[0], 2)
        t |> equal(vals[1], 3)
        delete vals
    }
    t |> run("keys participate in the chain") @(t : T?) {
        var tab : table<int; int>
        for (i in range(6)) {
            tab |> insert(i, i * i)
        }
        let n = unsafe(each_kv(tab)) |> _where(_.key % 2 == 0) |> count()
        t |> equal(n, 3)
    }
}

[test]
def test_to_table_sink(t : T?) {
    t |> run("kv pass-through + where agrees with a hand loop") @(t : T?) {
        var src <- make_int_table(6)
        var expected : table<int; int>
        for (k, v in keys(src), values(src)) {
            if (k > 1) {
                expected[k] = v
            }
        }
        var got <- _fold(each_kv(src) |> _where(_.key > 1) |> to_table())
        t |> equal(length(got), length(expected))
        for (k, v in keys(expected), values(expected)) {
            t |> equal(got?[k] ?? -1, v)
        }
        delete got
        delete expected
        delete src
    }
    t |> run("projection remaps keys and values") @(t : T?) {
        var src <- make_int_table(5)
        var got <- _fold(each_kv(src) |> _select((_.key * 2) => _.value + 1) |> to_table())
        t |> equal(length(got), 5)
        t |> equal(got?[8] ?? -1, 41)
        delete got
        delete src
    }
    t |> run("bare to_table clones the table through the fused walk") @(t : T?) {
        var src <- make_int_table(5)
        var got <- _fold(each_kv(src) |> to_table())
        t |> equal(length(got), 5)
        t |> equal(got?[0] ?? -1, 0)
        t |> equal(got?[4] ?? -1, 40)
        delete got
        delete src
    }
    t |> run("keys chain lands in the set form") @(t : T?) {
        var src <- make_int_table(5)
        var got <- _fold(keys(src) |> _where(_ != 3) |> to_table())
        t |> equal(length(got), 4)
        t |> success(key_exists(got, 0) && !key_exists(got, 3))
        delete got
        delete src
    }
    t |> run("array source with a tuple projection") @(t : T?) {
        var arr <- [for (i in range(4)); i + 1]
        var got <- _fold(arr |> _select(_ => _ * _) |> to_table())
        t |> equal(length(got), 4)
        t |> equal(got?[3] ?? -1, 9)
        delete got
        delete arr
    }
    t |> run("duplicate keys keep the last occurrence") @(t : T?) {
        var arr <- [for (i in range(6)); i]
        var got <- _fold(arr |> _select((_ % 2) => _) |> to_table())
        t |> equal(length(got), 2)
        t |> equal(got?[0] ?? -1, 4)
        t |> equal(got?[1] ?? -1, 5)
        delete got
        delete arr
    }
    t |> run("take bounds the walk and the reserve") @(t : T?) {
        var src <- make_int_table(5)
        var got <- _fold(keys(src) |> take(2) |> to_table())
        t |> equal(length(got), 2)
        delete got
        delete src
    }
    t |> run("non-tuple-literal projection rides the bind arm") @(t : T?) {
        var src <- make_int_table(5)
        var got <- _fold(each_kv(src) |> _select(flip_kv(_)) |> to_table())
        t |> equal(length(got), 5)
        t |> equal(got?[40] ?? -1, 4)
        delete got
        delete src
    }
    t |> run("string keys through the fused map arm") @(t : T?) {
        var src <- make_int_table(4)
        var got <- _fold(each_kv(src) |> _select(("k{_.key}" => _.value)) |> to_table())
        t |> equal(length(got), 4)
        t |> equal(got?["k2"] ?? -1, 20)
        delete got
        delete src
    }
    t |> run("tier-2 iterator to_table agrees with the fused emit") @(t : T?) {
        var src <- make_int_table(6)
        var fused <- _fold(each_kv(src) |> _where(_.key % 2 == 0) |> to_table())
        var tier2 <- to_table(unsafe(each_kv(src)) |> _where(_.key % 2 == 0))
        t |> equal(length(fused), length(tier2))
        for (k, v in keys(tier2), values(tier2)) {
            t |> equal(fused?[k] ?? -1, v)
        }
        delete fused
        delete tier2
        delete src
    }
    t |> run("array-input to_table forms borrow the source") @(t : T?) {
        var pairs <- [for (i in range(3)); (i => i * i)]
        var m <- to_table(pairs)
        t |> equal(length(pairs), 3)   // source intact
        t |> equal(length(m), 3)
        t |> equal(m?[2] ?? -1, 4)
        var bare <- [for (i in range(4)); i * 100]
        var s <- to_table(bare)
        t |> equal(length(bare), 4)
        t |> success(key_exists(s, 300))
        delete s
        delete bare
        delete m
        delete pairs
    }
    t |> run("selector-based to_table keeps its tier-2 path") @(t : T?) {
        var arr <- [for (i in range(4)); i]
        var got <- to_table(to_sequence(arr), $(x : int) => x, $(x : int) => x * 10)
        t |> equal(length(got), 4)
        t |> equal(got?[3] ?? -1, 30)
        delete got
        delete arr
    }
}

[test]
def test_table_group_by(t : T?) {
    t |> run("kv lane: count + sum per group agrees with a hand loop") @(t : T?) {
        var tab <- make_int_table(20)
        let groups <- _fold(each_kv(tab)
                            ._group_by(_.key % 3)
                            ._select((K = _._0, N = _._1 |> count(), S = _._1 |> select($(kv : IKV) => kv.value) |> sum()))
                            .to_array())
        var expN : table<int; int>
        var expS : table<int; int>
        for (k, v in keys(tab), values(tab)) {
            expN[k % 3] ++
            expS[k % 3] += v
        }
        t |> equal(length(groups), length(expN))
        for (g in groups) {
            t |> equal(g.N, expN?[g.K] ?? -1)
            t |> equal(g.S, expS?[g.K] ?? -1)
        }
        delete expN
        delete expS
        delete tab
    }
    t |> run("values lane: group key + reducer over struct values") @(t : T?) {
        var tab <- make_pt_table()
        let groups <- _fold(values(tab)
                            ._group_by(_.x % 2)
                            ._select((K = _._0, MaxY = _._1 |> select($(p : Pt) => p.y) |> max()))
                            .to_array())
        t |> equal(length(groups), 2)
        for (g in groups) {
            t |> equal(g.MaxY, g.K == 0 ? 40 : 30)
        }
        delete tab
    }
    t |> run("keys lane: group over raw keys") @(t : T?) {
        var tab <- make_int_table(10)
        let groups <- _fold(keys(tab)._group_by(_ % 4)._select((K = _._0, N = _._1 |> length)).to_array())
        t |> equal(length(groups), 4)
        var total = 0
        for (g in groups) {
            total += g.N
        }
        t |> equal(total, 10)
        delete tab
    }
    t |> run("upstream where + select segments feed the bucket fill") @(t : T?) {
        var tab <- make_int_table(20)
        let groups <- _fold(each_kv(tab)
                            ._where(_.key % 2 == 0)
                            ._select(_.value)
                            ._group_by(_ % 40)
                            ._select((K = _._0, N = _._1 |> length))
                            .to_array())
        // even keys 0..18 -> values 0,20,..,180; % 40 buckets {0, 20}, 5 each
        t |> equal(length(groups), 2)
        for (g in groups) {
            t |> equal(g.N, 5)
        }
        delete tab
    }
    t |> run("having + trailing where + order") @(t : T?) {
        var tab <- make_int_table(12)
        let groups <- _fold(each_kv(tab)
                            ._group_by(_.key % 5)
                            ._having(_._1 |> length >= 2)
                            ._select((K = _._0, S = _._1 |> select($(kv : IKV) => kv.value) |> sum()))
                            ._where(_.S > 60)
                            ._order_by(_.S)
                            .to_array())
        // buckets 0..4 sizes 3,3,2,2,2; sums 150,180,90,110,130 -> having keeps all, where keeps S>60
        t |> equal(length(groups), 5)
        var prev = -1
        for (g in groups) {
            t |> success(g.S > 60 && g.S >= prev)
            prev = g.S
        }
        delete tab
    }
    t |> run("count terminator counts groups") @(t : T?) {
        var tab <- make_int_table(10)
        t |> equal(_fold(each_kv(tab)._group_by(_.key % 3)._select((K = _._0, N = _._1 |> length)).count()), 3)
        delete tab
    }
    t |> run("empty table yields no groups") @(t : T?) {
        let e : table<int; int>
        let groups <- _fold(each_kv(e)._group_by(_.key)._select((K = _._0, N = _._1 |> length)).to_array())
        t |> equal(length(groups), 0)
    }
    t |> run("fused agrees with the tier-2 iterator path") @(t : T?) {
        var tab <- make_int_table(15)
        let fused <- _fold(each_kv(tab)
                           ._group_by(_.key % 4)
                           ._select((K = _._0, N = _._1 |> count()))
                           .to_array())
        var tier2 <- unsafe(each_kv(tab)) |> _group_by(_.key % 4) |> _select((K = _._0, N = _._1 |> length)) |> to_array()
        t |> equal(length(fused), length(tier2))
        var t2 : table<int; int>
        for (g in tier2) {
            t2[g.K] = g.N
        }
        for (g in fused) {
            t |> equal(g.N, t2?[g.K] ?? -1)
        }
        delete t2
        delete tier2
        delete tab
    }
}
