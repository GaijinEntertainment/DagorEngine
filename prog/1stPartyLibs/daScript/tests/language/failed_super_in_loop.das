// Derived ctor must call super() exactly once — call inside a loop is unbounded.
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
    def Derived {
        // 30308: super() in a loop body — count is unbounded, not exactly once
        for (i in range(2)) {
            super()
        }
        y = 20
    }
}

[export]
def main {
    let d = new Derived()
    feint("d.x={d.x}\n")
}
