options gen2
expect 50640    // internal_variable — malformed AST, let expression has a null variable entry

require _malformed_tree_macros

[export]
def main {
    for (i in nullvar_range(0, 10)) {
        print("{i}\n")
    }
}
