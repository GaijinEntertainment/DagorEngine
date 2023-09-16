def test_find
    let const_tab <- {{ "one"=>1; "two"=>2 }}
    find(const_tab,"one") <| $(num)
        assert(deref(num)==1)
    get(const_tab,"one") <| $(num)
        assert(num==1)
    find(const_tab,"one") <| $(num)
        if num != null
            assert(deref(num)==1)
    assert(const_tab?["one"] ?? 100500 == 1)
    var tab <- {{ "one"=>1; "two"=>2 }}
    find(tab,"one") <| $(num)
        assert(deref(num)==1)
    get(tab,"one") <| $(var num)
        num = 3
    assert(tab?["one"] ?? 100500 == 3)
    get(tab,"one") <| $(var num)
        assert(num==3)
        num = 4
    assert(tab?["one"] ?? 100500 == 4)
    get(tab,"one") <| $(var num)
        assert(num==4)
        num = 5
    assert(tab?["one"] ?? 100500 == 5)
    unsafe
        var dummy = 0
        tab?["one"] ?? dummy = 1
        assert(tab?["one"] ?? 100500==1)
        assert(dummy==0)
    return true

[export]
def test : bool
    test_find()

    var tab:table<string;int>
    var i,del:int
    let total:int=32
    verify(!erase(tab,"0"))
    i = 0
    while i!=total
        // tab[string(i)] = i++
        tab[string(i)] = i
        i++
    assert(length(tab)==total)
    // debug(tab)
    i = 0
    while i!=total
        verify(tab[string(i)]==i)
        i++
    assert(length(tab)==total)
    // debug(tab)
    i = 0
    del = 0
    while i < total
        verify(erase(tab,string(i)))
        verify(!erase(tab,string(i)))
        del ++
        i += 7
    assert(length(tab)==total-del)
    // debug(tab)
    i = 0
    while i < total
        find(tab,string(i)) <| $(pValue)
            if pValue!=null
                assert(deref(pValue)==i)
            else
                assert(i%7==0)
        i ++
    while i < total
        let pValue = unsafe(tab?[string(i)])
        if pValue!=null
            assert(deref(pValue)==i)
        else
            assert(i%7==0)
        i ++
    if true
        var j,k:int=0
        i = 0
        while i < total
            find(tab,string(i)) <| $(pVal)
                j+= pVal ?? 0
            if i%7!=0
                k +=i
            else
                assert(i%7==0)
            i ++
        assert(j==k)
    if true
        var j,k,zero:int=0
        i = 0
        while i < total
            find(tab,string(i)) <| $(pVal)
                j += pVal ?? zero
            if i%7!=0
                k +=i
            i ++
        assert(j==k)
    // debug(tab)
    i = 0
    while i < total
        tab[string(i)] = i
        i += 7
    assert(length(tab)==total)
    // debug(tab)
    i = 0
    while i!=total
        verify(tab[string(i)]==i)
        i++
    assert(length(tab)==total)
    // debug(tab)
    var cnt = 0
    for k,v in keys(tab),values(tab)
        // debug(k,"key=")
        // debug(v,"value=")
        assert(k==string(v))
        cnt ++
    assert(cnt==total)
    verify(erase(tab,"1"))
    verify(!erase(tab,"-1"))


    var tab_bool:table<bool;int>
    assert(!key_exists(tab_bool, false))
    assert(!key_exists(tab_bool, true))
    tab_bool[true] = 0
    assert(key_exists(tab_bool, true))
    tab_bool[false] = -1
    assert(key_exists(tab_bool, false))

    var tab_int:table<int;int>
    assert(!key_exists(tab_int, -1))
    tab_int[-1] = -1
    assert(key_exists(tab_int, -1))
    erase(tab_int, -1)
    assert(!key_exists(tab_int, -1))

    var tab_string:table<string;int>
    assert(!key_exists(tab_string, "---"))
    tab_string["---"] = -1
    assert(key_exists(tab_string, "---"))
    erase(tab_string, "---")
    assert(!key_exists(tab_string, "---"))
    assert(!key_exists(tab_string, " "))
    tab_string[" "] = -1
    assert(key_exists(tab_string, " "))
    erase(tab_string, " ")
    assert(!key_exists(tab_string, " "))
    tab_string[""] = -2
    assert(key_exists(tab_string, ""))
    verify(tab_string[""] == -2)
    erase(tab_string, "")
    assert(!key_exists(tab_string, ""))

    test_erase_collision()
    test_lock_panic()

    return true

[sideeffects]
def test_lock_panic
    var tab <- {{ "one" => 1; "two" => 2 }}
    var failed = false
    try
        find(tab,"one") <| $(t)
            assert(t!=null)
            tab["three"] = 3
    recover
        failed = true
    assert(failed)
    failed = false
    try
        find(tab,"three") <| $(t)
            assert(t==null)
            erase(tab,"three")
    recover
        failed = true
    assert(failed)
    return true

let
    TOTAL = 100

struct TestStruct
    test : string

struct TableContainer
    t : table<string; TestStruct>

def check(var t : TableContainer; deletedEntries : array<int>)
    for i in range (1, TOTAL)
        var deleted : bool
        for x in deletedEntries
            if x == i
                deleted = true
                break
        if deleted
            continue
        let n = "name_{i}"
        if t.t[n].test != n
            return false
    return true

[sideeffects]
def test_erase_collision
    var t : TableContainer
    var names : array<string>
    for i in range(1, TOTAL)
        push(names, "name_{i}")
    for n in names
        t.t[n] <- [[TestStruct test = n ]]
    var deletedEntries : array<int>
    verify(check(t, deletedEntries))
    for i in range(1, TOTAL)
        let x = "name_{i}"
        push(deletedEntries, i)
        erase(t.t, x)
        verify(check(t, deletedEntries))
