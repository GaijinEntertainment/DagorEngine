local z = 0
let function foo(x, y) {
    return function(a, b) {
        z = a + b
        return function(c, d) {
            z = c - d
            return function(e, f) {
                z = e * f
                return function(g, h) {
                    return g / h
                }
            }
        }
    }
}