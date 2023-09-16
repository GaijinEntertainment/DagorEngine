// options log=true, log_stack=true, log_nodes=true, optimize=true
require math

[export]
def test
    var a,b : float3x4
    identity(a)
    identity(b)
    a = a*b
    a := b
    return true

