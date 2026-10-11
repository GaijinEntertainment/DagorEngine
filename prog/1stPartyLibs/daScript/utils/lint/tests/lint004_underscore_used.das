options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT004: a leading underscore says "unused on purpose"; a `_name` local that IS read wears the
// wrong sign. Arguments are exempt - a parameter name is often constrained (`_in` dodging a
// keyword) and cannot drop the underscore.

expect 50503:1

require daslib/lint

def bad_used_underscore(x : int) : int {
    let _tmp = x * 2               // LINT004 - read below
    return _tmp + 1
}

def good_unused_underscore(x : int) : int {
    let _tmp = x * 2               // never read: the underscore is right
    return x
}

def good_underscore_arg(_in : int) : int {
    return _in + 1                 // arguments are exempt
}

[export]
def main() {
    print("{bad_used_underscore(1)} {good_unused_underscore(2)} {good_underscore_arg(3)}\n")
}
