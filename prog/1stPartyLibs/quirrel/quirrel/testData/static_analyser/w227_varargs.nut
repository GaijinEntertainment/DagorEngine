

function foo(_a, ...) {
    return function (_b, ...) { return _a + _b; }
}


foo("...")