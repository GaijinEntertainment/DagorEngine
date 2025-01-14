require UnitTest

[export]
def test
    var tab : table<EntityId; string>
    tab |> insert(EntityId(1), "hello")
    tab |> insert(EntityId(2), "world")
    tab |> insert(EntityId(3), "!")
    assert(length(tab) == 3)
    tab |> erase(EntityId(3))
    assert(length(tab) == 2)

    tab |> find(EntityId(2)) <| $ ( val )
        assert(val!=null && *val=="world")

    tab |> get(EntityId(1)) <| $ ( val )
        assert(val=="hello")

    if tab |> key_exists(EntityId(1))
        assert(true)
    if tab |> key_exists(EntityId(3))
        assert(false)

    let t = tab ?[EntityId(1)] ?? "goodbye"
    assert(t=="hello")
    var ptab = unsafe(addr(tab))
    let q = ptab ?[EntityId(2)] ?? "goodbye"
    assert(q=="world")

    var count = 0
    for k,v in keys(tab),values(tab)
        if int(k) == 1
            assert(v=="hello")
        if int(k) == 2
            assert(v=="world")
        count ++
    assert(count==2)

    var set : table<EntityId>
    set |> insert(EntityId(1))
    set |> insert(EntityId(2))
    set |> insert(EntityId(3))
    assert(length(set) == 3)
    if set |> key_exists(EntityId(3))
        assert(true)
    if set |> key_exists(EntityId(4))
        assert(false)
    return true


