// options print_ref = true, print_var_access = true, log = true// , log_optimization_passes = true

def strcat(a,b)
    var c = a
    c += b                  // this will not prevent folding
    return c

def make_name(x)
    return strcat("hello_",x)

var
    g_cond = true

def make_another_name(x)
    assert(x=="modify_arg")
    if g_cond
        print("{x}")// this will prevent folding
    return x + "_ok"

let
    g_name = "_global_ok"

def make_g_name(x)
    return x + g_name

struct Foo
    a : int = 19
    b : float = 20.

def up_foo(var a)
    a.a ++

def name_foo()
    var t = Foo()
    up_foo(t)
    return "{t}"        // this will fold

def stil_not_writing(var a:string&;b:string&)
    assert(a=="blah")
    assert(b=="_global_ok")
    a = "it"

[export]
def test
    g_cond = false
    let t = make_name("world")
    assert("{t}"=="hello_world")
    let q = make_another_name("modify_arg")
    assert("{q}"=="modify_arg_ok")
    let w = name_foo()
    print("w = {w}\n")
    assert("{w}"=="[[ 20; 20.000000000]]")
    let g = make_g_name("touch_global")
    assert("{g}"=="touch_global_global_ok")
    var T = "blah"
    stil_not_writing(T,g_name)
    assert(T=="it")
    let tt = "_global_ok"
    T = "blah"
    stil_not_writing(T,tt)
    return true
