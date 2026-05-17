// Simple sub-program compiled by gc tests.
// Exercises basic TypeDecl creation: variables, functions, structs.
options gen2

struct Foo {
    x : int
    y : float
    name : string
}

def add(a, b : int) : int {
    return a + b
}

[export]
def test() {
    var f = Foo(x = 1, y = 2.0, name = "hello")
    let r = add(f.x, 10)
    feint("{r} {f.name}\n")
}
