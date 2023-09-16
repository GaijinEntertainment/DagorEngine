require daslib/templates

let FAIL_TO_COMPILE = false

def take_int_or_float ( a : int | float )
    print("int_or_float : {typeinfo(typename a)} = {a}\n")

def take_array_of_int_or_float ( a : array<int|float> )
    print("array_of_int_or_float : {typeinfo(typename a)} = {a}\n")


def take_partial_0 ( var a : array<auto(TT)> | table<int;auto(TT)> )
    var t : TT-# := a[0]
    print("partial = {t}\n")
    return t

struct Foo
    a : int

def take_temp_or_not ( a : Foo# | Foo )
    print("temp_or_not : {typeinfo(typename a)} = {a}\n")

[export]
def main
    // basic
    take_int_or_float(1)
    take_int_or_float(2.)
    static_if FAIL_TO_COMPILE
        take_int_or_float("3")  // does not compile
    // nested
    take_array_of_int_or_float([{auto 1;2;3}])
    take_array_of_int_or_float([{auto 1.;2.;3.}])
    static_if FAIL_TO_COMPILE
        take_array_of_int_or_float([{auto "1";"2";"3"}]) // does not compile
    // partial
    take_partial_0([{auto 1;2;3}])
    take_partial_0({{0=>1;1=>2;2=>3;3=>4}})
    static_if FAIL_TO_COMPILE
        take_partial_0({{"0"=>1;"1"=>2;"2"=>3;"3"=>4}})
    // temp or not
    take_temp_or_not([[Foo a=1]])
    var t : Foo#
    take_temp_or_not(t)
