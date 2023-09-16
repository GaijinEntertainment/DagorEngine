def fn_with_default_null_ptr(var a : int? implicit = [[int?]])
    var b : int
    a ?? b = 2

[export]
def test:bool
    let t = [[ int 1; 2; 3; 4; 5 ]]
    unsafe
        let pt = addr(t[0])
        assert(deref(pt)==1)
        assert(pt[0]==1 && pt[1]==2 && pt[2]==3 && pt[3]==4 && pt[4]==5)
    fn_with_default_null_ptr()
    return true

