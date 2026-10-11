options gen2

module _standalone_dep_call public

require daslib/fio

// A required module whose FUNCTIONS the entry module calls at run time - a plain one that reaches
// a C++ extern only from here, and a class method dispatched through its Func slot.
def dep_probe(x : int) : int {
    return x + length(getcwd())
}

class DepProbe {
    def method(x : int) : int {
        return x * 2
    }
}

[hybrid]
def dep_hybrid(x : int) : int {
    return x + 1
}

[jit]
def dep_jit(x : int) : int {
    return x + 2
}

struct DepPair {
    a : int
    b : int
}

def dep_pair(x : int) : DepPair {
    return DepPair(a = x, b = x + 1)
}

def dep_text(s : string) : int {
    return length(s)
}

def dep_fill(var arr : array<int>; n : int) {
    arr |> push(n)
}
