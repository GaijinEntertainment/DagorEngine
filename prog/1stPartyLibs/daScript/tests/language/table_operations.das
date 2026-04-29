options gen2
require dastest/testing_boost public

let {
    TA_TOTAL = 100
}

struct TaTestStruct {
    test : string
}

struct TaTableContainer {
    t : table<string; TaTestStruct>
}

def ta_check(var tc : TaTableContainer; deletedEntries : array<int>) {
    for (i in range(1, TA_TOTAL)) {
        var deleted : bool
        for (x in deletedEntries) {
            if (x == i) {
                deleted = true
                break
            }
        }
        if (deleted) {
            continue
        }
        let n = "name_{i}"
        if (tc.t |> get_value(n).test != n) {
            return false
        }
    }
    return true
}

[sideeffects]
def ta_test_find(t : T?) {
    let const_tab <- { "one" => 1, "two" => 2 }
    get(const_tab, "one") <| $(num) {
        t |> equal(num, 1)
    }
    t |> equal(const_tab?["one"] ?? 100500, 1)
    var tab <- { "one" => 1, "two" => 2 }
    get(tab, "one") <| $(var num) {
        num = 3
    }
    t |> equal(tab?["one"] ?? 100500, 3)
    get(tab, "one") <| $(var num) {
        t |> equal(num, 3)
        num = 4
    }
    t |> equal(tab?["one"] ?? 100500, 4)
    get(tab, "one") <| $(var num) {
        t |> equal(num, 4)
        num = 5
    }
    t |> equal(tab?["one"] ?? 100500, 5)
    unsafe {
        var dummy = 0
        tab?["one"] ?? dummy = 1
        t |> equal(tab?["one"] ?? 100500, 1)
        t |> equal(dummy, 0)
    }
}

[sideeffects]
def ta_test_erase_collision(t : T?) {
    var tc : TaTableContainer
    var names : array<string>
    for (i in range(1, TA_TOTAL)) {
        push(names, "name_{i}")
    }
    for (n in names) {
        tc.t |> emplace(n, TaTestStruct(test = n))
    }
    var deletedEntries : array<int>
    verify(ta_check(tc, deletedEntries))
    for (i in range(1, TA_TOTAL)) {
        let x = "name_{i}"
        push(deletedEntries, i)
        erase(tc.t, x)
        verify(ta_check(tc, deletedEntries))
    }
    t |> success(true)
}

[sideeffects]
def ta_test_lock_panic(t : T?) {
    var tab <- { "one" => 1, "two" => 2 }
    var failed = false
    try {
        get(tab, "one") <| $(lv) {
            tab |> insert("three", 3)
        }
    } recover {
        failed = true
    }
    t |> success(failed)
    t |> success(!key_exists(tab, "three"))
    failed = false
    try {
        get(tab, "three") <| $(lv) {
            erase(tab, "three")
        }
    } recover {
        failed = true
    }
    t |> success(!failed)
}

[sideeffects]
def ta_test_insert_default(t : T?) {
    var td : table<string; int>
    td |> insert_default("one", 1)
    t |> equal(td?["one"] ?? 100500, 1)
    td |> insert_default("one", 2) // no effect
    t |> equal(td?["one"] ?? 100500, 1)
    t |> success(!key_exists(td, "zero"))
    td |> insert_default("zero")
    t |> equal(td?["zero"] ?? 100500, 0)

    var tmap : table<int; array<int>>
    tmap |> emplace(1, [1])
    tmap |> emplace(2, [2, 2])
    tmap |> emplace_default(1) // no effect
    tmap |> emplace_default(2) // no effect
    tmap |> emplace_default(0)
    tmap |> emplace_default(3)
    verify(length(tmap) == 4)
    for (i in range(0, 4)) {
        let found = get(tmap, i) <| $(val) {
            if (i == 0 || i == 3) {
                assert(length(val) == 0)
            } elif (i == 1) {
                assert(length(val) == 1 && val[0] == 1)
            } elif (i == 2) {
                assert(length(val) == 2 && val[0] == 2 && val[1] == 2)
            }
        }
        verify(found)
    }
    t |> success(true)
}

[sideeffects]
def ta_test_get_with_default(t : T?) {
    var tab <- { 1 => "1", 2 => "2", 3 => "3" }
    get_with_default(tab, 2) <| $(val) {
        verify(val == "2")
    }
    get_with_default(tab, 1) <| $(val) {
        verify(val == "1")
        val += " modified"
    }
    t |> equal(tab?[1] ?? "", "1 modified")
    // add new entry with default value
    get_with_default(tab, 4) <| $(val) {
        verify(val == "")
        val += "four"
    }
    t |> equal(tab?[4] ?? "", "four")
    modify(tab, 2) <| $(val) => val + " modified"
    t |> equal(tab?[2] ?? "", "2 modified")
    modify(tab, 4) <| $(val) => "'{val}'"
    t |> equal(tab?[4] ?? "", "'four'")
    // modify non-existing entry, add it with default value
    modify(tab, 5) <| $(val) => "{val}five"
    t |> equal(tab?[5] ?? "", "five")
}

[test]
def test_table_operations(t : T?) {
    t |> run("table find and get") @(t : T?) {
        ta_test_find(t)
    }
    t |> run("table insert delete verify") @(t : T?) {
        var tab : table<string; int>
        let total : int = 32
        verify(!erase(tab, "0"))
        var i = 0
        while (i != total) {
            tab |> insert(string(i), i)
            i++
        }
        t |> equal(length(tab), total)
        i = 0
        while (i != total) {
            verify(tab |> get_value(string(i)) == i)
            i++
        }
        t |> equal(length(tab), total)
        var del = 0
        i = 0
        while (i < total) {
            verify(erase(tab, string(i)))
            verify(!erase(tab, string(i)))
            del ++
            i += 7
        }
        t |> equal(length(tab), total - del)
        i = 0
        while (i < total) {
            let found = get(tab, string(i)) <| $(pValue) {
                assert(pValue == i)
            }
            assert(!found == (i % 7 == 0))
            i ++
        }
        {
            var j = 0
            var k = 0
            i = 0
            while (i < total) {
                get(tab, string(i)) <| $(pVal) {
                    j += pVal
                }
                if (i % 7 != 0) {
                    k += i
                } else {
                    assert(i % 7 == 0)
                }
                i ++
            }
            t |> equal(j, k)
        }
        i = 0
        while (i < total) {
            tab |> insert(string(i), i)
            i += 7
        }
        t |> equal(length(tab), total)
        i = 0
        while (i != total) {
            verify(tab |> get_value(string(i)) == i)
            i++
        }
        t |> equal(length(tab), total)
        var cnt = 0
        for (ky, vl in keys(tab), values(tab)) {
            assert(ky == string(vl))
            cnt ++
        }
        t |> equal(cnt, total)
        verify(erase(tab, "1"))
        verify(!erase(tab, "-1"))
    }
    t |> run("table key_exists for various types") @(t : T?) {
        var tab_bool : table<bool; int>
        t |> success(!key_exists(tab_bool, false))
        t |> success(!key_exists(tab_bool, true))
        tab_bool |> insert(true, 0)
        t |> success(key_exists(tab_bool, true))
        tab_bool |> insert(false, -1)
        t |> success(key_exists(tab_bool, false))

        var tab_rng = {0 .. 5}
        t |> success(key_exists(tab_rng, 0 .. 5))

        var tab_int : table<int; int>
        t |> success(!key_exists(tab_int, -1))
        tab_int |> insert(-1, -1)
        t |> success(key_exists(tab_int, -1))
        erase(tab_int, -1)
        t |> success(!key_exists(tab_int, -1))

        var tab_string : table<string; int>
        t |> success(!key_exists(tab_string, "---"))
        tab_string |> insert("---", -1)
        t |> success(key_exists(tab_string, "---"))
        erase(tab_string, "---")
        t |> success(!key_exists(tab_string, "---"))
        t |> success(!key_exists(tab_string, " "))
        tab_string |> insert(" ", -1)
        t |> success(key_exists(tab_string, " "))
        erase(tab_string, " ")
        t |> success(!key_exists(tab_string, " "))
        tab_string |> insert("", -2)
        t |> success(key_exists(tab_string, ""))
        verify(tab_string |> get_value("") == -2)
        erase(tab_string, "")
        t |> success(!key_exists(tab_string, ""))
    }
    t |> run("table erase collision") @(t : T?) {
        ta_test_erase_collision(t)
    }
    t |> run("table lock panic") @(t : T?) {
        ta_test_lock_panic(t)
    }
    t |> run("table insert_default and emplace_default") @(t : T?) {
        ta_test_insert_default(t)
    }
    t |> run("table get_with_default and modify") @(t : T?) {
        ta_test_get_with_default(t)
    }
}
