// Super absent from try/recover entirely — both branches lack super, no normal path covers it.
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
        // 30308: try block has no super → normal-flow path count is 0;
        // recover is ignored by the lint (panic-and-exit semantics)
        try {
            y = 1
        } recover {
            y = -1
        }
    }
}

[export]
def main {
    let d = new Derived()
    feint("d={d.x}\n")
}
