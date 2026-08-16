options gen2

require dastest/testing_boost public

struct Pt {
    x : int
    y : int
}

[test]
def test_each_kv(t : T?) {
    t |> run("int keys, int values") @(t : T?) {
        var tab : table<int; int>
        for (i in range(10)) {
            tab |> insert(i, i * 10)
        }
        var n = 0
        var ksum = 0
        var vsum = 0
        for (kv in each_kv(tab)) {
            n++
            ksum += kv.key
            vsum += kv.value
        }
        t |> equal(n, 10)
        t |> equal(ksum, 45)
        t |> equal(vsum, 450)
    }
    t |> run("string keys, float values") @(t : T?) {
        var tab : table<string; float>
        tab |> insert("a", 1.5)
        tab |> insert("b", 2.5)
        var s = 0.0
        for (kv in each_kv(tab)) {
            s += kv.value
        }
        t |> equal(s, 4.0)
    }
    t |> run("struct values") @(t : T?) {
        var tab : table<int; Pt>
        tab |> insert(1, Pt(x = 10, y = 20))
        tab |> insert(2, Pt(x = 30, y = 40))
        var xs = 0
        var ys = 0
        for (kv in each_kv(tab)) {
            xs += kv.value.x
            ys += kv.value.y
        }
        t |> equal(xs, 40)
        t |> equal(ys, 60)
    }
    t |> run("empty table yields nothing") @(t : T?) {
        let tab : table<int; int>
        var n = 0
        for (_kv in each_kv(tab)) {
            n++
        }
        t |> equal(n, 0)
    }
    t |> run("agrees with zipped keys/values") @(t : T?) {
        var tab : table<int; string>
        tab |> insert(1, "one")
        tab |> insert(2, "two")
        tab |> insert(3, "three")
        var roundTrip : table<int; string>
        for (kv in each_kv(tab)) {
            roundTrip |> insert(kv.key, kv.value)
        }
        t |> equal(length(roundTrip), length(tab))
        for (k, v in keys(tab), values(tab)) {
            t |> success(key_exists(roundTrip, k))
            t |> equal(roundTrip?[k] ?? "", v)
        }
    }
    t |> run("element is a copy — table unchanged") @(t : T?) {
        var tab : table<int; Pt>
        tab |> insert(1, Pt(x = 10, y = 20))
        for (kv in each_kv(tab)) {
            var local = kv
            local.value.x = 999
        }
        t |> equal((tab?[1] ?? Pt()).x, 10)
    }
}
