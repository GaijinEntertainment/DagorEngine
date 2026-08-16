// Inline table slot-walk (keys/values for-loop sources over workhorse-keyed tables):
// the JIT scans the open-addressed ctrl bytes natively instead of calling the C++
// iterator per element. String keys keep the generic iterator path.
options gen2
require dastest/testing_boost

def kv_sums(t : table<int; int>) : tuple<n : int; ks : int; vs : int> {
    var n = 0
    var ks = 0
    var vs = 0
    for (k, v in keys(t), values(t)) {
        n++
        ks += k
        vs += v
    }
    return (n = n, ks = ks, vs = vs)
}

def keys_sum(t : table<int; int>) : int {
    var s = 0
    for (k in keys(t)) {
        s += k
    }
    return s
}

def values_bump(var t : table<int; int>) {
    for (v in values(t)) {
        v++
    }
}

def keys_break_at(t : table<int; int>; stop : int) : int {
    var n = 0
    for (_k in keys(t)) {
        n++
        break if (n == stop)
    }
    return n
}

def string_kv_sum(t : table<string; int>) : int {
    var s = 0
    for (_k, v in keys(t), values(t)) {
        s += v
    }
    return s
}

def keys64_sum(t : table<int64; int>) : int64 {
    var s = 0l
    for (k in keys(t)) {
        s += k
    }
    return s
}

def insert_during_iteration(var t : table<int; int>) : bool {
    var caught = false
    try {
        for (k in keys(t)) {
            t[100 + k] = 1
        }
    } recover {
        caught = true
    }
    return caught
}

[test]
def test_table_walk(t : T?) {
    t |> run("kv zip with tombstones") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@kv_sums))
        var tab : table<int; int>
        for (i in range(10)) {
            tab[i] = i * 10
        }
        tab |> erase(3)
        tab |> erase(7)
        let r = kv_sums(tab)
        t |> equal(r.n, 8)
        t |> equal(r.ks, 35)
        t |> equal(r.vs, 350)
        delete tab
    }
    t |> run("keys-only walk") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@keys_sum))
        var tab : table<int; int>
        for (i in range(5)) {
            tab[i] = i
        }
        t |> equal(keys_sum(tab), 10)
        var e : table<int; int>
        t |> equal(keys_sum(e), 0)
        delete tab
        delete e
    }
    t |> run("values walk mutates by ref") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@values_bump))
        var tab : table<int; int>
        for (i in range(4)) {
            tab[i] = i * 10
        }
        values_bump(tab)
        let r = kv_sums(tab)
        t |> equal(r.vs, 60 + 4)
        delete tab
    }
    t |> run("break exits the walk and unlocks") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@keys_break_at))
        var tab : table<int; int>
        for (i in range(8)) {
            tab[i] = i
        }
        t |> equal(keys_break_at(tab, 3), 3)
        tab[100] = 1   // table must be unlocked after the break
        t |> equal(length(tab), 9)
        delete tab
    }
    t |> run("string keys keep the generic path") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@string_kv_sum))
        var tab : table<string; int>
        tab["a"] = 1
        tab["b"] = 2
        tab["c"] = 4
        t |> equal(string_kv_sum(tab), 7)
        delete tab
    }
    t |> run("int64 keys ride the walk") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@keys64_sum))
        var tab : table<int64; int>
        tab[10l] = 1
        tab[20l] = 2
        tab[30l] = 3
        t |> equal(keys64_sum(tab), 60l)
        delete tab
    }
    t |> run("insert during locked iteration panics") @(t : T?) {
        // no delete: the panic leaves the table locked by design (panic is not resumable);
        // the context teardown reclaims it — same shape as array.das's locked-resize test
        var tab : table<int; int>
        for (i in range(5)) {
            tab[i] = i
        }
        t |> success(insert_during_iteration(tab))
    }
}
