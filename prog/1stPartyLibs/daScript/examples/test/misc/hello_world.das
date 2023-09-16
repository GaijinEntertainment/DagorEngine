require dump_fields

[|>dump_fields]
struct Foo
    a : int

struct Bar : Foo
    b : int

[export]
def main
    var a = Foo()
    print("a = {a}\n")
    var b = Bar()
    print("b = {b}\n")

options log
