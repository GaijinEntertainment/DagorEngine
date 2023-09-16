require rtti

[export]
def test:bool
    verify(is_same_type(typeinfo(rtti_typeinfo 1),typeinfo(rtti_typeinfo 2)))
    verify(!is_same_type(typeinfo(rtti_typeinfo 1.0),typeinfo(rtti_typeinfo 2)))

    let v : variant<i:int; f:float>
    let vinfo & = unsafe(typeinfo(rtti_typeinfo v))
    assert(vinfo.argCount==2u)
    for i,n,t in range(2),arg_names(vinfo),arg_types(vinfo)
        if i==0
            assert(n=="i")
            verify(is_same_type(typeinfo(rtti_typeinfo 1),t,RefMatters no,ConstMatters no))
        elif i==1
            assert(n=="f")
            verify(is_same_type(typeinfo(rtti_typeinfo 1.0),t,RefMatters no,ConstMatters no))

    return true
