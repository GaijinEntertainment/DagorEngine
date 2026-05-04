options gen2
require dastest/testing_boost
require daslib/faker
require daslib/fuzzer
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

[test]
def test_debug_eval(t : T?) {
    t |> run("arithmetics") @@(t : T?) {
        var context : table<string; Result>
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
    t |> run("variable operations") @@(t : T?) {
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
        t |> equal("{character_at(temp_arr_string[1],0)}", context |> eval_test("temp_arr_string[1][0]"))
        t |> equal("Foo( a = 1, b = 2.23)", context |> eval_test("temp_struct"))
        t |> equal("{temp_struct.a}", context |> eval_test("temp_struct.a"))
        t |> equal("2.23", context |> eval_test("temp_struct.b"))
        t |> equal("1, 2, 3", context |> eval_test("temp_float3"))
        t |> equal("2", context |> eval_test("temp_float3.y"))
        t |> equal("1, 2, 3", context |> eval_test("temp_float3_2"))
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
}



