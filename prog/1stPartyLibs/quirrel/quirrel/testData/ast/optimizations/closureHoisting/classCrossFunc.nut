local z = 0
function foo(x, y) {
    return function() {
        let c = class {
            x = 10
            function zed() {
                return function () {
                    return x + 10;
                }() + this.x
            }
        }
        return c().zed()
    }
}