expect 30101:2

// this one creates circular dependency
struct Foo
    a : Foo

// this one ok
struct Node
    left, right : Node?

// this one has fancy circular dependency
struct Bar
    a : array<Bar>


struct L
    i : int

struct A
    l : array<L>

struct C
    b : table<string; A>
    c : table<string; A>

[export]
def test:bool
    let a : Foo
    let b : Node
    let c : Bar
    // test 2
    let l : C
    return true

