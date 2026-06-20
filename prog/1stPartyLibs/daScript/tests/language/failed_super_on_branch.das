// Derived ctor must call super() on every path — calling only in one if-branch is not enough.
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
    def Derived(f : bool) {
        // 30308: false-branch path has zero super() calls
        if (f) {
            super()
        }
        y = 20
    }
}

[export]
def main {
    let d = new Derived(true)
    feint("d.x={d.x}\n")
}
