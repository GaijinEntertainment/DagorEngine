expect 40209:2

//options log_optimization_passes=true
// options log=true

def two_plus_two : int
    return 2 + 2

def var_two_plus_two : int
    let x, y : int = 2;
    return x + y

def var_op3_two_plus_two(cond:bool) : int
    let x, y : int = 2
    return (cond ? x : y) + 2

def var_op3_arg_two_plus_two(cond:bool) : int
    let x, y : int = 2
    let cc : bool = cond
    return (cc ? x : y) + 2

def test_dead_code(X:int):int
    var x : int = X
    while x < 5
        x ++
        break
        static_assert(false,"fail to eliminate dead code after break")
    if true
        return x--
        static_assert(false,"fail to eliminate dead code after return")
    else
        static_assert(false,"fail to eliminate dead code in the if")

def test_empty_if(cond:bool)
    if cond
        assert(false)
    else
        assert(false)

[export]
def test:bool
    if (3 + ( 2>3 ? 0 : 1 ))!=4
        3 + ( 2>3 ? 0 : 1 )
        static_assert(false,"fail to eliminate dead code")
    else
        3 + ( 2>3 ? 0 : 1 )
    // static_assert(false,"this static assert always fails")
    var x,y:int = 3
    x
    x + y                   // 40209: top level no side effect operation +
    hash(x) + hash(y)       // 40209: top level no side effect operation +
    return true

