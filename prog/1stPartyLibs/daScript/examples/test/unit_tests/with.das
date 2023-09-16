struct Point
    x, y : int = 0

[export]
def test
    var p = Point()
    with p
        x = 1
        y = 2
    assert(p.x==1 && p.y==2)
    return true

