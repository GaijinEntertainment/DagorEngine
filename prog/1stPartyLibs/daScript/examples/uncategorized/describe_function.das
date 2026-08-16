options gen2

// describe_function demo — pretty-prints a compiled function back to source.
//
// `describe_function` (daslib/ast) renders an AST Function node as daslang
// source. It always emits gen2 syntax: braces on every block, parens on
// control flow. We compile a small program at runtime, grab its module, and
// print each user function. Run standalone to see the round-tripped source:
//
//   daslang.exe describe_function.das
//
// The same pattern works for `describe(expr)` on any Expression and
// `describe(prog)` on a whole Program.

require daslib/ast
require daslib/rtti

// The embedded source is gen2; its literal { } braces are escaped as \{ \} so
// they are not read as "{interpolation}" inside this string literal.
let SAMPLE_SRC = "
options gen2
def demo(x : int; var arr : array<int>) : int \{
    var total = 0
    for (i in range(x)) \{
        if (i % 2 == 0) \{
            total += i
        \} else \{
            total -= i
        \}
    \}
    while (total > 100) \{
        total -= 10
    \}
    return total
\}
"

[export]
def main() {
    compile("demo_module", SAMPLE_SRC, CodeOfPolicies()) $(ok; prog; errors) {
        if (!ok) {
            print("compile failed:\n{errors}\n")
            return
        }
        let thisMod = get_this_module(prog)
        for_each_function(thisMod, "") $(var func) {
            if (func.flags.builtIn || func.flags.generated || func.flags._lambda) {
                return
            }
            print("// describe_function({func.name}) ->\n")
            print("{describe_function(func)}\n")
        }
    }
}
