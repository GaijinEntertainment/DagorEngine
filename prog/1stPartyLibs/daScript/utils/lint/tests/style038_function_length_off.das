options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _function_length = 0            // 0 disables STYLE038 for the module
// STYLE038 off-switch: 'options _function_length = 0' disables the rule, so
// this 14-line function must stay silent (no expect line).

require daslib/style_lint

def long_but_allowed(x : int) : int {
    var r = x
    r += 11
    r += 2
    r += 3
    r += 4
    r += 5
    r += 6
    r += 7
    r += 8
    r += 9
    r += 10
    r += 11
    return r
}
