
local z = 0
function foo(x, y) {
    let ar = [function fff(a, b) {
        let c = class {
            x = 10
            function zed() {
                return 10 + this.x;
            }
        }
        return c().zed()
    }, function () { return 10 }, @(r) r * 2]
    ar[0](3, 4);
}