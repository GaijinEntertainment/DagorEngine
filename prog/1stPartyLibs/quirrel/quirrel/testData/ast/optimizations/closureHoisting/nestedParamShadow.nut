// Test: nested function parameter shadows a captured variable.
// inner() captures 'x' from outer, but deep(x) has param 'x'.
// inner should NOT be hoisted (it captures from immediate parent).

function outer(x) {
    function inner() {
        function deep(x) { return x }
        return x
    }
    return inner()
}
