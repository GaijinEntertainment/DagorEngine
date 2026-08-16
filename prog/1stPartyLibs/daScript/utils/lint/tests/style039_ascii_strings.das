options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
options _ascii_strings = true   // fixture is not under daslib/ or modules/ - force the rule on
// STYLE039: non-ASCII byte in a string literal
//
// Problem: error messages and other emitted strings flow into logs and consoles
// that are not UTF-8 aware; a non-ASCII byte (an em-dash, a multiplication sign)
// turns into mojibake there. Strings in shipped code must be ASCII unless the
// character is the point - then suppress per line with '// nolint:STYLE039'.

expect 31209:3

require daslib/style_lint

// --- Bad patterns (must fire - 3 total) ---

def bad_em_dash() : string {
    return "shader failed — see log"    // STYLE039 (em-dash U+2014)
}

def bad_multiply_suffix() : string {
    return "speedup ×4"    // STYLE039 (multiplication sign U+00D7)
}

def bad_interpolated(n : int) : string {
    return "value → {n}"    // STYLE039 (arrow in an interpolation chunk)
}

// --- Good patterns (must stay silent) ---

def good_ascii() : string {
    return "shader failed - see log (x4)"
}

def good_intended() : string {
    return "┌─┐"    // nolint:STYLE039 (box drawing is the point)
}

def good_escape_spelling() : string {
    return "\xEF\xBB\xBF"    // UTF-8 BOM via ASCII escapes - the source spelling is already log-safe
}

def good_escape_beside_typography() : string {
    return "\xEF\xBB\xBF"    // BOM — the em-dash in THIS comment must not fire the escape-spelled literal
}

[export]
def main() {
    print("{bad_em_dash()} {bad_multiply_suffix()} {bad_interpolated(3)} {good_ascii()} {good_intended()}\n")
}
