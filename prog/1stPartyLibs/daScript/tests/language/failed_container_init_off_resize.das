options gen2
options default_init_containers = false
expect 31013    // unsafe_function_call - with the policy off, resize of an init-carrying element type is unsafe again

struct Foo {
    x : int = 7
}

[export]
def main {
    var arr : array<Foo>
    arr |> resize(2)
}
