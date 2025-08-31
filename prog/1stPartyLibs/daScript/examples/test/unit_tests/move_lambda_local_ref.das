options gen2
struct Foo {
    bar : lambda<() : array<int> >
}

def Bar(var arr : array<int>) {
    return @  capture(<- arr) () {
        return <- clone_to_move(arr)
    }
}

let {
    foo = new Foo(
        bar <- Bar([ 1, 2, 3 ])
    )
}

[export]
def test() {
    let l <- invoke(foo.bar)
    assert(l[0] == 1 && l[1] == 2 && l[2] == 3)
    return true
}
