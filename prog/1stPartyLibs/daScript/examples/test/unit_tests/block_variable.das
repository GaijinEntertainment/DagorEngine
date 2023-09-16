struct Foo
    a : int

var g = 0

[export]
def test
    // void, no arg
    let localVar = [[Foo a=34]]
    var localVoidNoArgBlock = $ <|
        g = localVar.a
    g = 0
    localVoidNoArgBlock |> invoke()
    assert(g==localVar.a)
    // void, no arg
    var localVoidArgBlock = $ <| ( x : int ) : void
        g = localVar.a + x
    g = 0
    localVoidArgBlock |> invoke(1)
    assert(g==localVar.a+1)
    // result, no arg
    let localResultNoArgBlock = $ <|
        g = localVar.a
        return localVar.a
    g = 0
    let res1 = localResultNoArgBlock |> invoke()
    assert(g==localVar.a)
    assert(res1==localVar.a)
    // result, arg
    let localResultArgBlock = $ <| ( x : int ) : int
        g = localVar.a + x
        return localVar.a + x
    g = 0
    let res2 = localResultArgBlock |> invoke( 3 )
    assert(g==localVar.a+3)
    assert(res2==localVar.a+3)
    // result cmres, no arg
    let localCmresResultNoArgBlock = $ <|
        g = localVar.a
        return localVar
    g = 0
    let res3 = localCmresResultNoArgBlock |> invoke()
    assert(g==localVar.a)
    assert(res3.a==localVar.a)
    // result cmres, arg
    let localCmresResultArgBlock = $ <| ( x : int ) : Foo
        g = localVar.a + x
        return [[Foo a = localVar.a+x]]
    g = 0
    let res4 = localCmresResultArgBlock |> invoke( 3 )
    assert(g==localVar.a+3)
    assert(res4.a==localVar.a+3)
    return true


