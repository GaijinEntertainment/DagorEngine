options gen2
expect 50503    // runtime_macro — macro_sticky_error on a VALID tree must still fail the compile

require _malformed_tree_macros

[export]
def main {
    for (i in sticky_range(0, 10)) {
        print("{i}\n")
    }
}
