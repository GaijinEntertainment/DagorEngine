local z = 0
let function foo(x, y) {
    let f1 = function(a) {
        let f2 = function(x) {
            let f3 = function(c) {
                return a + c
            }
            return x + y
        }
    }
}