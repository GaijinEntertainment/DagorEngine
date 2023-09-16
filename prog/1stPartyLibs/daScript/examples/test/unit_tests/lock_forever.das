let
    num_2_name <- lock_forever ( {{
        0 => "zero";
        1 => "one";
        2 => "two"
    }} )

[export]
def test
    let n0 = num_2_name?[0] ?? "0"
    assert(n0=="zero")
    let n1 = num_2_name?[1] ?? "1"
    assert(n1=="one")
    let n2 = num_2_name?[2] ?? "2"
    assert(n2=="two")
    let n3 = num_2_name?[3] ?? "3"
    assert(n3=="3")
    return true
