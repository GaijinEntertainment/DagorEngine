require daslib/enum_trait

enum Foo
    a
    b

enum Bar
    a
    b
    c

[export]
def main
    print("foo = {typeinfo(enum_length type<Foo>)}\nbar = {typeinfo(enum_length type<Bar>)}\n")
