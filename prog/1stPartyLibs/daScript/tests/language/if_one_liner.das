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
