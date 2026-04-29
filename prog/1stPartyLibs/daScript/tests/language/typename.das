options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

struct TnFoo {
    bar : int
}

def tn_testInfT(var a : auto(numT)) {
    return typeinfo typename(type<numT>)
}

def tn_testVarRefT(var a) {
    return typeinfo typename(a)
}

def tn_testVarValT(var a) {
    return typeinfo typename(a + 0)
}

def tn_arrType(var a : array<auto(arrT)>) {
    return typeinfo typename(type<arrT>)
}

def tn_tabTypes(var a : table<auto(key); auto(value)>) {
    return typeinfo typename(type<key>) + ":" + typeinfo typename(type<value>)
}

[test]
def test_typename(t : T?) {
    t |> run("typeinfo typename for various types") @(t : T?) {
        var x : auto(numT) = 3
        t |> equal(typeinfo typename(type<numT>), "int")
        t |> equal(tn_testInfT(3), "int")
        t |> equal(tn_testVarRefT(3), "int")
        t |> equal(tn_testVarValT(3), "int const")
        var y : table<string; int>
        t |> equal(tn_tabTypes(y), "string:int")
        var z : array<float>
        var zz : array<TnFoo>
        t |> equal(tn_arrType(z), "float")
        t |> equal(tn_arrType(zz), "TnFoo")
    }
}
