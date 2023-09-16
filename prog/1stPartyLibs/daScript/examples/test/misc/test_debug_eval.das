require daslib/debug_eval
require UnitTest

def print ( result:Result )
    if !empty(result.error)
        print("error: {result.error}\n")
    unsafe
        var text : string
        if result.data != null
            text = sprint_data(result.data, addr(result.tinfo), print_flags_debug)
        else
            text = sprint_data(result.value, addr(result.tinfo), print_flags_debug)
        print("{text}\n")

let print_flags_debug = (
            print_flags escapeString
        |   print_flags namesAndDimensions
        |   print_flags singleLine
        // |   print_flags humanReadable
        // |   print_flags typeQualifiers
        // |   print_flags refAddresses
    )

def eval_test ( context:table<string;Result>; expr:string ) : string
    var result = debug_eval(context, expr)
    if !empty(result.error)
        return result.error
    if result.data != null
        return sprint_data(result.data, unsafe(addr(result.tinfo)), print_flags_debug)
    else
        return sprint_data(result.value, unsafe(addr(result.tinfo)), print_flags_debug)

struct Foo
    a : int
    b : float

[export]
def main
    var context : table<string; Result>

    var temp_int : int = 13
    var temp_str = "hello, world"
    var temp_dim_int = [[auto 1;2;3;4;5;6]]
    var temp_arr_string <- [{auto "a";"b";"c";"d";"e"}]
    var temp_struct =  [[Foo a=1, b=2.23f]]
    var temp_float4 = float4(1,2,3,4)
    var temp_tuple = [[auto 3, 0.14f]]
    var temp_tab <- {{1 => "one"; 2 => "two"; 3 => "three"}}
    var temp_foo = [[TestObjectFoo() fooData=13]]
    unsafe
        context["temp_int"] = [[Result data = addr(temp_int), tinfo = typeinfo(rtti_typeinfo temp_int)]]
        context["temp_str"] = [[Result data = addr(temp_str), tinfo = typeinfo(rtti_typeinfo temp_str)]]
        context["temp_dim_int"] = [[Result data = addr(temp_dim_int), tinfo = typeinfo(rtti_typeinfo temp_dim_int)]]
        context["temp_arr_string"] = [[Result data = addr(temp_arr_string), tinfo = typeinfo(rtti_typeinfo temp_arr_string)]]
        context["temp_struct"] = [[Result data = addr(temp_struct), tinfo = typeinfo(rtti_typeinfo temp_struct)]]
        context["temp_float4"] = [[Result data = addr(temp_float4), tinfo = typeinfo(rtti_typeinfo temp_float4)]]
        context["temp_tuple"] = [[Result data = addr(temp_tuple), tinfo = typeinfo(rtti_typeinfo temp_tuple)]]
        context["temp_tab"] = [[Result data = addr(temp_tab), tinfo = typeinfo(rtti_typeinfo temp_tab)]]
        context["temp_foo"] = [[Result data = addr(temp_foo), tinfo = typeinfo(rtti_typeinfo temp_foo)]]

    context |> debug_eval("temp_foo.fooData") |> print
    context |> debug_eval("temp_tab") |> print
    context |> debug_eval("temp_tab[1]") |> print
    context |> debug_eval("temp_tab[2]") |> print
    context |> debug_eval("temp_tab[-1]") |> print

    context|>debug_eval("temp_tuple") |> print
    context|>debug_eval("temp_tuple._0") |> print
    context|>debug_eval("temp_tuple._1") |> print

    context|>debug_eval("temp_float4") |> print
    context|>debug_eval("temp_float4.x") |> print
    context|>debug_eval("temp_float4.y") |> print
    context|>debug_eval("temp_float4.z") |> print
    context|>debug_eval("temp_float4.w") |> print

    context|>debug_eval("temp_int+3") |> print
    context|>debug_eval("temp_str") |> print
    context|>debug_eval("temp_str[2]") |> print
    context|>debug_eval("temp_dim_int") |> print
    context|>debug_eval("temp_dim_int[2]") |> print
    context|>debug_eval("temp_arr_string") |> print
    context|>debug_eval("temp_arr_string[1]") |> print
    context|>debug_eval("temp_arr_string[1][0]") |> print
    context|>debug_eval("temp_struct") |> print
    context|>debug_eval("temp_struct.a") |> print
    context|>debug_eval("temp_struct.b") |> print

    context|>debug_eval("length(temp_str)") |> print

    verify("true"==context|>eval_test("true"))
    verify("false"==context|>eval_test("false"))
    verify("false"==context|>eval_test("1==2"))
    verify("true"==context|>eval_test("1==2-1"))
    verify("true"==context|>eval_test("!(1+1!=2)"))
    verify("true"==context|>eval_test("1<=2"))
    verify("true"==context|>eval_test("1!=2"))
    verify("false"==context|>eval_test("1>=2"))
    verify("false"==context|>eval_test("1>2"))
    verify("true"==context|>eval_test("1<2"))

    verify(context|>eval_test("-2+2")=="0")
    verify(context|>eval_test("-2*2")=="-4")
    verify(context|>eval_test("1+2+3")=="6")
    verify(context|>eval_test("1*2*3")=="6")
    verify(context|>eval_test("2+3*4")=="14")
    verify(context|>eval_test("4*3+2")=="14")
    verify(context|>eval_test("2+2")=="4")
    verify(context|>eval_test("(2+3)*4")=="20")
    verify(context|>eval_test("+2")=="2")
    verify(context|>eval_test("-2")=="-2")
    verify(context|>eval_test("-(2+3)*4")=="-20")
    verify(context|>eval_test("-")=="unexpected end of stream")
    verify(context|>eval_test("2+")=="unexpected end of stream")
    verify(context|>eval_test("2/0")=="runtime error, division by 0")
    verify(context|>eval_test("2+()")=="unexpected character ), expecting number or (")
    verify(context|>eval_test("1+#")=="invalid token invalid character #, expecting number or (")
    verify(context|>eval_test("123)))")=="unexpected token 41")
    verify(context|>eval_test("foo+1")=="unknown variable foo")
    verify(context|>eval_test("0x12+0x1f")=="49")
    verify(context|>eval_test("temp_foo.fooData+1")=="14")



