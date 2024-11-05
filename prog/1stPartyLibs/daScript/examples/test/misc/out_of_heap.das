options heap_size_limit = 8192

struct Foo
    data : uint8[1024]

[export]
def main
    for i in range(1024)
        print("allocating Foo N{i}..\n")
        var pfoo = new Foo()
        pfoo.data[0] = uint8(i)
