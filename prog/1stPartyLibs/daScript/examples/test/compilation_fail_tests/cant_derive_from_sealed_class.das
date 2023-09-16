expect 30115

class sealed A
    name : string = "foobar"

class B : A                 // 30115: can't derive from sealed class or structure A
    def get_name : string
        return name

[export]
def main
    let b = new B()
    print("b = {b.name}\n")
