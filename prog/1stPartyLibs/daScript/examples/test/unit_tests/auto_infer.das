// options optimize=false, log=true
// options log_infer_passes=true

options no_unused_block_arguments = true

options no_unused_function_arguments = false

struct Foo
    bar : int

let
    intnum = 5
    floatnum = 5.5
    stringval = "hello, world"

let
    some_a = "test"
    some_b <- [{ auto[] some_a }]

def foo(b:block<(a:int):int>)
    return invoke(b, 3)

def foobar(b:block<(a:int;b:float):string>)
    return invoke(b,1,2.0) + "bar"

def madd(a,b,c)
    return a*b+c

def take_arr(a:int[])   // infer array size
    var summ = 0
    for x in a
        summ += x
    return summ

def random_pick(source : string[]; var seed : int4&) : string
    return "pick_1"

def random_pick(source : tuple<string; float>[]; var seed : int4&) : string
    return "pick_2"

def test_pick()
    let s1 : string[2]
    let s2 : tuple<string; float>[3]
    var seed : int4
    let q1 = random_pick(s1,seed)
    assert(q1=="pick_1")
    let q2 = random_pick(s2,seed)
    assert(q2=="pick_2")

[sideeffects]
def qq ( a : auto(T1); b : block<(a1:T1;a2:T2):bool>; c : auto(T2) )
    pass

[export]
def test
    // global infer
    assert(length(some_b)==1)
    // regular array
    let a:int[10]
    let b = a
    let c : auto const = b
    // array size
    verify(take_arr([[int[4] 1;2;3;4]])==10)
    // even better array size
    verify(length(a)==10)
    // pointer
    var pFoo : Foo? = new Foo
    var pFooI : auto? = pFoo
    pFooI.bar = 1
    pFoo = pFooI
    assert(pFoo.bar==1)
    // array
    var TT_A : array<int>
    let TT_iA : array<auto> <- TT_A
    // table
    var TT_T : table<string;int>
    let TT_iT : table<auto;auto> <- TT_T
    let TT_ikT : table<auto;int> <- TT_T
    let TT_ivT : table<string;auto> <- TT_T
    // block return type
    let f1 = foo() <| $(arg:int)
        return arg+1
    assert(f1==4)
    // block argument
    let f2 = foo() <| $(arg=5)
        return arg+2
    assert(f2==5)
    // forward infer entire block type and argument types
    let f3 = foo() <| $(arg)
        return arg + 3
    assert(f3==6)
    // two arguments
    let blah = foobar() <| $(ia,fb)
        assert(ia==1)
        assert(fb==2.0)
        return "foo"
    assert(blah=="foobar")
    // function return type
    assert(fn1()==6)
    // full-blown generic
    let x = madd(1,2,3)
    assert(x==5)
    let y = madd(1.0,2.0,3.0)
    assert(y==5.0)
    // and pick
    test_pick()
    // and array
    let ZZT = [[auto "two"=>2]]
    assert(!typeinfo(is_dim ZZT))
    let ZZA = [[auto[] "two"=>2]]
    assert(typeinfo(is_dim ZZA) && typeinfo(dim ZZA)==1)
    // infering block goo
    qq(1,$ [unused_argument(XX,YY)] (XX,YY) => true,2.0)
    return true

def fn3
    return 3

def fn2
    return fn3() + 1

def fn1
    return fn2() + 2

