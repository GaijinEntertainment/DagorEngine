#allow-delete-operator
//-file:declared-never-used
//-file:unconditional-terminated-loop

let c = {}

foreach (a in c) {
    delete c.x          // FP 1
    c.rawdelete("y")    // FP 2
    break;
}

foreach (a in c) {
    delete c.x          // EXPECTED 1
    c.rawdelete("y")    // EXPECTED 2
}

function _f(arr) {
    foreach (a in arr) {
        c.remove(5)     // FP 3
        return 10;
    }
    return 20
}

foreach (a in c) {
    if (a > 0) {
        c.remove(5)     // FP 4
    }
    return 10;
}

foreach (a in c) {
    if (a < 0) {
        c.clear()     // EXPECTED 3
    } else {
        throw "EX"
    }
}


