// options log=true,log_infer_passes=false,optimize=true

options no_unused_function_arguments = false

struct Foo
    bar : int

def testInfT(var a:auto(numT))
    return typeinfo(typename type<numT>)

def testVarRefT(var a)
    return typeinfo(typename a)

def testVarValT(var a)
    return typeinfo(typename a+0)

def arrType(var a:array<auto(arrT)>)
    return typeinfo(typename type<arrT>)

def tabTypes(var a:table<auto(key);auto(value)>)
    return typeinfo(typename type<key>) + ":" + typeinfo(typename type<value>)

[export]
def test:bool
    var x : auto(numT)  = 3
    assert(typeinfo(typename type<numT>)=="int")
    assert(testInfT(3)=="int")
    assert(testVarRefT(3)=="int")
    assert(testVarValT(3)=="int")
    var y : table<string;int>
    assert(tabTypes(y)=="string:int")
    var z : array<float>
    var ZZ : array<Foo>
    assert(arrType(z)=="float")
    assert(arrType(ZZ)=="Foo")
    return true

