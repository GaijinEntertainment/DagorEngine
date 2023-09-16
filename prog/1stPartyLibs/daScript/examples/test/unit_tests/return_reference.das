// options log_stack=true

var
    g_t = 2

[unsafe_operation]
def ret_ref : auto &
    var qq : auto & = g_t
    unsafe
        return qq

[unsafe_operation]
def ret_safe_ref(var a:auto&) : auto&
    unsafe
        return a

def add2(bl:block<int&>)
    invoke(bl) += 2

[export]
def test
    unsafe
        var x = ret_ref()
        var xx : int& = ret_safe_ref(x)
        assert(xx==2)
        var z : int & = ret_ref()
        assert(x==2 && z==2)
        ret_ref() = 3
        assert(x==2 && z==3)
        xx = 4
        assert(x==4 && z==3 && xx==4)
        add2() <| $():auto&
            unsafe
                return x
        assert(x==6)
    return true
