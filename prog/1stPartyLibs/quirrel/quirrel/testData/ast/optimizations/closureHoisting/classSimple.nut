
function foo(x, y) {
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