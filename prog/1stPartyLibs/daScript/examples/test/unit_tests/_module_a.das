module _module_a

var
    global_1 = "from a"
    global_2 = "from a"
    global_3 = "from_a"

[generic]
def from_a_get_3()
    return global_3

def fun_1
    return "from_a"

def fun_2
    return "from_a"

def fun_3
    return "from_a"

def from_a_get_fun_3()
    return fun_3()

[export]
def test
    return true
