// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins container interop: array<T[N]> and table<K; T[N]> operations. These exact
// behaviors must survive Stage 4 when the dedicated `[]` overload families in
// builtin.das are deleted in favor of the base generics.
// NOTE deliberately absent: get_key(table<K;V[]>, v) — broken on master
// (builtin.das:1407 overload fails to resolve its own valT[-2] contortion);
// covered by the target spec instead.
options gen2
require dastest/testing_boost public

def make_iota() : int[4] {
    var r : int[4]
    for (i in range(4)) {
        r[i] = i + 1
    }
    return r
}

[test]
def test_array_of_fa(t : T?) {
    t |> run("push / push_clone / emplace") @(t : T?) {
        var v = make_iota()
        var arr : array<int[4]>
        arr |> push(v)
        arr |> push_clone(v)
        var v2 : int[4]
        v2[0] = 99
        arr |> emplace(v2)
        t |> equal(length(arr), 3)
        t |> equal(arr[0][0], 1)
        t |> equal(arr[1][3], 4)
        t |> equal(arr[2][0], 99)
        // push copied — source intact
        t |> equal(v[0], 1)
        delete arr
    }
    t |> run("mutate element in place") @(t : T?) {
        var arr : array<int[4]>
        arr |> push(make_iota())
        arr[0][2] = 77
        t |> equal(arr[0][2], 77)
        delete arr
    }
    t |> run("iterate array of FA") @(t : T?) {
        var arr : array<int[4]>
        arr |> push(make_iota())
        arr |> push(make_iota())
        var total = 0
        for (fa in arr) {
            for (x in fa) {
                total += x
            }
        }
        t |> equal(total, 20)
        delete arr
    }
}

[test]
def test_table_of_fa(t : T?) {
    t |> run("insert / get / get_value") @(t : T?) {
        var tab : table<string; int[4]>
        var v = make_iota()
        tab |> insert("a", v)
        var found = false
        tab |> get("a") $(p) {
            found = true
            t |> equal(p[0], 1)
            t |> equal(p[3], 4)
        }
        t |> success(found)
        let gv = tab |> get_value("a")
        t |> equal(gv[1], 2)
        delete tab
    }
    t |> run("mutating get writes through") @(t : T?) {
        var tab : table<string; int[4]>
        tab |> insert("k", make_iota())
        tab |> get("k") $(var p) {
            p[0] = 555
        }
        t |> equal((tab |> get_value("k"))[0], 555)
        delete tab
    }
    t |> run("values() iterates FA refs") @(t : T?) {
        var tab : table<string; int[4]>
        tab |> insert("a", make_iota())
        var count = 0
        for (val in values(tab)) {
            count ++
            t |> equal(val[0], 1)
        }
        t |> equal(count, 1)
        delete tab
    }
    t |> run("insert_clone") @(t : T?) {
        var tab : table<string; int[4]>
        var v = make_iota()
        tab |> insert_clone("c", v)
        v[0] = 1000
        t |> equal((tab |> get_value("c"))[0], 1)
        delete tab
    }
}

[test]
def test_struct_with_fa_in_containers(t : T?) {
    t |> run("struct holding FA inside array") @(t : T?) {
        var arr : array<tuple<name : string; data : float[2]>>
        var tp : tuple<name : string; data : float[2]>
        tp.name = "p"
        tp.data[1] = 3.5
        arr |> push(tp)
        t |> equal(arr[0].data[1], 3.5)
        delete arr
    }
}
