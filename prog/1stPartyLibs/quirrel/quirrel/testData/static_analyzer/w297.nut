

let x = {}

keepref(x) // FP


function _foo(p) {
    keepref(p) // EXPECTED 1
}


let _z = @(y) keepref(y) // EXPECTED 2

class _C {
    constructor(t){
        keepref(t) // EXPECTED 3
    }
}