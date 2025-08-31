options gen2
enum DefEnum {
    foo = 0
    bar
}

struct DefStruct {
    x : int
    y : float
}

[export]
def test {
    let t_int = int()
    assert(t_int == 0)
    let t_string = string("")
    assert(t_string == "")
    let t_tuple = tuple<int; float>()
    assert(t_tuple._0 == 0 && t_tuple._1 == 0.0)
    let t_variant = variant<a : int; b : float>()
    assert((t_variant is a) && (t_variant as a == 0))
    let t_struct = DefStruct()
    assert(t_struct.x == 0 && t_struct.y == 0.0)
    let t_enum = default<DefEnum>
    assert(t_enum == DefEnum.foo)
    let t_array <- array<int>()
    assert(length(t_array) == 0)
    let t_array_struct = array struct<DefStruct>((x = 1), (x = 2))
    assert(length(t_array_struct) == 2)
    assert(t_array_struct[0].x == 1)
    assert(t_array_struct[1].x == 2)
    assert(t_array_struct[0].y == 0.0)
    assert(t_array_struct[1].y == 0.0)
    return true
}

