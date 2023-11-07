//-file:suspicious-bracket

function foo(_p) { return "X" }

let x = { bar = "T" }

let _a = [
    foo(10)
    (x.bar) ? 10 : 20
]
