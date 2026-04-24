options gen2
options unit_test = 1.23 // this is here to test custom option

require UnitTest
require dastest/testing_boost public

var {
    foo : TestObjectFoo
}

[sideeffects]
def GetGoo(arg : string) {
    return CheckEid(foo, arg)
}

[sideeffects]
def GetGoo2(arg : string) {
    return CheckEid2(foo, arg)
}

[test]
def test_check_eid(t : T?) {
    let a = CheckEid(foo, "goo")
    let b = GetGoo("goo")
    t |> equal(a, b)
    let a2 = CheckEid2(foo, "goo")
    let b2 = GetGoo2("goo")
    t |> equal(a2, b2)
}

