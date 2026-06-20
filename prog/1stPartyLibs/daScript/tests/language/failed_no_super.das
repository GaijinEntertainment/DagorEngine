// Derived ctor must call super() — missing on every path.
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
        // 30308: class constructor does not call super(...) on every control-flow path
        y = 20
    }
}

[export]
def main {
    let d = new Derived()
    feint("d.x={d.x}\n")
}
