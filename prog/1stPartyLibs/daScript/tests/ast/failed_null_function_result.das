options gen2
expect 50609    // internal_function — malformed AST, function is missing its result type

require _malformed_tree_macros

[null_result_fn]
struct Foo {
    a : int
}

[export]
def main {
    var f = Foo(a = 1)
    print("{f.a}\n")
}
