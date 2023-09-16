// options log = true, log_stack=true, log_nodes = true, optimize=true // , print_ref = true, print_var_access = true, log_optimization_passes = true

struct Foo
    x, y : int
    a : int[3]

struct Bar
    a, b : Foo

def make123
    //  Optimization - return to pass CMRES into local ref, for structure init
    //      Return
    //          InitLocalCMRes
    //          SetCMResValueT_TT 1
    //          SetCMResValueT_TT 2
    //          SetCMResValueT_TT 3
    return [[int 1;2;3]]                // stack ptr to hold cmres for returnRefAndEval

def pass123
    //  Optimization - GetCMRES to be passed as call return value
    //      Retrun
    //          CallAndCopyOrMove   (GetCMResOffs 0)
    return make123()

def make456
    var a : int[3]
    a[0] = 4
    a[1] = 5
    a[2] = 6
    return a

def inv123
    // Optimization - GetCMRES passed directly to invoke
    //      Return
    //          InvokeAndCopyOrMove (GetCMResOffs 0)
    return invoke($(){ return make123(); })

[export]
def test
    //  Optimization - call cmres to be passed directly to call
    //      CallAndCopyOrMove (GetLocal a0)
    let a0 = make456()
    assert(a0[0]==4 && a0[1]==5 && a0[2]==6)
    //  Optimization - call cmres to be passed directly to call
    //      CallAndCopyOrMove (GetLocal a1)
    var a1 : int[3]
    a1 = make456()
    assert(a1[0]==4 && a1[1]==5 && a1[2]==6)
    //  this tests invoke in inv123
    let a2 = inv123()
    assert(a2[0]==1 && a2[1]==2 && a2[2]==3)
    //  Optimization - local [[ ]] to be unrolled to direct set
    //      InitLocal
    //      SetLocalValue 1
    //      SetLocalValue 2
    let f0 = [[Foo x=1, y=2]]
    assert(f0.x==1 && f0.y==2)
    //  Optimization - copy [[ ]] to be unrolled to Ref set
    //      InitLocalRef
    //      SetLocalValueRefOff 1
    //      SetLocalValueRefOff 2
    var f1 : Foo
    f1 = [[Foo x=1, y=2]]                   // stack ptr to hold &f1
    assert(f1.x==1 && f1.y==2)
    //  Optimization - call CMRES to be passed directly to call
    //      InitLocal
    //      CallAndCopyOrMove   (GetLocal f3.a)
    let f3 = [[Foo a = make123()]]
    assert(f3.a[0]==1 && f3.a[1]==2 && f3.a[2]==3)
    //  Optimization - call CMRES ref to be passed directly to call
    //      InitLocalRef
    //      CallAndCopyOrMove (GetLocalRefOff offset_of_a)
    var f4 : Foo
    f4 = [[Foo a = pass123()]]              // stack ptr to hold &f4
    assert(f4.a[0]==1 && f4.a[1]==2 && f4.a[2]==3)
    // Optimization - invoke CMRES ref to be passed directly to invoke
    let f5 = invoke() <|
        return [[Foo x=1,y=2]]
    assert(f5.x==1 && f5.y==2)
    // Optimization - invoke CMRES ref
    //      InitLocal
    //      InvokeAndCopyOrMove (GetLocal f6)
    var f6 : Foo
    f6 = invoke() <|
        return [[ Foo x=1, y=2, a=inv123() ]]
    assert(f6.x==1 && f6.y==2 && f6.a[0]==1 && f6.a[1]==2 && f6.a[2]==3)
    // Optimization
    //      InitLocal
    //      InvokeAndCopyOrMove (GetLocal f7.a)
    let f7 = [[ Foo a=invoke($(){ return make123(); }) ]]
    assert(f7.a[0]==1 && f7.a[1]==2 && f7.a[2]==3)
    //  Optimization - nested [[ structures ]] to be flattened, only single init
    //      InitLocal
    //      SetLocalValue 1
    //      SetLocalValue 2
    let b0 = [[Bar a = [[Foo x=1, y=2]]]]
    assert(b0.a.x==1 && b0.a.y==2)
    //  Optimization - nested [[ array ]] to be flattened, only single init
    //      InitLocal
    //      SetLocalValue 1
    //      SetLocalValue 2
    //      SetLocalValue 3
    //      SetLocalValue 4
    let b1 = [[Bar a=[[Foo x=1, y=2]]; b=[[Foo x=3, y=4]]]]
    assert(b1[0].a.x==1 && b1[0].a.y==2 && b1[1].b.x==3 && b1[1].b.y==4)
    return true
