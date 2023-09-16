[export]
def test:bool
    let q <- [{ for x in range(10); x*x }]
    for y,qV in range(10),q
        verify(y*y==qV)
    let t <- [{ for x in range(10); x*x; where (x & 1)==0 }]
    var count = 0
    for z in range(10)
        if (z & 1)==0
            verify(t[count++]==z*z)
    return true

