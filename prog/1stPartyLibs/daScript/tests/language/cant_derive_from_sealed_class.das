// sealed class derivation
// verifies compiler rejects inheriting from a class marked 'sealed'
options gen2
expect 30115

class sealed A {
    name : string = "foobar"
}

class B : A {// 30115: can't derive from sealed class or structure A
    def get_name : string {
        return name
    }
}

[export]
def main {
    let b = new B()
    feint("b = {b.name}\n")
}
