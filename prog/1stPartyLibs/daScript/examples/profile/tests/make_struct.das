struct Foo
    x,y,z,w:int

[sideeffects]
def make_1
    var t:Foo
    t.x = 1
    t.y = 2
    t.z = 3
    t.w = 4
    return t

[sideeffects]
def make_2
    return [[Foo x=1, y=2, z=3, w=4]]

def test_1
    var q = 0
    for x in range(1000000)
        q += make_1().x
    return q

def test_2
    var q = 0
    for x in range(1000000)
        q += make_2().x
    return q

[export]
def test
    var q1 = 0
    profile(20,"make t.x=1;...") <|
        q1 = test_1()
    assert(q1==1000000)
    var q2 = 0
    profile(20,"make [[Foo x=1;...]]") <|
        q2 = test_2()
    assert(q2==1000000)
    return true



