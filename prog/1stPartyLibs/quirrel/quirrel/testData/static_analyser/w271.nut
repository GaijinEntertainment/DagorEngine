//-file:declared-never-used

function loc(...) {}

function _foo(x) {
    if (x == 4)
        return $"$ yyyy={x}" // FP 1
    else
        return "$ xxxx={x}"  // EXPECTED
}

function _bar(a, b) {
    return "{a}/{b}".subst({a=a b=b}) // FP 2
}

function _qux(a, b) {
    return loc("bb/aa", "{x} ({y})", {x=b y=b})  // FP 3
}

function _bex(a, b) {
    return "jsadkl askdjal kjad".split("{css}") // FP 4
}
