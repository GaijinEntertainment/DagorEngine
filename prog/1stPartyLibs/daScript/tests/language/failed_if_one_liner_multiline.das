// if/else/elif one-liner on separate line
// verifies that one-liner bodies without braces must be on the same line
options gen2
expect 20000:7    // syntax_error

// --- prefix if ---

def test_if_newline() {
    if (false)
        debug("a")         // error: if body on next line
}

def test_else_newline() {
    if (true) {
        debug("ok")
    } else
        debug("b")         // error: else body on next line
}

def test_if_and_else_newline() {
    if (false)
        debug("c")         // error: if body on next line
    else
        debug("d")         // error: else body on next line
}

def test_elif_newline() {
    if (false) {
        debug("e")
    } elif (true)
        debug("f")         // error: elif body on next line
}

def test_static_if_newline() {
    static_if (true)
        debug("g")         // error: static_if body on next line
}

// --- postfix if split across lines ---

def test_postfix_split(a : int) : int {
    // `return 13` becomes standalone statement, `if (a < 10)` becomes prefix if,
    // then `return a` on a new line triggers the one-liner error
    return 13
        if (a < 10)
    return a                // error: body of dangling prefix if on next line
}
