require rtti

var
    delFoo = 0
    delFoo3D = 0

class Foo
    x, y : int = 0
    [export] def Foo                    // custom initializer
        Foo`set(self,1,1)
    def Foo(T:int)                      // custom initializer
        self->set(T,T)
    def Foo(X,Y:int)                    // custom initializer
        Foo`set(self,X,Y)
    def set(X,Y:int)                    // inline method variable
        x = X
        y = Y
    def ups()
        x = -1
        y = -1
    def operator delete                        // custom finalizer
        delFoo ++

class Foo3D : Foo
    z : int = 13
    def Foo3D                           // overriding default initializer
        assert(x==0 && y==0)             // Foo initializer is NOT called by default
        assert(z==13)
        Foo`Foo(self)                   // call parents initializer explicitly
        assert(x==1 && y==1)
        z = 3
    def override set(X,Y:int)           // overriding method variable
        Foo`set(self,X,Y)               // calling generated method function directly
        z = 0
    def override ups()                  // for testing 'polymorphism'
        x = -2
        y = -2
        z = -2
    def operator delete                        // custom finalizer
        delete cast<Foo> self           // calling Foo finalizer
        delFoo3D ++

def test_foo
    unsafe
        var f = Foo()                   // this is unsafe
        assert(f.x==1 && f.y==1)
        f->set(1,2)
        assert(f.x==1 && f.y==2)
        f = Foo(11,12)
        assert(f.x==11 && f.y==12)
        f = Foo(13)
        assert(f.x==13 && f.y==13)
        var f3d = Foo3D()               // this is unsafe
        assert(f3d.x==1 && f3d.y==1 && f3d.z==3)
        f3d->set(1,2)
        assert(f3d.x==1 && f3d.y==2 && f3d.z==0)
        f3d.z = 13
        f3d->set(3,4)
        assert(f3d.x==3 && f3d.y==4 && f3d.z==0)
        assert(delFoo==0 && delFoo3D==0)
        delete f
        assert(delFoo==1 && delFoo3D==0)
        delete f3d
        assert(delFoo==2 && delFoo3D==1)
        delFoo = 0
        delFoo3D = 0

def test_foo_ptr
    var f = new Foo()
    f->ups()
    assert(f.x==-1 && f.y==-1)
    assert(delFoo==0 && delFoo3D==0)
    unsafe
        delete f
    assert(delFoo==1 && delFoo3D==0)
    f = cast<Foo?> new Foo3D()
    f->ups()
    assert(f.x==-2 && f.y==-2)
    unsafe
        delete f
    assert(delFoo==2 && delFoo3D==1)
    delFoo = 0
    delFoo3D = 0

class Goo
    a : int

def test_rtti
    unsafe
        var g = Goo()       // unsafe
        var f = Foo()       // unsafe
        var f2 = Foo()      // unsafe
        var f3d = Foo3D()   // unsafe
        var f3d2 = Foo3D()  // unsafe
        assert(f.__rtti!=g.__rtti)
        assert(f3d.__rtti!=g.__rtti)
        assert(f.__rtti==f2.__rtti)
        assert(f3d.__rtti==f3d2.__rtti)
        assert(f.__rtti!=f3d.__rtti)
        assert(class_info(f)!=class_info(g))
        assert(class_info(f3d)!=class_info(g))
        assert(class_info(f)==class_info(f2))
        assert(class_info(f3d)==class_info(f3d2))
        assert(class_info(f)!=class_info(f3d))
        verify(is_compatible_cast(class_info(f),class_info(f)))   // can cast from f to f
        verify(is_compatible_cast(class_info(f),class_info(f3d)))   // can cast from f3d to f
        verify(!is_compatible_cast(class_info(f3d),class_info(f)))  // can't cast from f to f3d
        verify(!is_compatible_cast(class_info(g),class_info(f)))  // can't cast from f to g
        verify(!is_compatible_cast(class_info(f),class_info(g)))  // can't cast from g to f
        verify(!is_compatible_cast(class_info(g),class_info(f3d)))  // can't cast from f3d to g
        verify(!is_compatible_cast(class_info(f3d),class_info(g)))  // can't cast from g to f3d
        assert(class_info(f).name=="Foo")
        assert(class_info(f3d).name=="Foo3D")
        assert(class_info(g).name=="Goo")

[export]
def test
    test_foo()
    test_foo_ptr()
    test_rtti()
    return true
