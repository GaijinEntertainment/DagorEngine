enum DefEnum
    foo = 0
    bar

struct DefStruct
    x : int
    y : float

[export]
def test
    let t_int = [[int]]
    assert(t_int==0)
    let t_string = [[string]]
    assert(t_string=="")
    let t_tuple = [[tuple<int;float>]]
    assert(t_tuple._0==0 && t_tuple._1==0.0)
    let t_variant = [[variant<a:int;b:float>]]
    assert((t_variant is a) && (t_variant as a == 0))
    let t_struct = [[DefStruct]]
    assert(t_struct.x==0 && t_struct.y==0.0)
    let t_enum = [[DefEnum]]
    assert(t_enum == DefEnum foo)
    let t_array <- [[array<int>]]
    assert(length(t_array)==0)
    return true

