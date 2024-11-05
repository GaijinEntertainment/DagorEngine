function foo(x, y) {
    return function(a, b) {
        println(a + b)
        return function(c, d) {
            println(c - d)
            return function(e, f) {
                println(e * f)
                return function(g, h) {
                    println(g / h)
                }
            }
        }
    }
}