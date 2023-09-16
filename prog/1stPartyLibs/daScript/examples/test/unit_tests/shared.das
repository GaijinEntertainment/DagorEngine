let shared
    g_foo <- {{ "one" => 1; "two" => 2 }}

[export]
def test
    lock(g_foo) <| $ ( foo )
        assert((foo?["one"] ?? 0)==1)
        assert((foo?["two"] ?? 0)==2)
        assert((foo?["three"] ?? 0)==0)
    return true
