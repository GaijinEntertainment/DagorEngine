options gen2
require dastest/testing_boost public

// Regression test for permissive tuple constness when passing a tuple
// whose inner pointer is non-const to a parameter whose inner pointer
// is const. See src/ast/ast_typedecl.cpp — isSameType propagates
// isPassType into tuple argTypes.

struct Loc {
    a : int
}

struct Node {
    at : Loc
}

typedef NodePtr = Node?

var ats : array<tuple<string, Loc const?>>

var generic_hits : int = 0
var exact_hits : int = 0

def take(a : array<auto(TT)>; b : TT) {
    generic_hits ++
}

def takeng(a : array<tuple<string, Loc const?>>; b : tuple<string, Loc const?>) {
    exact_hits ++
}

def feed(var a : NodePtr&) {
    take(ats, ("test", unsafe(addr(a.at))))
    takeng(ats, ("test", unsafe(addr(a.at))))
}

[test]
def test_permissive_tuple_const(t : T?) {
    t |> run("tuple with const-pointer field accepts non-const-pointer argument") @(t : T?) {
        generic_hits = 0
        exact_hits = 0
        var e : Node
        e.at.a = 42
        var ep : NodePtr = unsafe(addr(e))
        feed(ep)
        t |> equal(generic_hits, 1)
        t |> equal(exact_hits, 1)
    }
}
