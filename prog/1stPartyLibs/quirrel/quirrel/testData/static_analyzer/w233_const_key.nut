function bar(_g) {}
function foo() {}

let { s, o } = foo()

const Z = "xxs"

let _seen = bar(@() o.value && s.value?[Z])