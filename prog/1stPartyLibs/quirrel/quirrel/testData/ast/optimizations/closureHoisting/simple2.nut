local z = 0
function foo(x, y) {
    let f1 = function(a) {
        let f2 = function(x) {
            let f3 = function(c) {
                return a + c
            }
            return x + y
        }
    }
}