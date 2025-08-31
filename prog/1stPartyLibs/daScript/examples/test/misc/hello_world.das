options gen2

variant Foo {
    i : int
    f : float
}

typedef Foo2 = variant<i : int; f : float>

[export]
def main {
    let x = Foo(i = 3)
    let y : Foo = Foo2(f = 4.0)
    let a = array variant<i : int; f : float>(i = 3, f = 4.0)
}

