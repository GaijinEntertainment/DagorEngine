def safe_addr(var a)
    unsafe
        return addr(a)

def safe_addr_val(var a : auto&)
    unsafe
        return addr(a)

[export]
def test()

    var ar : array<int>
    var dummy : int
    ar?[0] ?? dummy = 1
    assert(dummy==1)
    resize(ar,1)
    ar?[0] ?? dummy = 2
    assert(length(ar)==1 && ar[0]==2)
    assert(dummy==1)

    var par = safe_addr(ar)
    clear(ar)
    dummy = 0
    par?[0] ?? dummy = 1
    assert(dummy==1)
    resize(ar,1)
    par?[0] ?? dummy = 2
    assert(length(ar)==1 && ar[0]==2)
    assert(dummy==1)
    par = null
    par?[0] ?? dummy = 3
    assert(dummy==3)

    var ta : table<string;int>
    dummy = 0
    ta?["one"] ?? dummy = 1
    assert(dummy==1)
    ta |> insert("one",0 )
    ta?["one"] ?? dummy = 2
    verify(length(ta)==1 && ta |> get_value("one")==2)
    assert(dummy==1)

    var pta = safe_addr(ta)
    clear(ta)
    dummy = 0
    pta?["one"] ?? dummy = 1
    assert(dummy==1)
    assert(length(ta)==0)
    ta |> insert("one",1)
    pta?["one"] ?? dummy = 2
    verify(length(ta)==1 && ta |> get_value("one")==2)
    assert(dummy==1)
    pta = null
    pta?["one"] ?? dummy = 3
    assert(dummy==3)

    var di : int[2]
    dummy = 0
    di?[3] ?? dummy = 1
    assert(dummy==1)
    di?[0] ?? dummy = 2
    assert(di[0]==2)
    assert(dummy==1)

    var pdi = safe_addr(di)
    dummy = 0
    pdi?[3] ?? dummy = 1
    assert(dummy==1)
    di[0] = 0
    pdi?[0] ?? dummy = 2
    assert(di[0]==2)
    assert(dummy==1)
    pdi = null
    pdi?[0] ?? dummy = 3
    assert(dummy==3)

    var vec = int3(1,2,3)
    dummy = 0
    vec?[3] ?? dummy = 1
    assert(dummy==1)
    vec?[0] ?? dummy = 2
    assert(vec[0]==2)
    assert(dummy==1)

    var pvec = safe_addr_val(vec)
    dummy = 0
    pvec?[3] ?? dummy = 1
    assert(dummy==1)
    vec[0] = 0
    pvec?[0] ?? dummy = 2
    assert(vec[0]==2)
    assert(dummy==1)
    pvec = null
    pvec?[0] ?? dummy = 3
    assert(dummy==3)

    return true
