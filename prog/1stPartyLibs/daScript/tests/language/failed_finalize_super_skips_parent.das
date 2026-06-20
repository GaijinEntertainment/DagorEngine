// A hand-written delete cast<Ancestor>(self) that SKIPS the immediate parent does not
// satisfy the chain lint: it would bypass the parent's field-slice cleanup now that
// generated finalizers chain level by level. Only the immediate parent counts.
options gen2
expect 30308

class GrandBase {
    def operator delete {
        feint("grand base finalizer\n")
    }
}

class MidBase : GrandBase {
}

class Leaf : MidBase {
    def operator delete {
        // 30308: cast skips MidBase — not the chain call
        delete cast<GrandBase> self
    }
}

[export]
def main {
    var l = new Leaf()
    unsafe {
        delete l
    }
}
