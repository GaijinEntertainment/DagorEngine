// options logCpp=true

struct iVec2
    x,y:int

def iVec2(X,Y:int)
    return [[iVec2 x=X,y=Y]]

def operator - ( a: iVec2 ) : iVec2
    return [[iVec2 x=-a.x,y=-a.y]]

def operator + ( a,b:iVec2 ) : iVec2
    return [[iVec2 x=a.x+b.x,y=a.y+b.y]]

def operator == ( a,b:iVec2 ) : bool
    return (a.x==b.x) && (a.y==b.y)

[export]
def test:bool
    let a = iVec2(4,5)
    let b = iVec2(6,7)
    assert(-a==iVec2(-4,-5))
    assert(a+b==iVec2(10,12))
    return true
