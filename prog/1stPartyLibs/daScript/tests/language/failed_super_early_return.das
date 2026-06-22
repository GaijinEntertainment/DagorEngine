// Path with early return before super — the if-true branch returns without calling super().
// Without exit-path tracking the linter would incorrectly fold the suffix's super into the
// early-return prefix and pass the lint. The CFG walker tracks early-exit paths separately.
options gen2
expect 30308

class Base {
    x : int = 0
    def Base {
        x = 13
    }
}

class Derived : Base {
    y : int = 0
    def Derived(skip : bool) {
        // 30308: skip=true takes the early return, super() never runs on that path
        if (skip) return
        super()
        y = 1
    }
}

[export]
def main {
    let d = new Derived(true)
    feint("d.x={d.x}\n")
}
