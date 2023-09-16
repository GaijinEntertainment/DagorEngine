
require trivial_value

[export]
def test
    let i_val = trivial_value(1)
    let i <- invoke(i_val,13)
    print("i = {i}\n")
    let s_val = trivial_value("foo")
    let s <- invoke(s_val,13)
    print("s = {s}\n")
    return true
