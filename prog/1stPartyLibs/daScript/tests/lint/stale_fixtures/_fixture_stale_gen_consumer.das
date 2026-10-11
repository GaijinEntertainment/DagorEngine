options gen2
// Instantiates _fixture_stale_gen's generic so its directives consume.
require _fixture_stale_gen

[export]
def main {
    var a : array<int>
    let b <- [1, 2, 3]
    gen_push(a, b)
    print("{length(a)}\n")
}
