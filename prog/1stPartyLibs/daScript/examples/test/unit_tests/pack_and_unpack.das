require math

[export]
def test
    let xf = float4(1,2,3,4)
    let xb = pack_float_to_byte(xf)
    let xff = unpack_byte_to_float(xb)
    assert(xb==0x04030201)
    assert(xff==float4(1,2,3,4))
    return true
