options gen2
expect 30101:2, 30108:4, 30109:2

require dastest/testing_boost public
require daslib/lpipe

typedef foo = int[32767][32767]


struct Foo {// 30101: structure Foo is too big
    a : int[32676][32767]
}

[export]
def argument_too_big(a : foo) {// 30108: argument variable a is too big
    feint("a = {a}\n")
    var b <- @(b : foo) { // 30108: argument variable b is too big
        feint("b = {b}\n")
    }
    invoke() $(c : foo) {// 30108: block argument variable c is too big
        feint("c = {c}\n")
    }
    lpipe(a)
}

[test]
def test_invalid_types(t : T?) {
    var a : int[32767][32767]       // 30108: local variable a is too big
    feint("a = {a}\n")
    var p = new Foo()               // 30109: can't new to a type that is too big
    var pp = new Foo()            // 30109: can't ascend type which is too big
}

    // this one happens during infer, and not lint. so we can't have it in the test
    // var sz = typeinfo(sizeof type<int[32767][32767]>)    // 30101: typeinfo(sizeof int[32767][32767]) is too big
    // feint("sz = {sz}\n")


