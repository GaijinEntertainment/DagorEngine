function foo(x, y) {
    function bar(a, b) {
        function qux(c, d) {
            return c(d)
        }

        return qux(bar, b)
    }
}