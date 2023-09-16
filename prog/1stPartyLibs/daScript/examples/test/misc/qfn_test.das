require qfn

[qfn]
struct FooBar
    a : int

[export]
def main
    var buf = CommandBuffer()
    var foo = [[FooBar a = 1]]
    send(buf, foo)


