require daslib/templates_boost
require daslib/ast_boost
require daslib/strings
require daslib/defer

def test_block_to_array
    var blk_array <- qmacro_block_to_array <|
        a <- 1
        b <- 2
        c <- 3
    for a in blk_array
        print("{describe(a)}\n")
    delete blk_array

def test_addr
    let foobar = "FOO_BAR"
    var inscope aaddr <- qmacro(@@ $i(foobar))
    print("\nADDR IS:\n{describe(aaddr)}\n")

def test_fun_arg
    var inscope foo <- [{VariablePtr
        new [[Variable() name:="v1", _type<-qmacro_type(type<int>)]];
        new [[Variable() name:="v2", _type<-qmacro_type(type<float>), init<-qmacro(1.2)]]
    }]
    var inscope fun <- qmacro_function("add") <| $ ( a: int; $a(foo); b : int )
        return a + b
    print("\nFUNCTION ARGUMENTS ARE:\n{describe(fun)}\n")
    delete fun

def test_var
    var inscope tt <- typeinfo(ast_typedecl type<float>)
    var inscope vvar <- qmacro_variable("foo",type<$t(tt)?>)
    print("\nVARIABLE IS:\n{describe(vvar)}\n")

def test_type
    var inscope argT <- typeinfo(ast_typedecl type<int>)
    var inscope resT <- qmacro_type(type<$t(argT)???>)
    print("\nTYPE IS:\n{describe(resT)}\n")

def test_fun
    var inscope argT <- typeinfo(ast_typedecl type<int???>)
    var inscope fun <- qmacro_function("add") <| $ ( a:$t(argT); b:float )
        return $i("a") + b
    print("\nFUNCTION REIFICATION IS:\n{describe(fun)}\n")
    delete fun

def test_for
    var bus = "BUS"
    var inscope blk <- qmacro_block <|
        for $i(bus) in foobar
            print("{$i(bus)}\n")
    print("\nFOR REIFICATION IS:\n{describe(blk)}\n")
    delete blk

def test_regular
    let bus = "bus_variable_name"
    let car = "car_variable_name"
    let cll = "call_me_maybe"
    let bar = "barbar"
    var a1 = 1
    var a2 = a1 + 1
    var a3 = a1 + 13
    var inscope expr <- quote(length(bus));
    var inscope qqblk : array<ExpressionPtr>
    for i in range(3)
        qqblk |> emplace_new <| qmacro(print("{$v(i)}\n"))
    var inscope arguments <- [{ExpressionPtr quote(a1); quote(a2)}]
    var inscope aargs <- [{ExpressionPtr quote(a1); quote(a2); quote(a3)}]
    var inscope subtype <- typeinfo(ast_typedecl type<int>)
    var inscope blk <- qmacro <|
        let t = $i(bus)
        let q = $v([[auto a1,car]])
        let lb = $e(expr)
        print("{q}{t}{lb}\n")
        $b(qqblk)
        somefunnycall(1,$a(arguments),2)
        var aaa <- [[auto $a(aargs)]]
        var a : $t(subtype)
        $c(cll)(1,2,$a(arguments),3)
        foo.$f(bar) = 13
        foo?.$f("{bar}`bar") = 14
        foo.$f(bar).$f(bar) = 15
        foo as $f(bar) = 13
        if foo is $f(bar)
            print("hello!\n")
        let qq = @@$c(cll)
        let $i(bus) = "busbus"
        invoke <| $ ( $i(bus) : bool )
            pass
    print(describe(blk))
    delete blk

def test_make_struct
    let foobar = "FOO_BAR"
    var inscope mks <- qmacro([[Foo $f(foobar)=13]])
    print("\nADDR IS:\n{describe(mks)}\n")

[export]
def main
    test_make_struct()
    test_block_to_array()
    test_addr()
    test_regular()
    test_fun()
    test_type()
    test_var()
    test_fun_arg()
    test_for()

