options gen2
options auto_inline_functions = false
options _lint = "STYLE005"
options _nolint = "STYLE005"
// options _lint arms a rule the defaults turned off; options _nolint silences one.
// STYLE005 is default-off, so it fires here only because _lint names it - and it
// fires despite _nolint naming it too, which is the precedence: enable beats disable.

expect 31209:2

require daslib/style_lint

def private clamped(n : int) : int {
    if (n < 0) {
        return 0
    }
    if (n > 100) {
        return 100
    }
    return n
}

[export]
def main() {
    print("{clamped(-1)} {clamped(500)} {clamped(7)}\n")
}
