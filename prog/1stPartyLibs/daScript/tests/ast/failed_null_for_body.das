options gen2
expect 50607    // internal_expression — malformed AST, for loop is missing its body

require _malformed_tree_macros

[export]
def main {
    for (i in nobody_range(0, 10)) {
        print("{i}\n")
    }
}
