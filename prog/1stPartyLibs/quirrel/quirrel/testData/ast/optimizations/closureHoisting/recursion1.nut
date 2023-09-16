let function foo(x, y) {
    let function bar(a, b) {
        let function qux(c, d) {
            return c(d)
        }

        return qux(bar, b)
    }
}