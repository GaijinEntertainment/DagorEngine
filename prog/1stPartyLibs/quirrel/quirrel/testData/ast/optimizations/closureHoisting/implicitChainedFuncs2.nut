
local z = 0
function foo(x, y) {
    return function(a, b) {
        let fff = function() {
            return z
        }
        return function(c, d) {
            z = x + a - c;
            return function(f, g) {
                return f - g - x
            }
        }
    }
}