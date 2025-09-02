options gen2
def test_find {
    let const_tab <- { "one" => 1, "two" => 2 }
    get(const_tab, "one") <| $(num) {
        assert(num == 1)
    }
    assert(const_tab?["one"] ?? 100500 == 1)
    var tab <- { "one" => 1, "two" => 2 }
    get(tab, "one") <| $(var num) {
        num = 3
    }
    assert(tab?["one"] ?? 100500 == 3)
    get(tab, "one") <| $(var num) {
        assert(num == 3)
        num = 4
    }
    assert(tab?["one"] ?? 100500 == 4)
    get(tab, "one") <| $(var num) {
        assert(num == 4)
        num = 5
    }
    assert(tab?["one"] ?? 100500 == 5)
    unsafe {
        var dummy = 0
        tab?["one"] ?? dummy = 1
        assert(tab?["one"] ?? 100500 == 1)
        assert(dummy == 0)
    }
    return true
}

[export]
def test : bool {
    test_find()

    var tab : table<string; int>
    var i, del : int
    let total : int = 32
    verify(!erase(tab, "0"))
    i = 0
    while (i != total) {
        // tab[string(i)] = i++
        tab |> insert(string(i), i)
        i++
    }
    assert(length(tab) == total)
    // debug(tab)
    i = 0
    while (i != total) {
        verify(tab |> get_value(string(i)) == i)
        i++
    }
    assert(length(tab) == total)
    // debug(tab)
    i = 0
    del = 0
    while (i < total) {
        verify(erase(tab, string(i)))
        verify(!erase(tab, string(i)))
        del ++
        i += 7
    }
    assert(length(tab) == total - del)
    // debug(tab)
    i = 0
    while (i < total) {
        let found = get(tab, string(i)) <| $(pValue) {
            assert(pValue == i)
        }
        assert(!found == (i % 7 == 0))
        i ++
    }
    while (i < total) {
        let pValue = unsafe(tab?[string(i)])
        if (pValue != null) {
            assert(deref(pValue) == i)
        } else {
            assert(i % 7 == 0)
        }
        i ++
    }
    if (true) {
        var j, k : int = 0
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
        assert(j == k)
    }
    // debug(tab)
    i = 0
    while (i < total) {
        tab |> insert(string(i), i)
        i += 7
    }
    assert(length(tab) == total)
    // debug(tab)
    i = 0
    while (i != total) {
        verify(tab |> get_value(string(i)) == i)
        i++
    }
    assert(length(tab) == total)
    // debug(tab)
    var cnt = 0
    for (k, v in keys(tab), values(tab)) {
        // debug(k,"key=")
        // debug(v,"value=")
        assert(k == string(v))
        cnt ++
    }
    assert(cnt == total)
    verify(erase(tab, "1"))
    verify(!erase(tab, "-1"))


    var tab_bool : table<bool; int>
    assert(!key_exists(tab_bool, false))
    assert(!key_exists(tab_bool, true))
    tab_bool |> insert(true, 0)
    assert(key_exists(tab_bool, true))
    tab_bool |> insert(false, -1)
    assert(key_exists(tab_bool, false))

    var tab_rng = {0 .. 5};
    assert(key_exists(tab_rng, 0 .. 5))

    var tab_int : table<int; int>
    assert(!key_exists(tab_int, -1))
    tab_int |> insert(-1, -1)
    assert(key_exists(tab_int, -1))
    erase(tab_int, -1)
    assert(!key_exists(tab_int, -1))

    var tab_string : table<string; int>
    assert(!key_exists(tab_string, "---"))
    tab_string |> insert("---", -1)
    assert(key_exists(tab_string, "---"))
    erase(tab_string, "---")
    assert(!key_exists(tab_string, "---"))
    assert(!key_exists(tab_string, " "))
    tab_string |> insert(" ", -1)
    assert(key_exists(tab_string, " "))
    erase(tab_string, " ")
    assert(!key_exists(tab_string, " "))
    tab_string |> insert("", -2)
    assert(key_exists(tab_string, ""))
    verify(tab_string |> get_value("") == -2)
    erase(tab_string, "")
    assert(!key_exists(tab_string, ""))

    test_erase_collision()
    test_lock_panic()
    test_insert_default()
    test_get_with_default()

    return true
}

[sideeffects]
def test_lock_panic {
    var tab <- { "one" => 1, "two" => 2 }
    var failed = false
    try {
        get(tab, "one") <| $(t) {
            tab |> insert("three", 3)
        }
    } recover {
        failed = true
    }
    assert(failed)
    assert(!key_exists(tab, "three"))
    failed = false
    try {
        get(tab, "three") <| $(t) {
            erase(tab, "three")
        }
    } recover {
        failed = true
    }
    assert(!failed)
    return true
}

let {
    TOTAL = 100
}

struct TestStruct {
    test : string
}

struct TableContainer {
    t : table<string; TestStruct>
}

def check(var t : TableContainer; deletedEntries : array<int>) {
    for (i in range(1, TOTAL)) {
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
        if (t.t |> get_value(n).test != n) {
            return false
        }
    }
    return true
}

[sideeffects]
def test_erase_collision {
    var t : TableContainer
    var names : array<string>
    for (i in range(1, TOTAL)) {
        push(names, "name_{i}")
    }
    for (n in names) {
        t.t |> emplace(n, TestStruct(test = n))
    }
    var deletedEntries : array<int>
    verify(check(t, deletedEntries))
    for (i in range(1, TOTAL)) {
        let x = "name_{i}"
        push(deletedEntries, i)
        erase(t.t, x)
        verify(check(t, deletedEntries))
    }
}

[sideeffects]
def test_insert_default() {
    var t : table<string, int>
    t.insert_default("one", 1)
    assert(t?["one"] ?? 100500 == 1)
    t.insert_default("one", 2) // no effect
    assert(t?["one"] ?? 100500 == 1)

    assert(!t.key_exists("zero"))
    t.insert_default("zero")
    assert(t?["zero"] ?? 100500 == 0)

    var map : table<int, array<int>>
    map.emplace(1, [1])
    map.emplace(2, [2, 2])
    map.emplace_default(1) //no effect
    map.emplace_default(2) //no effect
    map.emplace_default(0)
    map.emplace_default(3)
    verify(map.length() == 4)
    for (i in range(0, 4)) {
        let found = map.get(i, $(val) {
            if (i == 0 || i == 3) {
                assert(val.length() == 0)
            } elif (i == 1) {
                assert(val.length() == 1 && val[0] == 1)
            } elif (i == 2) {
                assert(val.length() == 2 && val[0] == 2 && val[1] == 2)
            }
        })
        verify(found)
    }

    return true
}

[sideeffects]
def test_get_with_default() {
    var tab <- { 1 => "1", 2 => "2", 3 => "3" }

    get_with_default(tab, 2, $(val) {
        verify(val == "2")
    })
    get_with_default(tab, 1, $(val) {
        verify(val == "1")
        val += " modified"
    })
    assert(tab?[1] ?? "" == "1 modified")

    // add new entry with default value
    get_with_default(tab, 4, $(val) {
        verify(val == "")
        val += "four"
    })
    assert(tab?[4] ?? "" == "four")

    modify(tab, 2, $(val) => val + " modified")
    assert(tab?[2] ?? "" == "2 modified")

    modify(tab, 4, $(val) => "'{val}'")
    assert(tab?[4] ?? "" == "'four'")

    // modify non-existing entry, add it with default value
    modify(tab, 5, $(val) => "{val}five")
    assert(tab?[5] ?? "" == "five")

    return true
}