require strings

[sideeffects]
def checkz ( txt:string; fn:function<(arg:string):auto(TT)> )
    let t = invoke(fn, txt)
    assert(t == [[TT]])


[export]
def test
    checkz("str") <| @@(x:string) => to_int(x)
    checkz("str") <| @@(x:string) => to_uint(x)
    checkz("str") <| @@(x:string) => to_int64(x)
    checkz("str") <| @@(x:string) => to_uint64(x)
    checkz("str") <| @@(x:string) => to_float(x)
    checkz("str") <| @@(x:string) => to_double(x)
    return true
