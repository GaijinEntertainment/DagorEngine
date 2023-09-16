let
    g_a <- [{ auto 1;2;3 }]
    g_b := g_a

[export]
def test
    assert(length(g_a)==3 && length(g_b)==3)
    for x,y in g_a,g_b
        assert(x==y)
    let a <- [{ auto 1;2;3 }]
    let b := a
    assert(length(a)==3 && length(b)==3)
    for x,y in a,b
        assert(x==y)
    return true

