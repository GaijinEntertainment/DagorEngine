options gen2
expect 50640    // internal_variable — malformed AST, variable 't`less' is missing its type

// The macro injects a `let` whose Variable has no type; the compiler must report
// 50640 (not crash, not silently repair-and-succeed)

require _typeless_let_macro

[export]
def main {
    for (i in typeless_range(10)) {
        print("{i}\n")
    }
}
