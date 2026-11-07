// typeinfo offsetof of a missing field is an error, not a crash
options gen2
expect 30825

struct Foo {
    a : int
}

[export]
def main {
    let o = typeinfo offsetof < b > (type<Foo>)
}
