options gen2
require dastest/testing_boost
require daslib/strings_boost

require daslib/debug_eval

let print_flags_debug = (
            print_flags.escapeString
        |   print_flags.namesAndDimensions
        |   print_flags.singleLine)

def eval_test(context : table<string; Result>; expr : string) : string {
    var result = debug_eval(context, expr)
    if (!empty(result.error)) {
        return result.error
    }
    if (result.data != null) {
        return sprint_data(result.data, unsafe(addr(result.tinfo)), print_flags_debug)
    } else {
        return sprint_data(result.value, unsafe(addr(result.tinfo)), print_flags_debug)
    }
}

struct Foo {
    a : int
    b : float
}

def private check_arithmetics(t : T?) {
    let context : table<string; Result>
    t |> equal("true", context |> eval_test("true"))
    t |> equal("false", context |> eval_test("false"))
    t |> equal("false", context |> eval_test("1==2"))
    t |> equal("true", context |> eval_test("1==2-1"))
    t |> equal("true", context |> eval_test("!(1+1!=2)"))
    t |> equal("true", context |> eval_test("1<=2"))
    t |> equal("true", context |> eval_test("1!=2"))
    t |> equal("false", context |> eval_test("1>=2"))
    t |> equal("false", context |> eval_test("1>2"))
    t |> equal("true", context |> eval_test("1<2"))
    t |> equal("3", context |> eval_test("1|2"))
    t |> equal("2", context |> eval_test("1^3"))
    t |> equal("1", context |> eval_test("1&3"))
    t |> equal("0", context |> eval_test("-2+2"))
    t |> equal("-4", context |> eval_test("-2*2"))
    t |> equal("6", context |> eval_test("1+2+3"))
    t |> equal("6", context |> eval_test("1*2*3"))
    t |> equal("14", context |> eval_test("2+3*4"))
    t |> equal("14", context |> eval_test("4*3+2"))
    t |> equal("4", context |> eval_test("2+2"))
    t |> equal("20", context |> eval_test("(2+3)*4"))
    t |> equal("2", context |> eval_test("+2"))
    t |> equal("-2", context |> eval_test("-2"))
    t |> equal("-20", context |> eval_test("-(2+3)*4"))
    t |> equal("unexpected end of stream", context |> eval_test("-"))
    t |> equal("unexpected end of stream", context |> eval_test("2+"))
    t |> equal("runtime error, division by 0", context |> eval_test("2/0"))
    t |> equal("unexpected character ), expecting number or (", context |> eval_test("2+()"))
    t |> equal("invalid token invalid character #, expecting number or (", context |> eval_test("1+#"))
    t |> equal("unexpected token 41", context |> eval_test("123)))"))
    t |> equal("unknown variable foo", context |> eval_test("foo+1"))
    t |> equal("49", context |> eval_test("0x12+0x1f"))
}

def private check_operators(t : T?) {
    let context : table<string; Result>
    let cases = fixed_array(
        (src = "10-3-2", want = "5"),
        (src = "100-10-1", want = "89"),
        (src = "2-3+4", want = "3"),
        (src = "20-4-3-2", want = "11"),
        (src = "8/2", want = "4"),
        (src = "7/2", want = "3"),
        (src = "100/10/2", want = "5"),
        (src = "2+12/4", want = "5"),
        (src = "12/4*3", want = "9"),
        (src = "1+2*3-4", want = "3"),
        (src = "2/0", want = "runtime error, division by 0"),
        (src = "1&&2", want = "true"),
        (src = "2&&4", want = "true"),
        (src = "2&&0", want = "false"),
        (src = "0&&1", want = "false"),
        (src = "1||0", want = "true"),
        (src = "0||2", want = "true"),
        (src = "0||0", want = "false"),
        (src = "1^^1", want = "false"),
        (src = "1^^0", want = "true"),
        (src = "1&2", want = "0"),
        (src = "1|2&3", want = "3"),
        (src = "6&3|8", want = "10"),
        (src = "1&&2||0", want = "true"),
        (src = "0||1&&0", want = "false"),
        (src = "8/-2", want = "-4"),
        (src = "nosuchvar/-1", want = "unknown variable nosuchvar"),
        (src = "1<2&&3<4", want = "true"),
        (src = "1<2&&4<3", want = "false"),
        (src = "1<2||4<3", want = "true"),
        (src = "0&&2/0", want = "false"),
        (src = "1||2/0", want = "true"),
        (src = "0&&nosuchvar", want = "false"),
        (src = "1||nosuchvar", want = "true"),
        (src = "1&&nosuchvar", want = "unknown variable nosuchvar"),
        (src = "0||nosuchvar", want = "unknown variable nosuchvar"),
        (src = "1&&2/0", want = "runtime error, division by 0"),
        (src = "1||", want = "unexpected end of stream"),
        (src = "0&&", want = "unexpected end of stream"),
        (src = "1||1+", want = "unexpected end of stream"),
        (src = "0&&]", want = "unexpected character ], expecting number or ("),
        (src = "1||]", want = "unexpected character ], expecting number or ("),
        (src = "0&&#", want = "invalid token invalid character #, expecting number or ("),
        (src = "1||(2", want = "expecting )")
    )
    for (c in cases) {
        t |> equal(context |> eval_test(c.src), c.want, "eval '{c.src}'")
    }
}

def private check_variable_operations(t : T?) {
    var context : table<string; Result>
    var temp_int : int = 13
    var temp_str = "hello, world"
    var temp_dim_int = fixed_array(1, 2, 3, 4, 5, 6)
    var temp_arr_string <- [ "a", "b", "c", "d", "e"]
    var temp_struct =  Foo(a = 1, b = 2.23f)
    var temp_float3 = float3(1, 2, 3)
    var temp_float3_2 & = unsafe(temp_float3)
    var temp_tuple = (3, 0.14f)
    var temp_tab <- {1 => "one", 2 => "two", 3 => "three"}
    unsafe {
        context["temp_int"] = Result(data = addr(temp_int), tinfo = typeinfo rtti_typeinfo(temp_int))
        context["temp_str"] = Result(data = addr(temp_str), tinfo = typeinfo rtti_typeinfo(temp_str))
        context["temp_dim_int"] = Result(data = addr(temp_dim_int), tinfo = typeinfo rtti_typeinfo(temp_dim_int))
        context["temp_arr_string"] = Result(data = addr(temp_arr_string), tinfo = typeinfo rtti_typeinfo(temp_arr_string))
        context["temp_struct"] = Result(data = addr(temp_struct), tinfo = typeinfo rtti_typeinfo(temp_struct))
        context["temp_float3"] = Result(data = addr(temp_float3), tinfo = typeinfo rtti_typeinfo(temp_float3))
        context["temp_float3_2"] = Result(data = addr(temp_float3_2), tinfo = typeinfo rtti_typeinfo(temp_float3_2))
        context["temp_tuple"] = Result(data = addr(temp_tuple), tinfo = typeinfo rtti_typeinfo(temp_tuple))
        context["temp_tab"] = Result(data = addr(temp_tab), tinfo = typeinfo rtti_typeinfo(temp_tab))
    }
    t |> equal("16", context |> eval_test("temp_int+3"))
    t |> equal("\"hello, world\"", context |> eval_test("temp_str"))
    t |> equal("{'l'}", context |> eval_test("temp_str[2]"))
    t |> equal("[ 1, 2, 3, 4, 5, 6]", context |> eval_test("temp_dim_int"))
    t |> equal("{temp_dim_int[2]}", context |> eval_test("temp_dim_int[2]"))
    t |> equal("[ \"a\", \"b\", \"c\", \"d\", \"e\"]", context |> eval_test("temp_arr_string"))
    t |> equal("\"{temp_arr_string[1]}\"", context |> eval_test("temp_arr_string[1]"))
    t |> equal("{first_character(temp_arr_string[1])}", context |> eval_test("temp_arr_string[1][0]"))
    t |> equal("Foo( a = 1, b = 2.23)", context |> eval_test("temp_struct"))
    t |> equal("{temp_struct.a}", context |> eval_test("temp_struct.a"))
    t |> equal("2.23", context |> eval_test("temp_struct.b"))
    t |> equal("1{VEC_SEP}2{VEC_SEP}3", context |> eval_test("temp_float3"))
    t |> equal("2", context |> eval_test("temp_float3.y"))
    t |> equal("1{VEC_SEP}2{VEC_SEP}3", context |> eval_test("temp_float3_2"))
    t |> equal("2", context |> eval_test("temp_float3_2.y"))
    t |> equal("( 3, 0.14)", context |> eval_test("temp_tuple"))
    t |> equal("3", context |> eval_test("temp_tuple._0"))
    t |> equal("0.14", context |> eval_test("temp_tuple._1"))
    t |> equal("true", context |> eval_test("temp_tuple==temp_tuple"))
    t |> equal("12", context |> eval_test("length(temp_str)"))
    t |> equal("6", context |> eval_test("length(temp_dim_int)"))
    t |> equal("5", context |> eval_test("length(temp_arr_string)"))
    // this changes every time hash funciton is changed
    // t |> equal("table<int,string>( 1 => \"one\", 3 => \"three\", 2 => \"two\")",context|>eval_test("temp_tab"))
    t |> equal("\"one\"", context |> eval_test("temp_tab[1]"))
    t |> equal("\"two\"", context |> eval_test("temp_tab[2]"))
    t |> equal("key not found", context |> eval_test("temp_tab[-1]"))
    t |> equal("3", context |> eval_test("length(temp_tab)"))
}

[test]
def test_debug_eval(t : T?) {
    t |> run("arithmetics") @@(t : T?) {
        check_arithmetics(t)
    }
    t |> run("operator precedence and associativity") @@(t : T?) {
        check_operators(t)
    }
    t |> run("variable operations") @@(t : T?) {
        check_variable_operations(t)
    }
}
