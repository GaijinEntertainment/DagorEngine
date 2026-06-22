options gen2
require dastest/testing_boost public

// A block written on a single line no longer needs a trailing ';' on its last
// statement: `if (c) { foo() }` parses the same as the multi-line form. The
// lexer auto-inserts the statement terminator before a '}' that closes a
// semicolon-emitting block (see src/parser/ds2_lexer.lpp).

def private add_one(var x : int&) { x++ }

[test]
def test_oneliner_if(t : T?) {
    t |> run("if one-liner, no trailing semicolon") @(t : T?) {
        var x = 0
        if (true) { x = 42 }
        t |> equal(x, 42)
    }
    t |> run("if/else one-liner, no trailing semicolons") @(t : T?) {
        var x = 0
        if (false) { x = 1 } else { x = 2 }
        t |> equal(x, 2)
    }
    t |> run("elif chain one-liner") @(t : T?) {
        var x = 0
        if (false) { x = 1 } elif (true) { x = 2 } else { x = 3 }
        t |> equal(x, 2)
    }
}

[test]
def test_oneliner_bare_block(t : T?) {
    t |> run("bare block one-liner") @(t : T?) {
        var x = 0
        { x = 7 }
        t |> equal(x, 7)
    }
    t |> run("bare block one-liner with internal separator") @(t : T?) {
        var a = 0
        var b = 0
        { a = 1; b = 2 }
        t |> equal(a, 1)
        t |> equal(b, 2)
    }
    t |> run("nested one-liner blocks") @(t : T?) {
        var x = 0
        { { x = 9 } }
        t |> equal(x, 9)
    }
    t |> run("empty block still parses") @(t : T?) {
        let x = 5
        {}
        t |> equal(x, 5)
    }
}

[test]
def test_oneliner_loops(t : T?) {
    t |> run("for one-liner") @(t : T?) {
        var sum = 0
        for (i in range(4)) { sum += i }
        t |> equal(sum, 6)
    }
    t |> run("while one-liner") @(t : T?) {
        var n = 3
        var count = 0
        while (n > 0) { count += 1; n -= 1 }
        t |> equal(count, 3)
    }
}

[test]
def test_oneliner_last_statement_kinds(t : T?) {
    t |> run("call as last statement") @(t : T?) {
        var x = 0
        if (true) { add_one(x) }
        t |> equal(x, 1)
    }
    t |> run("multi-line still works (regression)") @(t : T?) {
        var x = 0
        if (true) {
            x = 1
            x += 1
        }
        t |> equal(x, 2)
    }
}
