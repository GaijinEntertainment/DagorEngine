// Complex sub-program compiled by gc tests.
// Exercises many TypeDecl patterns: generics, arrays, tables, lambdas, variants.
options gen2

require daslib/strings_boost

variant Value {
    i : int;
    f : float;
    s : string;
}

struct Pair {
    key : string
    value : Value
}

def generic_identity(x : auto) : auto {
    return x
}

def make_pairs() : array<Pair> {
    var result : array<Pair>
    result |> emplace(Pair(key = "one", value = Value(i = 1)))
    result |> emplace(Pair(key = "pi", value = Value(f = 3.14)))
    result |> emplace(Pair(key = "hello", value = Value(s = "world")))
    return <- result
}

[export]
def test() {
    var pairs <- make_pairs()
    var tab : table<string; int>
    for (p in pairs) {
        tab |> insert(p.key, generic_identity(1))
    }
    let lam = @(x : int) : int { return x * 2; }
    let r = invoke(lam, 21)
    feint("{r} {length(pairs)} {length(tab)}\n")
    delete pairs
    delete tab
}
