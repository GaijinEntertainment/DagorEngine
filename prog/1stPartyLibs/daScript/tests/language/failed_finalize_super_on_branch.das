// Derived finalizer must `delete super.self` on every path — one if-branch is not enough.
options gen2
expect 30308

class Base {
    def operator delete {
        feint("base finalizer\n")
    }
}

class Derived : Base {
    flag : bool = true
    def operator delete {
        // 30308: false-branch path has zero delete super.self calls
        if (flag) {
            delete super.self
        }
    }
}

[export]
def main {
    var d = new Derived()
    unsafe {
        delete d
    }
}
