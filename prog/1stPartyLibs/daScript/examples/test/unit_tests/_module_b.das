module _module_b

require _module_a

var
    global_1 = "from_b"    // colides with module_a::global_1
    global_4 = "from_b"

[generic]
def from_b_get_3()
    return global_3

[generic]
def from_b_get_4()
    return global_4

def fun_1
    return "from_b"

def fun_5
    return "from_b"

def from_b_get_fun_3()
    return fun_3()

[generic]
def from_b_get_fun_4()
    return  _::fun_4()

[generic]
def from_b_get_fun_5()
    return  __::fun_5()

[generic]
def from_b_get_own_fun_5()
    return ::fun_5()

[export]
def test
    return true
