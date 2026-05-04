options gen2
expect 30304:12,  30101:1, 30507:1

require dastest/testing_boost public
require daslib/contracts

def func(a : int = 1; b : int; c : int = 3) {
    feint("{a},{b},{c}\n")
}

def gen(a; b; c = 1) {// that one is bellow - 30101: function argument default value type mismatch 'string const explicit' vs 'int const'
    feint("{a},{b},{c}\n")
}

[expect_any_tuple(a)]
def contract(a) {
    feint("{a}\n")
}

[test]
def test_invalid_types(t : T?) {
    func(1, [a=5, c=6])         //30304: no matching functions or generics func
    func(1, 1, 1, [a=5, b=6])
    func(1, 1, 1, 1, [a=5, b=6])//30304: no matching functions or generics func
    func(3, [a=5, b=6])
    func(3, [b=6])
    func(3, [a=5])              //30304: no matching functions or generics func
    func("hello", [a=5, b=6])   //30304: no matching functions or generics func
    1 |> func(1, 1, [b=6])
    "hello" |> func([a=5, b=6]) //30304: no matching functions or generics func

    gen(1, [a=5, c=6])          //30304: no matching functions or generics gen
    gen("ttt", [b="ttt"])
    gen(1, 1, 1, [a=5, b=6])
    gen(1, 1, 1, 1, [a=5, b=6]) //30304: no matching functions or generics gen
    gen(3, [a=5, b=6])
    3 |> gen([a=5, b=6])
    3 |> gen(5, [b=6])
    1 |> gen(1, 1, 1, [a=5, b=6])  //30304: no matching functions or generics gen

    gen(3, [b=6])
    gen(3, [a=5])               //30304: no matching functions or generics gen
    gen("hello", [a=5, b=6, c=1])
    gen("hello", [a=5, b=6, c="ttt"])  //30101: function argument default value type mismatch string const explicit vs int const

    contract(1, [a= (1, "ttt")])//30304: no matching functions or generics contract
    contract(fixed_array(1, "ttt"), [a=1]) //30507: can't initialize array element; int = string const
                                      //30304: no matching functions or generics contract ( int const )
    contract((1, "ttt"), [a=1]) //30304: no matching functions or generics contract ( int const )
    contract((1, "ttt"), [a= ("ttt", 1)])
}

