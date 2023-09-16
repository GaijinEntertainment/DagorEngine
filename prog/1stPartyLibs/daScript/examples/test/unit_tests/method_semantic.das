struct Point
    x, y = 0.0f
    magnitude  = @@point_magnitude
    dot = @@point_dot

def Point(X,Y)
    return [[Point() x=X,y=Y]]

def point_magnitude(that:Point)
    return that.x*that.x + that.y*that.y

def point_dot(that,p:Point)
    return that.x*p.x + that.y*p.y

[export]
def test
    let p  = Point(2.,3.)
    let t = p->magnitude()
    let d = p->dot(Point(1.,1.))
    assert(t==13. && d==5.)
    return true
