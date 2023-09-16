require math

def add(a, b:float3 const&)
    return a + b

def test_add(m : float3x4):float3
    return add(m[3],m[3])

[export]
def test
    let m : float3x4
    assert(test_add(m) == float3(0,0,0))
    return true
