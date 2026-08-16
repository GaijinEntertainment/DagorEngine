options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _cyclomatic_complexity = 0      // 0 disables STYLE037 for the module
// STYLE037 off-switch: 'options _cyclomatic_complexity = 0' disables the rule,
// so this complexity-11 function must stay silent (no expect line).

require daslib/style_lint

def hairy_but_allowed(x : int) : int {
    if (x == 1) return 10
    if (x == 2) return 20
    if (x == 3) return 30
    if (x == 4) return 40
    if (x == 5) return 50
    if (x == 6) return 60
    if (x == 7) return 70
    if (x == 8) return 80
    if (x == 9) return 90
    if (x == 10) return 100
    return 0
}
