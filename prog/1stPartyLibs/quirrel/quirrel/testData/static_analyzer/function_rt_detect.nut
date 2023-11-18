
// -file:plus-string

function trim(s) { return s }

function _foo(p) { // EXPECTED
    if (p)
        return 1
    else
        return 1 + trim("A") + trim("B") + trim("C").join(true) + 9
}

function _bar(p) { // FP
    if (p)
        return "1"
    else
        return 2 + trim("A") + trim("B") + trim("C").join(true) + 8
}