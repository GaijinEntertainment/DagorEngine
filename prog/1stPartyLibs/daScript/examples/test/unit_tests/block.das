// options plot=true,plotFile="../../test.dot",plot_read_write=true
// options log=true, log_optimization_passes=true, print_ref = true, print_var_access = true

def twice(b:block)
    invoke(b)
    invoke(b)

def twice_good(b:block<int>):int
    var t = 0
    t += invoke(b)
    t += invoke(b)
    return t

[sideeffects]
def crapOnStack
    var a:int[20]
    for b in a
        b = 13

def func1(b:block)
    crapOnStack()
    invoke(b)
    crapOnStack()

def func2(b:block)
    crapOnStack()
    func3(b)
    crapOnStack()

def func3(b:block)
    crapOnStack()
    invoke(b)
    crapOnStack()

def func4(b:block)
    crapOnStack()
    invoke(b)
    crapOnStack()

def badd(ext:int;b:block<(arg:int):int>) : int
    return invoke(b,ext)

def radd(var ext:int&;b:block<(var arg:int&):int>):int
    return invoke(b,ext)

require dastest/testing_boost public

def block_var
    var blk = $ <| ( a, b : int )
        return a + b
    verify ( 3 == invoke(blk,1,2) )
    verify ( 7 == invoke(blk,3,4) )


[export]
def test:bool
    if true
        block_var()
    if true
        var a = 0
        var b = 0
        twice({ a++; })
        assert(a==2)
        b = twice_good() <| { return a++; }
        assert(a==4 && b==(2+3))
        b = twice_good() <|
            return a++
        assert(b==(4+5))
        b = twice_good({ return ++a; })
        assert(a==8)
        assert(b==(7+8))
    if true
        var x = 0
        var y = 0
        func1() <|
            x += 1
            y += 2
            func2() <| // here
                x++
                y += 3
            func4() <|
                x++
                y += 4
        assert(x==3 && y==9)
    if true
        var v1 = 1
        var res = badd(1000,$(a1:int):int{ return v1++ + a1; })
        assert(res==1001)
        res = badd(1000) <| $(a1:int):int{ return v1 + a1; }
        assert(res==1002)
        v1 = 3
        res = badd(1000) <| $(a1:int):int
            return v1 + a1
        assert(res==1003)
        v1 = 4
        res = badd(1000) <| $(a2:int):int
            return badd(2000,$(a3:int):int { return a3 + a2 + v1; })
        assert(res==3004)
        v1 = 3
        res = radd(v1) <| $(var a:int&):int
            return a++
        assert(res==3 && v1==4)
    return true

