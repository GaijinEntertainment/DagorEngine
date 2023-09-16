require _module_b

var
    global_1 = "local"
    global_3 = "local"
    global_4 = "local"

def fun_1
    return "local"

def fun_3
    return "local"

def fun_4
    return "local"

def fun_5
    return "local"

[export]
def test

    verify(::global_1=="local")
    //verify(module_a::global_1=="from_a")  // 30305: can't locate variable module_a::global_1, not visible
    verify(_module_b::global_1=="from_b")
    //verify(global_2=="from_a")            // 30305: can't locate variable global_2, not visible
    //verify(from_a_get_3()=="from_a")      // 30304: no matching functions or generics from_a_get_3, not visible
    verify(from_b_get_3()=="from_a")
    verify(from_b_get_4()=="from_b")
    verify(global_3=="local")


    verify(::fun_1()=="local")
    // verify(module_a::fun_1()=="from_a")  // 30304: no matching functions or generics module_a::fun_1, not visible
    verify(_module_b::fun_1()=="from_b")
    // verify(fun_2()=="from a")            // 30304: no matching functions or generics fun_2, not visible
    // verify(from_a_get_fun_3()=="from_a") // 30304: no matching functions or generics from_a_get_fun_3
    verify(from_b_get_fun_3()=="from_a")
    verify(fun_3()=="local")

    verify(from_b_get_fun_4()=="local")
    verify(from_b_get_fun_5()=="local")
    verify(from_b_get_own_fun_5()=="from_b")

    return true
