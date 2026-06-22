// Derived finalizer must call `delete super.self` when an ancestor has a finalizer.
options gen2
expect 30308

class Base {
    def operator delete {
        feint("base finalizer\n")
    }
}

class Derived : Base {
    def operator delete {
        // 30308: class finalizer does not call delete super.self on every control-flow path
        feint("derived finalizer\n")
    }
}

[export]
def main {
    var d = new Derived()
    unsafe {
        delete d
    }
}
