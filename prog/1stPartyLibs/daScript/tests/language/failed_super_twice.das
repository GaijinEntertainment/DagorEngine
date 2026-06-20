// Derived ctor must call super() exactly once — duplicate is rejected.
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
        // 30308: super() called twice on the single path
        super()
        super()
        y = 20
    }
}

[export]
def main {
    let d = new Derived()
    feint("d.x={d.x}\n")
}
