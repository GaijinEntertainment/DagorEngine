options gen2
expect 50607    // internal_expression — malformed AST, iteratorsAt/iteratorsAka shorter than iterators

require _malformed_tree_macros

[export]
def main {
    for (i in short_iters_range(0, 10)) {
        print("{i}\n")
    }
}
