options gen2

// Fixture for test_lineinfo_ranges.das — compiled by the test, never run.
// Shapes covered: let atInit spans, closure body blocks, class-method
// wrapper blocks. Keep everything braced; columns are derived at runtime.

def apply_it(blk : block<(q : int) : void>) {
    invoke(blk, 3)
}

class Kls {
    x : int
    def methA {
        x = 1
    }
    def methB() : int {
        return x
    }
}

def use_lets() {
    let cc = 1 + 2
    var dd : float = 3.0
    dd++
    let ee : int
    let ff <- [1, 2, 3]
    print("{cc} {dd} {ee} {ff}")
}

def use_fmt() : int {
    let ss = "pi={3.14159:.3f} ok"
    return ss == "" ? 0 : 1
}

def use_closures() {
    apply_it() $(q : int) {
        print("{q}")
    }
    let lam <- @(z : int) : int {
        return z + 1
    }
    print("{invoke(lam, 1)}")
}
