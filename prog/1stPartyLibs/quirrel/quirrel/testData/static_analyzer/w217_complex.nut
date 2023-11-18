
const MB = 3
function foo() {}
function bar(_p) {}
function qux(_p) {}
let n = {}

for (local offset = 1; offset < MB; offset++) {
    let idx = (n.v + offset) % MB
    if (foo() <= 0)
      continue
    foo()
    if (idx)
      bar(0.1)
    qux(idx)
    return true
}


for (;;) {
    foo()
    {
        foo()
        {
            foo()
            break
        }
    }
}

for (;;) {
    foo()
    try {
        foo()
        if (bar(10)) {
            qux(20)
            throw "x";
        } else {
            qux(30);
        }
        throw "z"
    } catch (_e) {
        foo()
        throw "y"
    }
    foo()
}

for (;;) {
    foo()
    if (qux(20)) {
        break
    }

    switch (bar(30)) {
        case 1:
            foo()
            break;
        case 2:
            bar(4000)
            break;
        default:
            foo()
    }

    return 0
}

for (;;) {
    foo()
    if (qux(20)) {
        break
    }

    switch (bar(30)) {
        case 1:
            foo()
            break;
        default:
            foo()
            continue
    }

    throw "3333"
}