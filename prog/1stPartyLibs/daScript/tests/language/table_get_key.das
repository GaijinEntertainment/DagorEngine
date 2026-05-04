options gen2

require dastest/testing_boost public

[test]
def test_get_key_int_float(t : T?) {
    t |> run("basic int->float table") @(t : T?) {
        var tab : table<int; float>
        tab[10] = 1.0
        tab[20] = 2.0
        tab[30] = 3.0
        var sum_keys = 0
        var sum_vals = 0.0
        for (v in values(tab)) {
            let k = get_key(tab, v)
            sum_keys += k
            sum_vals += v
        }
        t |> equal(sum_keys, 60)
        t |> equal(sum_vals, 6.0)
    }
}

[test]
def test_get_key_string_int(t : T?) {
    t |> run("string->int table") @(t : T?) {
        var tab : table<string; int>
        tab["alpha"] = 1
        tab["beta"] = 2
        var found : table<string; int>
        for (v in values(tab)) {
            let k = get_key(tab, v)
            found[k] = v
        }
        t |> equal(found["alpha"], 1)
        t |> equal(found["beta"], 2)
    }
}

[test]
def test_get_key_const_table(t : T?) {
    t |> run("const table parameter") @(t : T?) {
        var tab : table<int; int>
        tab[100] = 42
        check_const(t, tab)
    }
}

def check_const(t : T?; tab : table<int; int>) {
    for (v in values(tab)) {
        let k = get_key(tab, v)
        t |> equal(k, 100)
        t |> equal(v, 42)
    }
}

[test]
def test_get_key_after_erase(t : T?) {
    t |> run("tombstones after erase") @(t : T?) {
        var tab : table<int; int>
        for (i in range(10)) {
            tab[i] = i * 10
        }
        // Erase some entries to create tombstones
        tab |> erase(3)
        tab |> erase(5)
        tab |> erase(7)
        var count = 0
        for (v in values(tab)) {
            let k = get_key(tab, v)
            t |> equal(v, k * 10)
            count++
        }
        t |> equal(count, 7)
    }
}

[test]
def test_get_key_empty_table(t : T?) {
    t |> run("empty table iterates zero times") @(t : T?) {
        var tab : table<int; int>
        var count = 0
        for (v in values(tab)) {
            let _ = get_key(tab, v)
            count++
        }
        t |> equal(count, 0)
    }
}

[test]
def test_get_key_single_entry(t : T?) {
    t |> run("single entry") @(t : T?) {
        var tab : table<int; string>
        tab[42] = "hello"
        for (v in values(tab)) {
            let k = get_key(tab, v)
            t |> equal(k, 42)
            t |> equal(v, "hello")
        }
    }
}
