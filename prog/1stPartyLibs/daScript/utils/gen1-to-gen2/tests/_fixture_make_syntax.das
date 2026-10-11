options gen2 = false

[safe_when_uninitialized]
struct Foo
    a : int = 5
    b : float = 3.0

[safe_when_uninitialized]
struct Bar
    a : Foo

def make_them()
    var v1 = [[Foo a=1, b=2.0]]
    var v2 = [[/*a*/Foo/*b*/ a=1]]
    var v3 = [[Foo() a=1; a=2]]
    var v4 = [[Foo]]
    var v5 = [[auto 1; 2; 3]]
    var v6 = [[auto 4, 5]]
    var v7 = [[Bar a=[[Foo b=2.0]]]]
    var v8 = [[Foo /*c*/ a=1 /*d*/]]
    var w1 = {{ 1 => "a"; 2 => "b" }}
    var w2 = [{Foo a = 1; a = 2}]
    var w3 = [{auto 1; 2; 3}]
    var w4 = [{for x in 0..10; x * x; where x % 2 == 0}]
    var w5 = [[for x in 0..10; x * x]]
    var v9 = [[auto 1, 2; 3, 4]]
    var v10 = [[Foo#]]
    var v11 = [[auto[] 7]]
    var v12 = [[int 1; 2]]
    var v13 = [[tuple<string; float> "a", 1.0; "b", 2.0;]]
    return v1.a + v2.a + v3[0].a + v4.a + v5[0] + v6._0 + v7.a.a + v8.a + length(w1) + w2[0].a + w3[0] + w4[0] + v9[0]._0 + v10.a + v11[0] + v12[0] + length(v13)
