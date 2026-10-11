options gen2 = false

[safe_when_uninitialized]
struct Foo
    a : int = 5
    b : float = 3.0

def make_them()
    var s = [[Foo a=1, b=2.0]]
    var arr = [[auto 1; 2; 3]]
    var tup = [[auto 4, 5]]
    var tab = {{ 1 => "a"; 2 => "b" }}
    return s.a + arr[0] + tup._0 + length(tab)
