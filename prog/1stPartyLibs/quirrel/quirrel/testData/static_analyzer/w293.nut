

let _x = persist("x", @() {}) // FP
let _y = persist("y", @() {}) // FP
let _z = persist("x", @() {}) // EXPECTED

function mkWatched(_f, _k) { }

function foo(p) { return p }

let _a = mkWatched(persist, "a") // FP
let _b = mkWatched(persist, "b") // FP
let _c = mkWatched(foo, "b") // FP
let _d = mkWatched(persist, "a") // EXPECTED