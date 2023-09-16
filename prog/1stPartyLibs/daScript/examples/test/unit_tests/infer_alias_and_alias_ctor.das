// options log=true,log_infer_passes=true,optimize=true

def join(a:array<auto(numt)>)
    var t : numt = numt(0)
    for x in a
        t += x
    return t

[unsafe_operation]
def findObject(objects : table<string; auto(T)>; guid : string): T?
    var foundObj : T?
    get(objects, guid) <| $(obj)
        unsafe
            foundObj = reinterpret<T?> addr(obj)
    return foundObj

[export]
def test:bool
    var a:array<float>
    push(a,1.0)
    push(a,2.0)
    push(a,3.0)
    let t = join(a)
    assert(t==6.0)
    let tab <- {{ "one" => 1; "two" => 2 }}
    unsafe
        verify(findObject(tab,"one") ?? 0==1)
        verify(findObject(tab,"two") ?? 0==2)
    return true
