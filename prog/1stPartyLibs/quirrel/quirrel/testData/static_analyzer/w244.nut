

function fex(x) {
    this.y = x // FP 1
}


class _C {
    static function sss() {}
    function foo() {
       fex(@() this.y = 10) // FP 2
    }

    static function bar() {
        this.sss(); // FP 3
        this.y = 30 // EXPECTED 1
    }
}