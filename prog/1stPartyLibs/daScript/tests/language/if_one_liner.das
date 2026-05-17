options gen2
require dastest/testing_boost public

// --- helpers for postfix if ---

def returns_13_if_less_10(a : int) : int {
    return 13 if (a < 10)
    return a
}

def returns_with_else(a : int) : int {
    return 13 if (a < 10) else return 42
}

// --- helpers for prefix if ---

def prefix_early_return(a : int) : int {
    if (a < 10) return 13
    return a
}

// --- helpers for multi-line one-liner shapes (previously rejected) ---
// expression_if_one_liner accepts return/yield/break/continue/expr_no_bracket;
// these helpers exercise the now-legal "body on the next line" layout.

def if_body_on_next_line(a : int) : int {
    if (a < 10)
        return 1
    return 0
}

def else_body_on_next_line(a : int) : int {
    if (a < 10) {
        return 1
    } else
        return 2
}

def if_and_else_bodies_on_next_lines(a : int) : int {
    if (a < 10)
        return 1
    else
        return 2
}

def elif_body_on_next_line(a : int) : int {
    if (a == 1) {
        return 1
    } elif (a == 2)
        return 2
    return 0
}

def static_if_true_body_on_next_line() : int {
    static_if (true)
        return 7
    return 0
}

def static_if_false_body_on_next_line() : int {
    static_if (false)
        return 7
    return 0
}

def postfix_if_split_across_lines(a : int) : int {
    // historically rejected: parses as `return 13` (unconditional)
    // followed by `if (a < 10) return a` (prefix-if, body on next line).
    // Compiles cleanly now; always returns 13.
    return 13
        if (a < 10)
    return a
}

// --- tests ---

[test]
def test_postfix_if(t : T?) {
    t |> run("return when condition is true") @(t : T?) {
        t |> equal(returns_13_if_less_10(5), 13)
    }
    t |> run("fall through when condition is false") @(t : T?) {
        t |> equal(returns_13_if_less_10(20), 20)
    }
    t |> run("boundary at 10") @(t : T?) {
        t |> equal(returns_13_if_less_10(10), 10)
    }
}

[test]
def test_postfix_if_else(t : T?) {
    t |> run("true branch") @(t : T?) {
        t |> equal(returns_with_else(5), 13)
    }
    t |> run("else branch") @(t : T?) {
        t |> equal(returns_with_else(20), 42)
    }
}

[test]
def test_prefix_if_oneliner(t : T?) {
    t |> run("return early when condition met") @(t : T?) {
        t |> equal(prefix_early_return(5), 13)
    }
    t |> run("fall through when condition not met") @(t : T?) {
        t |> equal(prefix_early_return(20), 20)
    }
}

[test]
def test_prefix_if_oneliner_body(t : T?) {
    t |> run("executes body when true") @(t : T?) {
        var x = 0
        if (true) { x = 42; }
        t |> equal(x, 42)
    }
    t |> run("skips body when false") @(t : T?) {
        var x = 0
        if (false) { x = 42; }
        t |> equal(x, 0)
    }
}

[test]
def test_oneliner_break_continue(t : T?) {
    t |> run("break in prefix if one-liner") @(t : T?) {
        var count = 0
        for (i in range(10)) {
            if (i == 3) break
            count++
        }
        t |> equal(count, 3)
    }
    t |> run("continue in prefix if one-liner") @(t : T?) {
        var count = 0
        for (i in range(6)) {
            if (i % 2 == 0) continue
            count++
        }
        t |> equal(count, 3)
    }
}

[test]
def test_if_body_on_next_line(t : T?) {
    t |> run("body executes when condition true") @(t : T?) {
        t |> equal(if_body_on_next_line(5), 1)
    }
    t |> run("body skipped when condition false") @(t : T?) {
        t |> equal(if_body_on_next_line(20), 0)
    }
}

[test]
def test_else_body_on_next_line(t : T?) {
    t |> run("if branch taken") @(t : T?) {
        t |> equal(else_body_on_next_line(5), 1)
    }
    t |> run("else branch taken") @(t : T?) {
        t |> equal(else_body_on_next_line(20), 2)
    }
}

[test]
def test_if_and_else_bodies_on_next_lines(t : T?) {
    t |> run("if branch taken") @(t : T?) {
        t |> equal(if_and_else_bodies_on_next_lines(5), 1)
    }
    t |> run("else branch taken") @(t : T?) {
        t |> equal(if_and_else_bodies_on_next_lines(20), 2)
    }
}

[test]
def test_elif_body_on_next_line(t : T?) {
    t |> run("if branch taken") @(t : T?) {
        t |> equal(elif_body_on_next_line(1), 1)
    }
    t |> run("elif branch taken") @(t : T?) {
        t |> equal(elif_body_on_next_line(2), 2)
    }
    t |> run("neither branch taken") @(t : T?) {
        t |> equal(elif_body_on_next_line(3), 0)
    }
}

[test]
def test_static_if_body_on_next_line(t : T?) {
    t |> run("static_if (true) body on next line returns 7") @(t : T?) {
        t |> equal(static_if_true_body_on_next_line(), 7)
    }
    t |> run("static_if (false) elides next-line body, returns 0") @(t : T?) {
        t |> equal(static_if_false_body_on_next_line(), 0)
    }
}

[test]
def test_postfix_if_split_across_lines(t : T?) {
    t |> run("split layout compiles and returns 13") @(t : T?) {
        t |> equal(postfix_if_split_across_lines(5), 13)
        t |> equal(postfix_if_split_across_lines(20), 13)
    }
}
