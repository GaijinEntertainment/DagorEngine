// Derived finalizer must `delete super.self` exactly once — duplicate is rejected.
options gen2
expect 30308

class Base {
    def operator delete {
        feint("base finalizer\n")
    }
}

class Derived : Base {
    def operator delete {
        // 30308: delete super.self called twice on the single path
        delete super.self
        delete super.self
    }
}

[export]
def main {
    var d = new Derived()
    unsafe {
        delete d
    }
}
