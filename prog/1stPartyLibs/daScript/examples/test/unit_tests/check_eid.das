options unit_test = 1.23 // this is here to test custom option

require UnitTest

var
    foo : TestObjectFoo

[sideeffects]
def GetGoo(arg:string)
    return CheckEid(foo,arg)

[sideeffects]
def GetGoo2(arg:string)
    return CheckEid2(foo,arg)

[export]
def test
    let a = CheckEid(foo,"goo")
    let b = GetGoo("goo")
    assert(a==b)
    let a2 = CheckEid2(foo,"goo")
    let b2 = GetGoo2("goo")
    assert(a2==b2)
    return true

