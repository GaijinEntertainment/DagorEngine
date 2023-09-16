// options log=true, log_stack=true, optimize=true

struct Bar
    x:int = 1
    y:int = 2
    z:int = 3
    w:int = 4

def test_bar
    let tf = Bar()
    assert(tf.x==1 && tf.y==2 && tf.z==3 && tf.w==4)
    let t0 = [[Bar x=13]]
    assert(t0.x==13 && t0.y==0 && t0.z==0 && t0.w==0)
    let t1 = [[Bar() x=13]]
    assert(t1.x==13 && t1.y==2 && t1.z==3 && t1.w==4)
    return true

struct Foo
    i : int = 5
    f : float = 6.0
    s : string = "hello"

let
    testFoo = Foo()

let
    jestFoo = [[Foo i=3, f=4., s="blah"]]

[export]
def test
    test_bar()
    assert(testFoo.i==5 && testFoo.f==6. && testFoo.s=="hello")
    assert(jestFoo.i==3 && jestFoo.f==4. && jestFoo.s=="blah")
    let t = Foo()
    assert(t.i==5)
    assert(t.f==6.0)
    assert(t.s=="hello")
    let q = [[Foo i=6, s="world"]]
    assert(q.i==6)
    assert(q.f==0.0)
    assert(q.s=="world")
    let qq3 = [[Foo i=1; i=2; i=3]]
    assert(qq3[0].i==1 && qq3[1].i==2 && qq3[2].i==3)
    let ar = [[int 1 ; 2 ; 3 ; 4]]
    assert(ar[0]==1 && ar[1]==2 && ar[2]==3 && ar[3]==4)
    let arf = [[Foo Foo(); Foo()]]
    assert(arf[0].i==5 && arf[1].i==5)
    return true
