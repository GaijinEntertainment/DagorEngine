options gen2

require dastest/testing_boost public
require inline_reader

// Expression-level (inline) reader macros: `%wrap! <expr> %%` rewrites to `( <expr> )` and is
// re-parsed in place. These lock the inline-rewrite path that the old module-level-only suffix
// could not reach (a `var x = %m~…%%` / in-function suffix used to strand the host statement).

def dbl(a : int) : int {
    return a * 2
}

[test]
def test_inline_plain(t : T?) {
    let x = %wrap! 41 + 1 %%
    t |> equal(x, 42, "plain: %wrap! 41 + 1 %% == 42")
}

[test]
def test_inline_in_expr(t : T?) {
    // rewrite is parenthesized, so it composes inside a larger expression: 1 + (20) * 2
    let z = 1 + %wrap! 20 %% * 2
    t |> equal(z, 41, "in-expr: 1 + (20) * 2 == 41")
}

[test]
def test_inline_as_arg(t : T?) {
    let r = dbl(%wrap! 5 %%)
    t |> equal(r, 10, "as-arg: dbl((5)) == 10")
}

[test]
def test_inline_pipe(t : T?) {
    let n = %wrap! [1, 2, 3] %% |> length()
    t |> equal(n, 3, "pipe: ([1,2,3]) |> length() == 3")
}

[test]
def test_inline_multiline(t : T?) {
    let s = %wrap!
        100 +
        200
    %%
    t |> equal(s, 300, "multiline body captured and re-parsed")
}

[test]
def test_inline_multiline_location(t : T?) {
    // %at_line! emits the source line the lexer assigned to the macro. A multi-line body must report
    // its OPENING line, not the closing %% line. `single` is a single-line baseline on line A; `multi`
    // opens on line A+1, so the difference must be 1 (was 3 before the multi-line location fix).
    let single = %at_line! x %%
    let multi = %at_line!
        body
    %%
    t |> equal(multi - single, 1, "inline macro reports its OPENING line, not the closing %% line")
}
